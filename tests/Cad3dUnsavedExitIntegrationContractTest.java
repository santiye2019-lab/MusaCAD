import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Android source contract: no uncommitted 3D gesture or unsaved exit is silent. */
public final class Cad3dUnsavedExitIntegrationContractTest {
    private static void require(boolean ok,String explanation){
        if(!ok)throw new AssertionError(explanation);
    }
    public static void main(String[] args)throws Exception{
        String activity=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        String view=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/Mesh3dView.java"),StandardCharsets.UTF_8);

        require(activity.contains("saveCheckpoint=new Cad3dSaveCheckpoint(original)"),
            "new mesh resets last-export checkpoint");
        require(activity.contains("saveCheckpoint.hasUnsavedChanges(mesh.xyz)"),
            "exit guard compares geometry to the last successful export");
        require(activity.contains("private void request3dExit()")&&
                activity.contains("@Override public void onBackPressed()"),
            "legacy system back must pass through exit guard");
        require(activity.contains("registerOnBackInvokedCallback(")&&
                activity.contains("unregisterOnBackInvokedCallback("),
            "Android 13+ back invokes the same exit guard without a leaked callback");
        require(activity.contains(".setPositiveButton(\"DXF KOPYASI KAYDET\"")&&
                activity.contains(".setNegativeButton(\"KAYDETMEDEN ÇIK\"")&&
                activity.contains(".setNeutralButton(\"İPTAL\",null)"),
            "unsaved exit offers save copy, explicit discard and cancellation");
        require(activity.contains("if(saving){\n            android.widget.Toast.makeText(this"),
            "closing is blocked throughout an active export");
        require(activity.contains("meshView.cancelActiveAxisDrag();"),
            "exit/picker/axis steps cancel uncommitted drag previews");
        require(view.contains("public boolean cancelActiveAxisDrag()")&&
                view.contains("if(!axisDragging)return false;")&&
                view.contains("restoreCancelledDrag();"),
            "touch cancellation restores original XYZ before exiting/saving");

        require(activity.contains("final float[] exportSnapshot=Arrays.copyOf(mesh.xyz,mesh.xyz.length)"),
            "save captures the exact geometry it intends to publish");
        require(activity.contains("if(finishedExport&&saveCheckpoint!=null){")&&
                activity.contains("saveCheckpoint.markSaved(exportSnapshot);"),
            "failed or canceled saves cannot mark edits clean");
        int published=activity.indexOf("Dxf3dSafeSave.publish(staged,");
        int success=activity.indexOf("exported=true;",published);
        require(published>=0&&success>published,
            "successful publication precedes checkpoint advancement");
        require(activity.contains("Dxf3dSafeSave.stage("),
            "staged DXF validation remains enabled");
        System.out.println("3D unsaved-exit Android integration contract: back, picker, preview, checkpoint passed");
    }
}
