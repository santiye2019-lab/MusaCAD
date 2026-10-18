import com.musa.cad.DxfStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.io.IOException;
public final class DxfStreamMissingCoreTest {
    public static void main(String[]args)throws Exception {
        Path file=Files.createTempFile("musacad-dxf-no-core-", ".dxf");
        try{
            Files.writeString(file,"0\nEOF\n");
            try {
                new DxfStream(file.toFile(),StandardCharsets.UTF_8);
                throw new AssertionError("DxfStream has an independent Java byte-framer");
            }catch(IOException expected){
                if(!expected.getMessage().contains("Ortak CAD çekirdeği"))
                    throw expected;
            }
        }finally{Files.deleteIfExists(file);}
        System.out.println("PASS: seekable source DXF stream fails without shared CAD core");
    }
}
