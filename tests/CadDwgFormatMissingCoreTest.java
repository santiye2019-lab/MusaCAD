import com.musa.cad.CadDwgFormat;
import java.nio.charset.StandardCharsets;

/** A host without the common CAD library may not classify DWG in Java. */
public final class CadDwgFormatMissingCoreTest {
    public static void main(String[] args){
        boolean rejected=false;
        try{
            CadDwgFormat.identify("AC1032".getBytes(StandardCharsets.US_ASCII));
        }catch(IllegalStateException expected){
            rejected=expected.getMessage().contains("Ortak CAD çekirdeği");
        }
        if(!rejected)throw new AssertionError("Missing common CAD engine must fail closed");
        System.out.println("PASS: no Java-only DWG header recognition without common engine");
    }
}
