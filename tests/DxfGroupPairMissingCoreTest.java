import com.musa.cad.DxfGroupCode;
import java.io.IOException;
public final class DxfGroupPairMissingCoreTest {
    public static void main(String[]args)throws Exception{
        try {
            DxfGroupCode.parsePair("0","SECTION",0,32_000_000);
            throw new AssertionError("Java-only DXF pair acceptance is prohibited");
        }catch(IOException expected){
            if(!expected.getMessage().contains("Ortak CAD çekirdeği"))
                throw new AssertionError("Missing core must fail closed",expected);
        }
        System.out.println("PASS: DXF group-pair JNI fails closed when C engine is absent");
    }
}
