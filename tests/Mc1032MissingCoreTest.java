import com.musa.cad.Mc1032Native;
import java.io.IOException;
public final class Mc1032MissingCoreTest {
    public static void main(String[] args)throws Exception{
        try{Mc1032Native.encode(new long[0]);throw new AssertionError("Java codec fallback");}
        catch(IOException expected){
            if(!expected.getMessage().contains("cekirdegi"))
                throw expected;
        }
        System.out.println("PASS: MC1032 fails closed without first-party native engine");
    }
}
