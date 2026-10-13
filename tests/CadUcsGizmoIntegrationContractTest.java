import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Checks touch-mode routing, read-only source rejection, and preservation of guarded DXF save. */
public final class CadUcsGizmoIntegrationContractTest {
    private static void check(boolean b,String reason){if(!b)throw new AssertionError(reason);}
    public static void main(String[] args)throws Exception{
        String view=Files.readString(Path.of("app/src/main/java/com/musa/cad/Mesh3dView.java"),StandardCharsets.UTF_8);
        String ui=Files.readString(Path.of("app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        check(view.contains("drawUcsGizmo(canvas)")&&view.contains("CadGizmoMath.pick("),
            "X/Y/Z screen handles must be drawable and selectable");
        check(view.contains("CadGizmoMath.drawingDistance(")&&view.contains("gizmoUcs.move("),
            "drag must project screen pointer movement onto selected UCS axis");
        check(view.contains("if(startAxisDrag(lastX,lastY))"),
            "gizmo must capture pointer before orbit handling");
        check(view.contains("if(axisDragging){restoreCancelledDrag();moved=true;}"),
            "second finger must cancel drag and permit pinch");
        check(view.contains("case MotionEvent.ACTION_CANCEL:")&&
              view.contains("restoreCancelledDrag();return true;"),
            "Android pointer cancellation must restore original vertex");
        check(view.contains("interactionMode!=InteractionMode.EDIT"),
            "non-edit orbit and measure modes must never activate gizmo");
        check(view.contains("model.sourceLines[vertex]>=0"),
            "read-only/block-expanded vertices cannot show draggable gizmo");
        check(ui.contains("meshView.setAxisDragListener(this::moveGizmoVertex)"),
            "UI must wire editing callback");
        check(ui.contains("meshView.setUcsFrame(editingUcs)"),
            "UI must keep visible gizmo aligned with active UCS");
        check(ui.contains("mesh.sourceLines[vertex]<0||mesh.coordinateSlots[vertex]<0"),
            "3D edit must refuse unsupported block/source vertices");
        check(ui.contains("if(saving||mesh==null||mesh.solidProxy||mesh.acisSolids>0"),
            "gizmo must not mutate mesh during save or ACIS/proxy preview");
        check(ui.contains("Math.abs(value)>1e9d"),
            "gizmo coordinates must be finite and range-limited");
        check(ui.contains("Dxf3dSafeSave.stage(")&&ui.contains("Dxf3dSafeSave.publish("),
            "staged DXF writer must be retained");
        System.out.println("3D UCS gizmo contract passed: select, drag, cancel, read-only and safe save");
    }
}
