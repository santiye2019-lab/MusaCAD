import com.musa.cad.DxfGroupCode;
import java.io.IOException;
/** Exact JNI integration with cross-platform C DXF pair policy. */
public final class DxfGroupPairTest {
    private static void ok(String code,String value,int previous,int limit,int wanted)
            throws Exception{
        int found=DxfGroupCode.parsePair(code,value,previous,limit);
        if(found!=wanted)throw new AssertionError("Code "+code+" => "+found+" wanted "+wanted);
    }
    private static void reject(String code,String value,int previous,int limit,String message)
            throws Exception{
        try {
            DxfGroupCode.parsePair(code,value,previous,limit);
            throw new AssertionError("Invalid pair accepted "+code);
        }catch(IOException expected){
            if(!expected.getMessage().contains(message))
                throw new AssertionError("Wrong pair failure: "+expected.getMessage(),expected);
        }
    }
    public static void main(String[]args)throws Exception{
        ok("  0  ","SECTION",0,32_000_000,0);
        ok("105","",2,32_000_000,105);
        ok("+1071","valid",31_999_999,32_000_000,1071);
        reject("10",null,0,32_000_000,"Eksik");
        reject("-1","bad",0,32_000_000,"Geçersiz");
        reject("1072","bad",0,32_000_000,"Geçersiz");
        reject("0","EOF",32_000_000,32_000_000,"sayısı");
        reject("0","EOF",-1,32_000_000,"parametreleri");
        reject("0","EOF",0,0,"parametreleri");
        reject(null,"a",0,10,"parametreleri");
        System.out.println("PASS: JNI calls ONE shared C DXF pair validator");
    }
}
