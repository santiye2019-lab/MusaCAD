import com.musa.cad.CadSaveSourceIntegrity;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

public final class CadSaveSourceIntegrityTest {
    private static void yes(boolean value,String why){
        if(!value)throw new AssertionError(why);
    }
    private static boolean rejected(CadSaveSourceIntegrity baseline,File file){
        try{baseline.requireUnchanged(file);return false;}
        catch(IOException expected){return true;}
    }
    public static void main(String[] args)throws Exception{
        File source=File.createTempFile("musacad-integrity-", ".dwg");
        try{
            byte[] original=new byte[4096];
            for(int i=0;i<original.length;i++)original[i]=(byte)(i*17+23);
            Files.write(source.toPath(),original);
            long originalStamp=source.lastModified();
            CadSaveSourceIntegrity baseline=CadSaveSourceIntegrity.capture(source);
            baseline.requireUnchanged(source);

            // Catch adversarial/concurrent same-size edits even with original
            // modification time restored. A size/mtime guard cannot do this.
            byte[] modified=original.clone();
            modified[2048]^=0x17;
            Files.write(source.toPath(),modified);
            source.setLastModified(originalStamp);
            yes(source.length()==original.length,"size must match original");
            yes(rejected(baseline,source),"same-size source mutation must reject");
            Files.write(source.toPath(),original);
            baseline.requireUnchanged(source);

            Files.write(source.toPath(),new byte[]{1,2,3});
            yes(rejected(baseline,source),"source truncation must reject");
            Files.delete(source.toPath());
            yes(rejected(baseline,source),"missing source must reject");
            boolean blocked=false;
            try{CadSaveSourceIntegrity.capture(source);}
            catch(IOException expected){blocked=true;}
            yes(blocked,"missing source cannot create integrity snapshot");
        }finally{Files.deleteIfExists(source.toPath());}
        System.out.println("CAD SHA-256 source-integrity and same-size tamper rejection PASS");
    }
}
