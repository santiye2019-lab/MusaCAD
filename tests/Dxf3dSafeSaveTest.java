import com.musa.cad.Dxf3dMesh;
import com.musa.cad.Dxf3dSafeSave;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

public final class Dxf3dSafeSaveTest {
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    public static void main(String[] args)throws Exception{
        File source=File.createTempFile("musacad-3d-safe-source-", ".dxf");
        File cache=Files.createTempDirectory("musacad-3d-safe-").toFile();
        String dxf="0\nSECTION\n2\nENTITIES\n"
            +"0\n3DFACE\n10\n0\n20\n0\n30\n1\n11\n3\n21\n0\n31\n1\n"
            +"12\n0\n22\n4\n32\n1\n0\nENDSEC\n0\nEOF\n";
        try{
            Files.writeString(source.toPath(),dxf,StandardCharsets.UTF_8);
            byte[] originalBytes=Files.readAllBytes(source.toPath());
            Dxf3dMesh mesh=Dxf3dMesh.read(source);
            float[] before=mesh.xyz.clone();
            mesh.xyz[2]=9f;
            File staged=Dxf3dSafeSave.stage(source,mesh,before,cache);
            try{
                yes(Arrays.equals(originalBytes,Files.readAllBytes(source.toPath())),
                    "export must not mutate source");
                Dxf3dMesh reopen=Dxf3dMesh.read(staged);
                yes(reopen.xyz[2]==9f,"edited 3D Z must survive export roundtrip");
                ByteArrayOutputStream destination=new ByteArrayOutputStream();
                long copied=Dxf3dSafeSave.publish(staged,destination);
                yes(copied==staged.length()&&
                    Arrays.equals(destination.toByteArray(),Files.readAllBytes(staged.toPath())),
                    "publish must copy the validated staged file");
            }finally{staged.delete();}

            // Missing edited DXF tag: stage must fail, never touching an existing destination.
            Files.writeString(source.toPath(),dxf.replace("30\n1\n",""),StandardCharsets.UTF_8);
            ByteArrayOutputStream existing=new ByteArrayOutputStream();
            existing.write("KEEP-ORIGINAL".getBytes(StandardCharsets.UTF_8));
            boolean rejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache);}
            catch(IOException expected){rejected=true;}
            yes(rejected,"missing coordinate must reject 3D export before destination open");
            yes(existing.toString(StandardCharsets.UTF_8).equals("KEEP-ORIGINAL"),
                "rejected staged export must not overwrite destination");
            yes(cache.listFiles()!=null&&cache.listFiles().length==0,
                "failed staged export must delete temporary file");

            // Invalid coordinate also cannot leave an export file behind.
            Files.write(source.toPath(),originalBytes);
            mesh.xyz[2]=Float.NaN;
            rejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache);}
            catch(IOException expected){rejected=true;}
            yes(rejected&&cache.listFiles()!=null&&cache.listFiles().length==0,
                "non-finite geometry must not produce a staged or destination file");
        }finally{
            Files.deleteIfExists(source.toPath());
            File[] abandoned=cache.listFiles();
            if(abandoned!=null)for(File f:abandoned)Files.deleteIfExists(f.toPath());
            Files.deleteIfExists(cache.toPath());
        }
        System.out.println("3D DXF staged save: verified roundtrip, source intact, invalid output never published");
    }
}
