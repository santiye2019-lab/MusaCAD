import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** CI contract that 3D Android SAF output opens only after staged DXF validation. */
public final class Dxf3dSafeSaveIntegrationContractTest {
    public static void main(String[] args)throws Exception{
        String source=Files.readString(
            Path.of("app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        int callback=source.indexOf("if(request!=SAVE_DXF");
        int stage=source.indexOf("Dxf3dSafeSave.stage(",callback);
        int opened=source.indexOf("openOutputStream(uri,\"wt\")",callback);
        int published=source.indexOf("Dxf3dSafeSave.publish(staged,output)",callback);
        int deleted=source.indexOf("finally{if(staged!=null)staged.delete();}",callback);
        if(callback<0||stage<callback||opened<stage||published<opened||deleted<published)
            throw new AssertionError("3D DXF must validate stage before opening Android output URI and clean cache");
        if(source.substring(callback).contains("Dxf3dEditor.write(new File(path),output,mesh,original)"))
            throw new AssertionError("direct 3D writes to a truncating SAF stream are prohibited");
        System.out.println("3D SAF export contract: stage before open/truncate and cleanup");
    }
}
