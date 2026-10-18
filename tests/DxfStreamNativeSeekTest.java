import com.musa.cad.DxfStream;
import com.musa.cad.DxfBlocks;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.io.*;
/**
 * Real portable C JNI framing through the streaming source record path.
 * No APK, DWG parity, or Java-only fallback is exercised here.
 */
public final class DxfStreamNativeSeekTest {
    private static void require(boolean ok,String detail){
        if(!ok)throw new AssertionError(detail);
    }
    private static void rejects(File input)throws Exception {
        try(DxfStream read=new DxfStream(input,StandardCharsets.ISO_8859_1)){
            try{
                while(read.next()!=null){}
                throw new AssertionError("Overlong native source line accepted");
            }catch(IOException expected){
                require(expected.getMessage().contains("fazla uzun"),
                    "native bound must fail closed: "+expected);
            }
        }
    }
    public static void main(String[]args)throws Exception {
        String longValue="X".repeat(66000);
        String first="0\r\nSECTION\r\n2\nENTITIES\n";
        String second="0\nLINE\n8\nA\rB\n999\n"+longValue+
            "\n10\n0\n20\n0\n11\n1\n21\n1\n";
        String rest="0\nENDSEC\n0\nEOF";
        String content=first+second+rest;
        Path source=Files.createTempFile("musacad-dxf-shared-seek-", ".dxf");
        try {
            Files.writeString(source,content,StandardCharsets.ISO_8859_1);
            long lineOffset=first.getBytes(StandardCharsets.ISO_8859_1).length;
            long endsecOffset=(first+second).getBytes(StandardCharsets.ISO_8859_1).length;
            try(DxfStream reader=new DxfStream(source.toFile(),StandardCharsets.ISO_8859_1)) {
                require("SECTION".equals(reader.next().type),"SECTION parsed");
                require(reader.nextRecordOffset==lineOffset,"record source byte offset");
                DxfBlocks.Record line=reader.next();
                require("LINE".equals(line.type),"line survives 64KiB read boundary");
                require(reader.nextRecordOffset==endsecOffset,"next source record offset");
                require("ENDSEC".equals(reader.next().type),"ENDSEC parsed");
                require("EOF".equals(reader.next().type),"EOF parsed");
                for(int i=0;i<3;i++)require(reader.next()==null,"no phantom EOF replay");
                // Real seek reinitializes native pending bytes and source offsets.
                reader.seek(lineOffset,4);
                require("LINE".equals(reader.next().type),"seek to entity byte boundary");
                require(reader.nextRecordOffset==endsecOffset,"seek byte origin");
                require("ENDSEC".equals(reader.next().type),"seek next boundary");
                reader.seek(0,0);
                require("SECTION".equals(reader.next().type),"restart from byte zero");
            }
            final int[] placements={0};
            final String[] savedLayer={null};
            DxfBlocks.expand(source.toFile(),StandardCharsets.ISO_8859_1,
                placement->{placements[0]++;savedLayer[0]=placement.layer;});
            require(placements[0]==1,"single LINE preserved");
            require("A\rB".equals(savedLayer[0]),
                "interior CR must remain intact on seekable stream");

            // The native per-line cap is enforced in all source streaming paths.
            Files.writeString(source,"0\nLINE\n8\n"+"Z".repeat(1048577)+"\n",
                StandardCharsets.ISO_8859_1);
            rejects(source.toFile());
            System.out.println("PASS: shared-C DxfStream chunk, inner CR, byte-offset, seek and EOF");
        }finally{Files.deleteIfExists(source);}
    }
}
