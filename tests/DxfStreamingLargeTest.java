import com.musa.cad.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicInteger;

public class DxfStreamingLargeTest {
    public static void main(String[] args)throws Exception{
        File file=File.createTempFile("musacad-stream-", ".dxf");
        final int lineEntities=70000;
        try(BufferedWriter w=new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file),StandardCharsets.UTF_8),128*1024)){
            tag(w,0,"SECTION");tag(w,2,"HEADER");
            tag(w,9,"$INSUNITS");tag(w,70,"4");
            tag(w,9,"$LWDEFAULT");tag(w,370,"50");
            tag(w,9,"$LTSCALE");tag(w,40,"0.5");
            tag(w,0,"ENDSEC");

            tag(w,0,"SECTION");tag(w,2,"TABLES");
            tag(w,0,"LTYPE");tag(w,2,"DASHED");tag(w,49,"5");tag(w,49,"-2");
            tag(w,0,"LAYER");tag(w,2,"PIPE");tag(w,62,"1");tag(w,6,"DASHED");tag(w,370,"50");
            tag(w,0,"STYLE");tag(w,2,"ROMANS");tag(w,3,"romans.shx");tag(w,41,"0.8");
            tag(w,0,"ENDSEC");

            tag(w,0,"SECTION");tag(w,2,"BLOCKS");
            tag(w,0,"BLOCK");tag(w,2,"B1");tag(w,10,"0");tag(w,20,"0");tag(w,30,"0");
            line(w,"0",0,0,2,2);
            tag(w,0,"ENDBLK");
            tag(w,0,"ENDSEC");

            tag(w,0,"SECTION");tag(w,2,"ENTITIES");
            for(int i=0;i<lineEntities;i++)line(w,"PIPE",i,0,i+1,1);
            tag(w,0,"INSERT");tag(w,2,"B1");tag(w,8,"PIPE");tag(w,10,"100");tag(w,20,"100");tag(w,41,"1");tag(w,42,"1");
            tag(w,0,"ENDSEC");tag(w,0,"EOF");
        }

        long textLines=countLines(file);
        if(textLines<=600000)throw new AssertionError("fixture must exceed legacy line limit: "+textLines);

        AtomicInteger count=new AtomicInteger();final int[] direct={0},block={0};final int[] lastStart={-1};
        DxfBlocks.Result result=DxfBlocks.expand(file,StandardCharsets.UTF_8,new DxfBlocks.Sink(){
            public void accept(DxfBlocks.Placement p){
                count.incrementAndGet();
                if(p.directRoot)direct[0]++;else block[0]++;
                if(p.directRoot&&p.record.sourceStart<lastStart[0])throw new AssertionError("source ranges not monotonic");
                if(p.directRoot)lastStart[0]=p.record.sourceStart;
                if(p.record.sourceTo<=p.record.sourceStart)throw new AssertionError("empty source range");
            }
        });

        if(count.get()!=lineEntities+1)throw new AssertionError("placement count "+count.get());
        if(direct[0]!=lineEntities)throw new AssertionError("direct roots "+direct[0]);
        if(block[0]!=1)throw new AssertionError("block expansion "+block[0]);
        if(result.units!=4)throw new AssertionError("units");
        if(result.defaultLineweight!=50)throw new AssertionError("default lineweight");
        if(Math.abs(result.globalLineTypeScale-.5)>1e-9)throw new AssertionError("ltscale");
        if(!result.lineTypes.containsKey("DASHED"))throw new AssertionError("linetype");
        if(!result.textStyles.containsKey("ROMANS"))throw new AssertionError("style");
        if(!result.layerColors.containsKey("PIPE"))throw new AssertionError("layer");
        if(!result.placements.isEmpty())throw new AssertionError("streaming must not retain placements");
        if(file.length()>DxfStream.MAX_BYTES)throw new AssertionError("fixture unexpectedly too large");
        System.out.println("Streaming DXF passed: "+textLines+" lines, "+count+" placements, "+(file.length()/1048576d)+" MB");
        file.delete();
    }

    private static void line(BufferedWriter w,String layer,double x1,double y1,double x2,double y2)throws IOException{
        tag(w,0,"LINE");tag(w,8,layer);tag(w,10,Double.toString(x1));tag(w,20,Double.toString(y1));tag(w,11,Double.toString(x2));tag(w,21,Double.toString(y2));
    }
    private static void tag(BufferedWriter w,int code,String value)throws IOException{tag(w,Integer.toString(code),value);}
    private static void tag(BufferedWriter w,String code,String value)throws IOException{w.write(code);w.newLine();w.write(value);w.newLine();}
    private static long countLines(File file)throws IOException{long n=0;try(BufferedReader r=new BufferedReader(new FileReader(file))){while(r.readLine()!=null)n++;}return n;}
}
