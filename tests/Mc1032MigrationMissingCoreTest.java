import com.musa.cad.Mc1032Migration;
import java.io.IOException;
public final class Mc1032MigrationMissingCoreTest {
    public static void main(String[]args)throws Exception {
        try{
            Mc1032Migration.upgrade(new byte[32]);
            throw new AssertionError("Java-only migration fallback found");
        }catch(IOException expected){
            if(!expected.getMessage().contains("cekirdegi"))throw expected;
        }
        System.out.println("PASS: MC1032 migration fails closed without shared engine");
    }
}
