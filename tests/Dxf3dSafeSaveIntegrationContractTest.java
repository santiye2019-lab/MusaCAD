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
        String guardedStage=Files.readString(
            Path.of("app/src/main/java/com/musa/cad/Dxf3dSafeSave.java"),StandardCharsets.UTF_8);
        int shaBefore=guardedStage.indexOf("CadSaveSourceIntegrity.capture(source)");
        int sessionCheck=guardedStage.indexOf("openedSource.requireUnchanged(source)");
        int edit=guardedStage.indexOf("Dxf3dEditor.write(source,output,edited,original)");
        int afterVerify=guardedStage.indexOf("baseline.requireUnchanged(source)",edit);
        int sessionAfter=guardedStage.indexOf("openedSource.requireUnchanged(source)",edit);
        int stageReady=guardedStage.indexOf("ready=true;");
        if(sessionCheck<0||shaBefore<sessionCheck||edit<shaBefore||
            afterVerify<edit||sessionAfter<afterVerify||stageReady<sessionAfter)
            throw new AssertionError("Opening-session and staged SHA-256 must enclose 3D save/reopen");
        int openedHash=source.indexOf("CadSaveSourceIntegrity.capture(sourceFile)");
        int openedMesh=source.indexOf("Dxf3dMesh.read(sourceFile)",openedHash);
        int openedVerified=source.indexOf("openedIntegrity.requireUnchanged(sourceFile)",openedMesh);
        int openedAssigned=source.indexOf("openedSourceIntegrity=openedIntegrity;",openedVerified);
        if(openedHash<0||openedMesh<openedHash||openedVerified<openedMesh||
            openedAssigned<openedVerified||!source.contains("getCacheDir(),openedSourceIntegrity)"))
            throw new AssertionError("3D Activity must hash original bytes across load and guarded export");
        System.out.println("3D SAF export contract: stage before open/truncate and cleanup");
    }
}
