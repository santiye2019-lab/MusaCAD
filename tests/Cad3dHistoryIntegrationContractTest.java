import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Regression guard for single drag transactions, numeric axis steps, and safe DXF export. */
public final class Cad3dHistoryIntegrationContractTest {
    private static void require(boolean condition,String reason){
        if(!condition)throw new AssertionError(reason);
    }
    public static void main(String[]args)throws Exception{
        String ui=Files.readString(Path.of("app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        String view=Files.readString(Path.of("app/src/main/java/com/musa/cad/Mesh3dView.java"),StandardCharsets.UTF_8);
        require(ui.contains("private final Cad3dEditHistory editHistory=new Cad3dEditHistory()"),
            "3D editor owns bounded per-session undo/redo");
        require(ui.contains("editHistory.clear();"),"opening a new mesh clears stale undo entries");
        require(ui.contains("meshView.setAxisDragCommitListener(this::recordCompletedAxisDrag)"),
            "completed gizmo drag is attached to transaction recorder");
        require(ui.contains("editHistory.record(mesh.xyz,vertex,before,after)"),
            "one completed pointer drag becomes one undo action");
        require(ui.contains("editHistory.record(mesh.xyz,vertex,before,next)"),
            "numeric UCS X/Y/Z buttons contribute to the same undo stack");
        require(ui.contains("editHistory.undo(mesh.xyz)")&&ui.contains("editHistory.redo(mesh.xyz)"),
            "both undo and redo must restore mesh coordinates");
        require(ui.contains("undo3dButton.setOnClickListener(v->perform3dHistory(false))")&&
                ui.contains("redo3dButton.setOnClickListener(v->perform3dHistory(true))"),
            "undo and redo are user-actionable controls");
        require(ui.contains("meshView.isAxisDragging()"),
            "history cannot replay during unfinished touch gestures");
        require(ui.contains("if(saving||mesh==null||mesh.solidProxy||mesh.acisSolids>0"),
            "gizmo still rejects ACIS, proxy and saving state");
        require(ui.contains("Dxf3dSafeSave.stage(")&&ui.contains("Dxf3dSafeSave.publish("),
            "DXF save continues to validate a staged file before publication");

        require(view.contains("private AxisDragCommitListener axisDragCommitListener"),
            "view has transaction-complete callback");
        require(view.contains("axisDragCommitListener.finished(dragVertex,dragOriginal.clone(),after)"),
            "drag finish reports an immutable before snapshot and final XYZ");
        int move=view.indexOf("private void moveDraggedAxis(");
        int next=view.indexOf("private void drawEdges(",move);
        require(move>=0&&next>move&&!view.substring(move,next).contains("axisDragCommitListener.finished("),
            "pointer preview must not produce a history entry");
        int cancel=view.indexOf("private void restoreCancelledDrag()");
        int finishCancel=view.indexOf("private void moveDraggedAxis(",cancel);
        require(cancel>=0&&finishCancel>cancel&&
            !view.substring(cancel,finishCancel).contains("axisDragCommitListener.finished("),
            "pointer cancellation must never create an undo transaction");
        require(view.contains("case MotionEvent.ACTION_CANCEL:")&&
            view.contains("restoreCancelledDrag();return true;"),
            "touch cancellation must restore starting coordinates");
        System.out.println("3D undo/redo Android integration contract: one drag, steps, safe DXF and cancel passed");
    }
}
