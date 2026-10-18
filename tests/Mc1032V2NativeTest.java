import com.musa.cad.Mc1032V2Native;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Arrays;

public final class Mc1032V2NativeTest {
    private static long bits(double x) { return Double.doubleToRawLongBits(x); }
    private static void check(boolean ok,String reason) {
        if (!ok) throw new AssertionError(reason);
    }
    public static void main(String[] args) throws Exception {
        Mc1032V2Native.Drawing input = new Mc1032V2Native.Drawing(
            new String[]{"0","MECHANICAL","STRUCTURE"},
            new int[]{-1,0x224466ff,0x335599ff},
            new long[]{
                71,1,1,bits(-1.5),bits(2),bits(550.25),
                       bits(3.0),bits(4.5),bits(2100.125),
                999,2,2,bits(12),bits(13.25),bits(-300),
                       bits(0),bits(0),bits(0)
            });
        byte[] written = Mc1032V2Native.encode(input);
        check(written.length == 48 + 64 * 3 + 80 * 2,"native binary length");
        check(new String(written,0,6,"US-ASCII").equals("MC1032") &&
              written[6] == 2,"native revision");
        Mc1032V2Native.Drawing decoded = Mc1032V2Native.decode(written);
        check(Arrays.equals(input.entities,decoded.entities),"3D coordinate bit exact");
        check(Arrays.equals(input.names,decoded.names),"layer names");
        check(Arrays.equals(input.colors,decoded.colors),"layer RGBA");
        byte[] broken = Arrays.copyOf(written,written.length);
        broken[broken.length - 1] ^= 4;
        try { Mc1032V2Native.decode(broken);
              throw new AssertionError("corruption accepted");
        } catch (IOException expected) {
            check(expected.getMessage().contains("butunlugu"),
                  "native CRC enforcement");
        }
        try { Mc1032V2Native.decode("AC1032".getBytes("US-ASCII"));
              throw new AssertionError("AC1032 imported as MC1032");
        } catch (IOException expected) { }
        Mc1032V2Native.Drawing duplicate = new Mc1032V2Native.Drawing(
            input.names,input.colors,input.entities.clone());
        duplicate.entities[9] = 71;
        try { Mc1032V2Native.encode(duplicate);
              throw new AssertionError("duplicate ID accepted");
        } catch (IOException expected) { }
        File directory = Files.createTempDirectory("musacad-mc1032-v2").toFile();
        try {
            File first = Mc1032V2Native.writeNew(directory,input);
            check(first.getName().endsWith(".mcad"),"own format extension");
            Mc1032V2Native.Drawing reopened = Mc1032V2Native.read(first);
            check(Arrays.equals(input.entities,reopened.entities),"file reopening");
            check(Arrays.equals(input.names,reopened.names),"file layer reopening");
            File other = Mc1032V2Native.writeNew(directory,input);
            check(!first.getCanonicalPath().equals(other.getCanonicalPath()),
                  "source never overwritten");
            check(first.length() == written.length,"first file intact");
            check(first.delete() && other.delete(),"temporary files cleanup");
        } finally { directory.delete(); }
        System.out.println("PASS: real MC1032 v2 JNI and unique mobile file reopen");
    }
}
