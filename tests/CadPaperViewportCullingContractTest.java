import java.nio.file.Files;
import java.nio.file.Path;

/** Ensure the paper-space renderer culls only a viewport's projected bounds. */
public final class CadPaperViewportCullingContractTest {
    private static void check(boolean good,String why){if(!good)throw new AssertionError(why);}
    public static void main(String[]args)throws Exception {
        String parser=Files.readString(Path.of("app/src/main/java/com/musa/cad/DxfParser.java"));
        int method=parser.indexOf("private static void drawViewportModels(");
        int next=parser.indexOf("private static ViewportEntity viewportOf(",method);
        check(method>=0&&next>method,"paper-space renderer missing");
        String body=parser.substring(method,next);
        check(body.contains("combined.invert(screenToModel)"),"viewport must invert exact draw matrix");
        check(body.contains("screenToModel.mapRect(visibleModel)"),"visible model bounds not computed");
        check(body.contains("layer.intersects(visibleModel)"),"model entities not culled by true bounds");
        check(body.contains("visibleModel!=null"),"non-invertible matrices must use full renderer");
        check(body.contains("canvas.clipRect(clip)"),"viewport clipping must remain");
        check(body.contains("layer.drawStyled(canvas,paint,combined,print,mono,global)"),
            "original styled draw call must be preserved");
        System.out.println("Paper-space viewport safe-clip regression contract passed");
    }
}
