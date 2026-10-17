import com.musa.cad.DxfGroupCode;
import java.io.IOException;

/** Direct JVM/JNI test against the exact portable C validation implementation. */
public final class DxfGroupCodeTest {
    private static void check(String text,int expected)throws Exception{
        int code=DxfGroupCode.parse(text);
        if(code!=expected)throw new AssertionError(text+" => "+code+" instead of "+expected);
    }
    private static void reject(String text)throws Exception{
        try {
            DxfGroupCode.parse(text);
            throw new AssertionError("Invalid group code accepted: "+text);
        }catch(IOException expected){}
    }
    public static void main(String[]args)throws Exception{
        check("0",0);check("1071",1071);check("  +5 \t",5);
        check("0000000000000000000000000105",105);
        reject("1072");reject("-1");reject("0000001072");
        reject("1e2");reject("1 0");reject("+");reject("");
        reject("١٠");reject("９");reject("notanumber");
        System.out.println("PASS: JVM DXF group-code admission delegates to portable C");
    }
}
