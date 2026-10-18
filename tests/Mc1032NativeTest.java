import com.musa.cad.Mc1032Native;
import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;

public final class Mc1032NativeTest {
    private static void ok(boolean b,String why){if(!b)throw new AssertionError(why);}
    private static long bits(double value){return Double.doubleToRawLongBits(value);}
    public static void main(String[] args) throws Exception {
        long[] entities={
            42L,1L,bits(-15.25),bits(0.5),bits(5.0),bits(8.25),bits(0),bits(0),bits(0),
            2L,2L,bits(1),bits(-3),bits(0),bits(0),bits(2.75),bits(0),bits(0),
            100L,3L,bits(4),bits(6),bits(0),bits(0),bits(2),bits(30),bits(270),
            101L,4L,bits(-1),bits(1),bits(0),bits(0),bits(0),bits(0),bits(0)
        };
        byte[] encoded=Mc1032Native.encode(entities);
        ok(encoded.length==32+4*72,"incorrect own native binary length");
        ok(new String(encoded,0,6,"US-ASCII").equals("MC1032"),"own magic");
        ok(Arrays.equals(entities,Mc1032Native.decode(encoded)),
            "exact IEEE double bits + source handles preserved");
        byte[] bad=Arrays.copyOf(encoded,encoded.length);
        bad[50]^=1;
        try{Mc1032Native.decode(bad);throw new AssertionError("CRC not enforced");}
        catch(java.io.IOException expected){ok(expected.getMessage().contains("checksum"),"error route");}
        byte[] ac="AC1032".getBytes("US-ASCII");
        try{Mc1032Native.decode(ac);throw new AssertionError("AC/DWG misread");}
        catch(java.io.IOException expected){}
        long[] duplicate=Arrays.copyOf(entities,entities.length);
        duplicate[9]=42L;
        try{Mc1032Native.encode(duplicate);throw new AssertionError("duplicate handle allowed");}
        catch(java.io.IOException expected){}
        File dir=Files.createTempDirectory("musa-mc1032-").toFile();
        try {
            File source=Mc1032Native.writeNew(dir,entities);
            ok(source.getName().endsWith(".mcad"),"own file extension");
            ok(Arrays.equals(entities,Mc1032Native.read(source)),"real file reopen");
            File second=Mc1032Native.writeNew(dir,entities);
            ok(!second.getCanonicalPath().equals(source.getCanonicalPath()),"never overwrite");
            ok(source.length()==encoded.length,"first file untouched");
            ok(source.delete(),"delete first temp");ok(second.delete(),"delete second temp");
        }finally{dir.delete();}
        System.out.println("PASS: MC1032 real JNI codec and safe mobile file reopen");
    }
}
