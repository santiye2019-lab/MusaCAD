import com.musa.cad.Cad3dOnlyOpenGuard;
import java.io.IOException;

/** No duplicate Java CAD logic is permitted when the native shared engine is absent. */
public final class Cad3dOnlyMissingCoreTest {
    public static void main(String[] args)throws Exception {
        try {
            Cad3dOnlyOpenGuard.requireRealGeometry(null);
            throw new AssertionError("A missing common CAD library must NEVER silently accept geometry");
        } catch (IOException expected) {
            if (!expected.getMessage().contains("Ortak CAD çekirdeği"))
                throw new AssertionError("Must report missing common engine explicitly",expected);
        }
        System.out.println("PASS: no alternative Android CAD algorithm without shared core");
    }
}
