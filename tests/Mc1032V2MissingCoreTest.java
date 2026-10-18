import com.musa.cad.Mc1032V2Native;
import java.io.IOException;
public final class Mc1032V2MissingCoreTest {
    public static void main(String[] args) throws Exception {
        try {
            Mc1032V2Native.encode(new Mc1032V2Native.Drawing(
                new String[]{"0"},new int[]{-1},new long[0]));
            throw new AssertionError("Java-only CAD codec fallback detected");
        } catch(IOException expected) {
            if (!expected.getMessage().contains("cekirdegi")) throw expected;
        }
        System.out.println("PASS: MC1032 v2 fails closed without own C engine");
    }
}
