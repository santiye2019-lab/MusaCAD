import com.musa.cad.DxfSectionGuard;
import java.io.IOException;
public final class DxfSectionGuardMissingCoreTest{
    public static void main(String[]args)throws Exception{
        try{
            new DxfSectionGuard().consume("0","SECTION",0,10);
            throw new AssertionError("Java CAD fallback was wrongly enabled");
        }catch(IOException expected){
            if(!expected.getMessage().contains("Ortak CAD çekirdeği"))throw expected;
        }
        System.out.println("PASS: missing native DXF section engine blocks approval");
    }
}
