import com.musa.cad.DxfPortableLines;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.io.*;
public final class DxfPortableLinesTest {
    private static void eq(String expected,String actual){
        if(expected==null?actual!=null:!expected.equals(actual))
            throw new AssertionError("expected ["+expected+"] got ["+actual+"]");
    }
    private static Path file(byte[] data)throws Exception {
        Path p=Files.createTempFile("musacad-native-dxf-lines-",".dxf");
        Files.write(p,data);return p;
    }
    private static void lines(byte[] bytes,String... expected)throws Exception {
        Path p=file(bytes);
        try(DxfPortableLines reader=new DxfPortableLines(p.toFile())){
            for(String line:expected)eq(line,reader.line());
            eq(null,reader.line());eq(null,reader.line());
        }finally{Files.deleteIfExists(p);}
    }
    public static void main(String[]args)throws Exception {
        lines("0\r\nSECTION\r\n2\nENTITIES\n".getBytes(StandardCharsets.ISO_8859_1),
              "0","SECTION","2","ENTITIES");
        lines("a\rbc\n\nX".getBytes(StandardCharsets.ISO_8859_1),"abc","","X");
        lines(new byte[0]);
        byte[] bytes=new byte[65535+8];
        java.util.Arrays.fill(bytes,0,65535,(byte)'x');
        bytes[65535]=(byte)13;bytes[65536]=(byte)10;
        bytes[65537]=(byte)'Y';bytes[65538]=(byte)10;
        byte[] trimmed=java.util.Arrays.copyOf(bytes,65539);
        Path p=file(trimmed);
        try(DxfPortableLines reader=new DxfPortableLines(p.toFile())){
            String first=reader.line();
            if(first.length()!=65535||first.charAt(0)!='x'||first.charAt(65534)!='x')
                throw new AssertionError("Chunk-split content lost");
            eq("Y",reader.line());
            eq(null,reader.line());
        }finally{Files.deleteIfExists(p);}
        byte[] oversize=new byte[1048577];
        java.util.Arrays.fill(oversize,(byte)'Z');
        p=file(oversize);
        try(DxfPortableLines reader=new DxfPortableLines(p.toFile())){
            try{reader.line();throw new AssertionError("overlong native line accepted");}
            catch(IOException expected){
                if(!expected.getMessage().contains("fazla uzun"))throw expected;
            }
        }finally{Files.deleteIfExists(p);}
        System.out.println("PASS: real portable C/JNI DXF line framing with split chunks and byte cap");
    }
}
