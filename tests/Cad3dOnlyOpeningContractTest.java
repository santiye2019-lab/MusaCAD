import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.charset.StandardCharsets;

/**
 * Static integration guard: a 3D-only model may skip the normal 2D route
 * without altering the native DWG conversion or inventing 2D geometry.
 * Runtime geometry is validated by Dxf3dMeshTest.
 */
public final class Cad3dOnlyOpeningContractTest {
    private static String read(String path)throws Exception {
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void yes(boolean condition,String failure){
        if(!condition)throw new AssertionError(failure);
    }
    public static void main(String[]args)throws Exception {
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String mesh=read("app/src/main/java/com/musa/cad/Dxf3dMesh.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        yes(main.contains("if(prepare3dOnly(loaded,loaded.file))"),"DXF-only route missing");
        yes(main.contains("if(prepare3dOnly(loaded,converted))"),"DWG conversion route missing");
        yes(main.contains("private void handOff3dOnly("),"3D-only project lifecycle missing");
        yes(main.contains("project.threeDOnly=true;"),"3D-only project marker missing");
        yes(main.contains("project.workingDxf=loaded.workingDxf;"),"3D input file not retained");
        yes(main.contains("currentProject.threeDOnly"),"3D-only status/activation missing");
        yes(main.contains("open3d(Mesh3dActivity.MODE_ISO)"),"real 3D activity not opened");
        yes(mesh.contains("!solidProxy"),"unverified ACIS proxy accepted as real geometry");
        yes(mesh.contains("acisSolids>0 || !flatPlan"),"ordinary flat plans incorrectly routed");
        yes(parser.contains("if(entities.isEmpty())return null"),"2D empty model policy changed");
        String fallback=main.substring(main.indexOf("private boolean prepare3dOnly("),
            main.indexOf("private void scheduleRecentNativeRecord("));
        yes(!fallback.contains("DxfParser.blankDrawing()"),
            "3D-only fallback must not fabricate a 2D document");
        yes(main.contains("project.preparingEditor=false;"),"3D-only project stays indefinitely loading");
        System.out.println("3D-only DXF/DWG safe opening and lifecycle integration contract passed.");
    }
}
