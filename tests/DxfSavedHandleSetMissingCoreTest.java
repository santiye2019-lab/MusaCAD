import com.musa.cad.DxfSavedHandleSet;
import java.io.IOException;
public final class DxfSavedHandleSetMissingCoreTest {
    public static void main(String[] args)throws Exception {
        try {
            new DxfSavedHandleSet(20);
            throw new AssertionError("A Java-only CAD identity fallback was enabled");
        }catch(IOException expected) {
            if(!expected.getMessage().contains("Ortak CAD çekirdeği"))throw expected;
        }
        System.out.println("PASS: missing portable C identity set fails closed");
    }
}
