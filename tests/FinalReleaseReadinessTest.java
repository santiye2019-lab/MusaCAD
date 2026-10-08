import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class FinalReleaseReadinessTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Final readiness regression in "+area+": missing "+needle);
    }
    private static void requireGate(String workflow,String name){
        require(workflow,"- name: "+name,"quality gate");
    }
    public static void main(String[] args)throws Exception{
        String splash=read("app/src/main/java/com/musa/cad/SplashActivity.java");
        String license=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String manager=read("app/src/main/java/com/musa/cad/LicenseManager.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String mesh=read("app/src/main/java/com/musa/cad/Mesh3dActivity.java");
        String print=read("app/src/main/java/com/musa/cad/CadPrint.java");
        String workflow=read(".github/workflows/android.yml");

        require(splash,"LicenseManager.hasAccess(this)","startup/license gate");
        require(splash,"new Intent(this,MainActivity.class)","startup to main screen");
        require(license,"ShortLicenseCode.looksLikeShortCode","12-character license activation");
        require(manager,"ShortLicenseCode.verify","stored license verification");

        require(main,"homeOpenButton).setOnClickListener(v->open())","home DWG/DXF open");
        require(main,"cad.zoomBy(1.35f)","zoom controls");
        require(main,"layersButton).setOnClickListener(v->showLayers())","layer controls");
        require(main,"right3dButton).setOnClickListener(v->show3dToolsSheet())","3D entry");
        require(main,"CadPrint.show(","print flow");
        require(main,"Görünümü PDF olarak paylaş","PDF export/share flow");

        require(parser,"androidFamilyHint","font parity");
        require(parser,"drawVector(Canvas canvas,Matrix imageMatrix","vector DWG/DXF rendering");
        require(mesh,"Mesh3dView.InteractionMode.MEASURE","3D measurement");
        require(mesh,"Mesh3dView.InteractionMode.EDIT","3D editing");
        require(print,"PrintDocumentAdapter","Android print output");

        String[] gates={
            "Test Group 1 full-screen aspect preservation",
            "Test progressive native DWG first-frame and complete editor handoff",
            "Test CAD navigation zoom and culling policy",
            "Test DXF layer visibility rules",
            "Test DXF color palette",
            "Test DXF text styles and font metadata",
            "Test reference rendering parity contract",
            "Test 3D DXF surface geometry",
            "Android lint and unit tests",
            "Build and smoke-test native converter"
        };
        for(String gate:gates)requireGate(workflow,gate);

        require(workflow,"Build approved installable MusaCAD APK","final APK build route");
        require(workflow,"github.event_name == 'workflow_dispatch'","APK must stay explicit/approved");

        System.out.println("Final release readiness chain OK: startup/license/home/open/zoom/layers/colors/text/3D/print/PDF and final quality gates are wired.");
    }
}
