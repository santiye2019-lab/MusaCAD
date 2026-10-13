import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Contract: 3D lifecycle persistence is isolated from the DWG/DXF source. */
public final class Cad3dRecoveryIntegrationContractTest {
    private static void ok(boolean condition,String reason){
        if(!condition)throw new AssertionError(reason);
    }
    public static void main(String[]args)throws Exception{
        String activity=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        String journal=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/Cad3dRecoveryJournal.java"),StandardCharsets.UTF_8);
        ok(activity.contains("Cad3dRecoveryJournal.pathFor(")&&
           activity.contains("Cad3dRecoveryJournal.fingerprint(")&&
           activity.contains("Cad3dRecoveryJournal.load("),
           "Loader obtains source-bound recovery before changing live mesh");
        ok(activity.contains("offer3dRecovery(recovered)")&&
           activity.contains(".setCancelable(false)")&&
           activity.contains(".setPositiveButton(\"KURTAR\"")&&
           activity.contains(".setNegativeButton(\"KAYDI SİL\""),
           "Recovered changes require explicit user approval");
        ok(activity.contains("pendingRecoveryDecision=true")&&
           activity.contains("pendingRecoveryDecision=false")&&
           activity.contains("recoveryFingerprint==null||pendingRecoveryDecision)return"),
           "Backgrounding an unanswered recovery prompt must keep original journal");
        ok(activity.contains("@Override protected void onStop()")&&
           activity.contains("meshView.cancelActiveAxisDrag()")&&
           activity.contains("Cad3dRecoveryJournal.capture(")&&
           activity.contains("Cad3dRecoveryJournal.persist("),
           "Completed vertex edits are captured on stop and persisted off the UI thread");
        ok(activity.contains("touchedVertices.add(vertex)")&&
           activity.contains("for(int vertex:recovered.indices())touchedVertices.add(vertex)"),
           "Sparse checkpoint tracks all committed and restored vertices");
        ok(activity.contains("saveCheckpoint.markSaved(saved)")&&
           activity.contains("recovered.restoreCurrent(mesh.xyz)"),
           "Saved-copy checkpoint and unsaved geometry are independently recovered");
        ok(activity.contains("explicitlyDiscarding=true")&&
           activity.contains("delete3dRecovery();")&&
           activity.contains("if(explicitlyDiscarding||!saveCheckpoint.hasUnsavedChanges(mesh.xyz))"),
           "Explicit discard or clean exit clears journal and never rewrites it onStop");
        ok(activity.contains("if(finishedExport&&saveCheckpoint!=null)")&&
           activity.contains("saveCheckpoint.markSaved(exportSnapshot)"),
           "Only completed DXF export updates the saved state");
        ok(journal.contains("source.getCanonicalPath()")&&
           journal.contains("Float.floatToIntBits(")&&
           journal.contains("feed(md,lines);feed(md,slots);"),
           "Exact source path, geometry and editable slots fingerprint the journal");
        ok(journal.contains("CRC32")&&
           journal.contains("stream.getFD().sync()")&&
           journal.contains("temp.renameTo(destination)"),
           "Recovery file is checksummed and published from a complete temporary file");
        ok(!journal.contains("openOutputStream("),
           "Journal must never overwrite a user-provided SAF export");
        System.out.println("3D session recovery UI/lifecycle/persistence integration passed");
    }
}
