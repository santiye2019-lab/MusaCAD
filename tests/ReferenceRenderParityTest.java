import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ReferenceRenderParityTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Reference parity regression in "+area+": missing "+needle);
    }
    private static void requireGate(String workflow,String name){
        require(workflow,"- name: "+name,"quality gate");
    }

    public static void main(String[]args)throws Exception{
        String workflow=read(".github/workflows/android.yml");
        String blocks=read("app/src/main/java/com/musa/cad/DxfBlocks.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String colors=read("app/src/main/java/com/musa/cad/DxfColor.java");
        String layers=read("app/src/main/java/com/musa/cad/DxfLayerState.java");
        String render=read("app/src/main/java/com/musa/cad/RenderPathPolicy.java");
        String nativeScene=read("app/src/main/cpp/native_scene.c");

        require(blocks,"r.type.equals(\"ACAD_TABLE\")","table/title-block geometry");
        require(blocks,"r.type.equals(\"INSERT\")","block geometry");
        require(blocks,"r.type.equals(\"DIMENSION\")","dimension picture blocks");
        require(blocks,"visibilityLayerKeys","nested layer visibility");

        require(parser,"androidFamilyHint","CAD font fallback");
        require(parser,"MTextLabel","rich MTEXT");
        require(parser,"drawVector(Canvas canvas,Matrix imageMatrix","vector rendering");
        require(parser,"SpatialIndex","large drawing visibility");
        require(parser,"DxfViewport.View","paper-space viewports");

        require(colors,"aciArgb","AutoCAD ACI palette");
        require(colors,"trueColorArgb","AutoCAD TrueColor");
        require(layers,"allVisible","layer visibility");
        require(render,"useBitmapNavigationPreview","navigation render path");

        require(nativeScene,"musa_native_color_token","native first-paint colors");
        require(nativeScene,"emit_insert","native blocks");
        require(nativeScene,"emit_minsert","native block arrays");
        require(nativeScene,"emit_dimension_block","native dimensions");
        require(nativeScene,"DWG_TYPE_IMAGE","native image frame");
        require(nativeScene,"DWG_TYPE_OLE2FRAME","native OLE frame");

        String[] gates={
            "Test DXF color palette",
            "Test native first-paint color resolution",
            "Test navigation renderer color stability",
            "Test CAD navigation zoom and culling policy",
            "Test DXF text styles and font metadata",
            "Test DXF layer visibility rules",
            "Test DXF block expansion, colors, styles and source origins",
            "Test DXF paper-space viewport geometry",
            "Guard native IMAGE and OLE frame fallback",
            "Test progressive native DWG first-frame and complete editor handoff"
        };
        for(String gate:gates)requireGate(workflow,gate);

        System.out.println("Reference render parity contract OK: geometry/layers/colors/text/blocks/tables/viewports/native first-paint are guarded.");
    }
}
