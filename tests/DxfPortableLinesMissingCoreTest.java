import com.musa.cad.DxfPortableLines;
import java.io.*;
import java.nio.file.*;
public final class DxfPortableLinesMissingCoreTest {
    public static void main(String[]args)throws Exception{
        Path p=Files.createTempFile("musacad-no-c-dxf",".dxf");
        try {
            try {
                new DxfPortableLines(p.toFile());
                throw new AssertionError("Java-only line parser fallback was enabled");
            }catch(IOException expected){
                if(!expected.getMessage().contains("Ortak CAD çekirdeği"))throw expected;
            }
        }finally{Files.deleteIfExists(p);}
        System.out.println("PASS: missing portable DXF byte scanner fails closed");
    }
}
