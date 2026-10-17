import com.musa.cad.DxfSourceHandlePlan;
import java.io.IOException;
/** No Java-only fallback may silently invent source or saved entity handles. */
public final class DxfSourceHandleMissingCoreTest {
    public static void main(String[]args)throws Exception {
        try {
            DxfSourceHandlePlan.canonical("a1");
            throw new AssertionError("Missing shared C engine was silently ignored");
        } catch(IOException expected) {
            if(!expected.getMessage().contains("Ortak CAD çekirdeği"))
                throw new AssertionError("Wrong missing-core error",expected);
        }
        System.out.println("PASS: invalid native availability fails DXF save identity closed");
    }
}
