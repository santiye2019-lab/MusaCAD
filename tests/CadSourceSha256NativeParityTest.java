import com.musa.cad.CadSaveSourceIntegrity;
import java.io.File;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Arrays;
import java.lang.reflect.Field;

/** Independent JDK SHA-256 reference against the SAME native C implementation
 * used on Android, Windows and iPhone. Test-only reflection reads immutable
 * snapshot digests; no production getter exposing sensitive bytes.
 */
public final class CadSourceSha256NativeParityTest {
    private static final Field digestField;
    static{
        try {
            digestField=CadSaveSourceIntegrity.class.getDeclaredField("digest");
            digestField.setAccessible(true);
        }catch(ReflectiveOperationException ex){throw new ExceptionInInitializerError(ex);}
    }
    private static void check(int size)throws Exception{
        byte[] original=new byte[size];
        for(int i=0;i<size;i++)original[i]=(byte)(i*197+37);
        File file=File.createTempFile("musacad-sha-core-parity-",".dwg");
        try{
            Files.write(file.toPath(),original);
            byte[] actual=(byte[])digestField.get(CadSaveSourceIntegrity.capture(file));
            byte[] expected=MessageDigest.getInstance("SHA-256").digest(original);
            if(!Arrays.equals(actual,expected))
                throw new AssertionError("Common C SHA-256 mismatch vs JDK at "+size+" bytes");
            if(size>0){
                CadSaveSourceIntegrity baseline=CadSaveSourceIntegrity.capture(file);
                long stamp=file.lastModified();
                byte[] changed=original.clone();
                changed[size/2]^=0x01;
                Files.write(file.toPath(),changed);
                file.setLastModified(stamp);
                try{
                    baseline.requireUnchanged(file);
                    throw new AssertionError("Undetected same-size source edit");
                }catch(java.io.IOException expectedFailure){}
            }
        }finally{Files.deleteIfExists(file.toPath());}
    }
    public static void main(String[]args)throws Exception{
        for(int n:new int[]{1,3,55,56,63,64,65,127,128,129,65535,65536,65537,131072,1000000})
            check(n);
        System.out.println("PASS: source SHA-256 C/JDK byte parity 1..1,000,000 bytes, multi-chunk and tamper");
    }
}
