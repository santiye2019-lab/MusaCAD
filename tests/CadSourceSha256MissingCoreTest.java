import com.musa.cad.CadSaveSourceIntegrity;
import java.io.File;
import java.nio.file.Files;
import java.io.IOException;
/** Source integrity MUST fail closed when portable C digest is absent. */
public final class CadSourceSha256MissingCoreTest {
    public static void main(String[]args)throws Exception{
        File f=File.createTempFile("musacad-core-absent-",".dwg");
        try{
            Files.write(f.toPath(),new byte[]{65,66,67,68});
            try{
                CadSaveSourceIntegrity.capture(f);
                throw new AssertionError("Without native core CAD source must not be approved");
            }catch(IOException expected){
                if(!expected.getMessage().contains("Ortak CAD çekirdeği"))
                    throw new AssertionError("Wrong missing-core error",expected);
            }
        }finally{Files.deleteIfExists(f.toPath());}
        System.out.println("PASS: CAD source SHA-256 fails closed without common core");
    }
}
