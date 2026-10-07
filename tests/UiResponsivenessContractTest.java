import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class UiResponsivenessContractTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Responsiveness/UI regression in "+area+": missing "+needle);
    }

    public static void main(String[]args)throws Exception{
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String print=read("app/src/main/java/com/musa/cad/CadPrint.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String nativeScene=read("app/src/main/java/com/musa/cad/NativeScene.java");

        require(view,"NAVIGATION_SETTLE_MS=160L","gesture settle window");
        require(view,"settleFastNavigation()","delayed full-vector redraw");
        require(view,"shouldUsePreviewForNavigation()","fast navigation cache");
        require(view,"authoritativeVectorFramePending=true","first complete vector frame after DWG upgrade");
        require(view,"if(authoritativeVectorFramePending)","authoritative color handoff");
        require(view,"if(visualOverlay)invalidate();","command changes avoid unnecessary full redraw");
        require(view,"CadNavigationPolicy.pinchScaleFactor","CAD-speed pinch response");
        require(view,"removeCallbacks(endFastNavigation);fastNavigation=true;","gesture-wide fast-render latch");
        require(view,"mode==Mode.PAN){settleFastNavigation();requestNavigationFrame();return true;}","pan release settles once");
        require(view,"nativeDrawing.draw(c,imageMatrix,fastNavigation)","native renderer quality/gesture split");

        require(main,"DxfParser.render(converted,true)","DWG authoritative navigation preview");
        if(main.contains("currentProject.parsed.bitmap.recycle()"))throw new AssertionError("Responsiveness regression: open-tab navigation preview is recycled");
        require(parser,"Bitmap.Config.ARGB_8888","full-color navigation preview");
        require(nativeScene,"navigationFast&&visible!=null&&offsets.length>50000","native LOD only during active navigation");

        require(print,"ExecutorService previewExecutor=Executors.newSingleThreadExecutor","non-blocking print preview");
        require(print,"mainHandler.postDelayed(kickoff,100L)","debounced automatic preview");
        require(print,"FİZİKSEL YAZICI veya PDF","physical printer visibility");
        require(print,"setPositiveButton(\"YAZDIR\",null)","clear print action");
        require(print,"fitDialogToPhone(activity,dialog)","phone-sized print dialog");
        require(print,"button.setTextColor(Color.WHITE)","readable print dialog actions");

        require(main,"makeDialogButtonsReadable(projectCloseDialog)","readable save/close actions");
        require(main,"button.setTextColor(Color.WHITE)","white close dialog actions");

        System.out.println("UI responsiveness contract OK: persistent full-color preview, gesture-latched fast navigation, native idle color fidelity, immediate commands, asynchronous print preview and readable responsive actions are guarded.");
    }
}
