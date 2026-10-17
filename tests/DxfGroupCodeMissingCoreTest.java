import com.musa.cad.DxfGroupCode;
import java.io.IOException;

/** Host process without native C cannot use an alternative Java CAD parser. */
public final class DxfGroupCodeMissingCoreTest {
    public static void main(String[]args)throws Exception{
        try {
            DxfGroupCode.parse("5");
            throw new AssertionError("Missing core incorrectly accepted group code");
        }catch(IOException expected){
            if(!expected.getMessage().contains("Ortak CAD çekirdeği"))
                throw new AssertionError("Missing core must fail closed",expected);
        }
        System.out.println("PASS: no duplicate Java DXF group parsing when native core is absent");
    }
}
