import com.musa.cad.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

public class DxfStreamTest {
    private static final String START="0\nSECTION\n2\nENTITIES\n",END="0\nENDSEC\n0\nEOF\n";
    private static void check(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static File file(String text)throws IOException {
        File f=File.createTempFile("musacad-stream-",".dxf");
        Files.write(f.toPath(),text.getBytes(StandardCharsets.UTF_8));return f;
    }
    private static void fails(String text)throws Exception {
        File f=file(text);
        try{DxfBlocks.expand(f,StandardCharsets.UTF_8,p->{});throw new AssertionError("Malformed file accepted");}
        catch(IOException expected){}finally{f.delete();}
    }
    public static void main(String[] args)throws Exception {
        if(args.length>0){
            long start=System.nanoTime();Map<String,Integer> counts=new TreeMap<>();
            DxfBlocks.Result result=DxfBlocks.expand(new File(args[0]),StandardCharsets.UTF_8,p->{
                counts.merge(p.record.type,1,Integer::sum);
                double[] xy=p.transform.point(1,2);check(Double.isFinite(xy[0])&&Double.isFinite(xy[1]),"Nonfinite transform");
            });
            check(result.placements.isEmpty(),"Streaming retains placements");
            System.out.println("Records: "+counts+" skipped INSERTs: "+result.skipped+" seconds: "+(System.nanoTime()-start)/1e9);return;
        }
        // ENTITIES before BLOCKS exercises the two-pass index. CRLF and multibyte block names test byte offsets.
        String text=START+"0\nINSERT\n2\nİç\n8\nBORU\n10\n10\n20\n20\n0\nINSERT\n2\nİç\n10\n100\n0\nINSERT\n2\nMISSING\n0\nENDSEC\n"+
            "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n2\nİç\n10\n1\n20\n2\n0\nINSERT\n2\nALT\n0\nENDBLK\n"+
            "0\nBLOCK\n2\nALT\n0\nLINE\n10\n3\n20\n4\n11\n5\n21\n4\n0\nMTEXT\n1\nTürkçe yazı\n0\nENDBLK\n"+END;
        File f=file(text.replace("\n","\r\n"));
        try{
            DxfBlocks.Result baseline=DxfBlocks.expand(Arrays.asList(text.split("\n")));
            List<DxfBlocks.Placement> actual=new ArrayList<>();
            DxfBlocks.Result result=DxfBlocks.expand(f,StandardCharsets.UTF_8,actual::add);
            check(result.skipped==baseline.skipped&&actual.size()==baseline.placements.size(),"Expansion counts");
            for(int i=0;i<actual.size();i++){
                DxfBlocks.Placement a=actual.get(i),b=baseline.placements.get(i);
                check(a.record.type.equals(b.record.type)&&a.layer.equals(b.layer),"Type/layer");
                check(Arrays.equals(a.transform.point(3,4),b.transform.point(3,4)),"Nested placement");
                check(a.record.tags.equals(b.record.tags.subList(b.record.from,b.record.to)),"Text and tags");
            }
            Thread.currentThread().interrupt();
            try{DxfBlocks.expand(f,StandardCharsets.UTF_8,p->{});throw new AssertionError("Cancellation ignored");}
            catch(InterruptedIOException expected){}finally{Thread.interrupted();}
        }finally{f.delete();}
        fails(START+"0\nLINE\n10\n");fails(START+"invalid\n3\n"+END);fails(START+"0\nLINE\n");
        fails(START+"0\nTEXT\n1\n"+"a".repeat(65537)+"\n"+END);
        // A file over the old 32 MiB cap must stream within a 64 MiB Java heap.
        f=File.createTempFile("musacad-large-",".dxf");
        try{
            try(Writer out=Files.newBufferedWriter(f.toPath(),StandardCharsets.UTF_8)){
                out.write(START);String ignored="x".repeat(256);
                for(int i=0;i<150000;i++)out.write("0\nLINE\n10\n1\n20\n2\n11\n3\n21\n4\n1000\n"+ignored+"\n");
                out.write(END);
            }
            check(f.length()>32L*1024*1024,"Fixture too small");
            int[] count={0};DxfBlocks.expand(f,StandardCharsets.UTF_8,p->count[0]++);
            check(count[0]==150000,"Large file count");
        }finally{f.delete();}
        System.out.println("Streaming parity, UTF-8/CRLF offsets, cancellation, malformed input and large-file tests passed");
    }
}
