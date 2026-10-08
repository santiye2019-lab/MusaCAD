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
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String nativeScene=read("app/src/main/java/com/musa/cad/NativeScene.java");
        String manifest=read("app/src/main/AndroidManifest.xml");

        require(view,"NAVIGATION_SETTLE_MS=90L","gesture settle window");
        require(view,"settleFastNavigation()","delayed full-vector redraw");
        require(view,"private boolean shouldUsePreviewForNavigation(){return false;}","bitmap navigation disabled");
        require(view,"authoritativeVectorFramePending=true","first complete vector frame after DWG upgrade");
        require(view,"if(authoritativeVectorFramePending)","authoritative color handoff");
        require(view,"if(visualOverlay)invalidate();","command changes avoid unnecessary full redraw");
        require(view,"CadNavigationPolicy.pinchScaleFactor","CAD-speed pinch response");
        require(view,"removeCallbacks(endFastNavigation);fastNavigation=true;","gesture-wide fast-render latch");
        require(view,"mode==Mode.PAN){settleFastNavigation();requestNavigationFrame();return true;}","pan release settles once");
        require(view,"drawVectorNavigation(c,imageMatrix","sharp vector navigation");

        require(main,"DWG vektör model hazırlanıyor","authoritative DWG first frame");
        if(main.contains("currentProject.parsed.bitmap.recycle()"))throw new AssertionError("Responsiveness regression: open-tab navigation preview is recycled");
        require(parser,"public void drawVectorNavigation","sharp vector navigation renderer");
        require(nativeScene,"navigationFast&&visible!=null&&offsets.length>50000","native LOD only during active navigation");
        require(manifest,"android:hardwareAccelerated=\"true\"","hardware-accelerated CAD canvas");

        require(print,"ExecutorService previewExecutor=Executors.newSingleThreadExecutor","non-blocking print preview");
        require(print,"mainHandler.postDelayed(kickoff,100L)","debounced automatic preview");
        require(print,"FİZİKSEL YAZICI veya PDF","physical printer visibility");
        require(print,"setPositiveButton(\"YAZDIR\",null)","clear print action");
        require(print,"fitDialogToPhone(activity,dialog)","phone-sized print dialog");
        require(print,"button.setTextColor(Color.WHITE)","readable print dialog actions");

        require(main,"makeDialogButtonsReadable(projectCloseDialog)","readable save/close actions");
        require(main,"button.setTextColor(Color.WHITE)","white close dialog actions");

        int cloudStart=main.indexOf("private void runMusaAiCloud");
        int cloudEnd=main.indexOf("private static boolean isGandalfPreviewCommand",cloudStart);
        if(cloudStart<0||cloudEnd<=cloudStart)throw new AssertionError("Responsiveness/UI regression: Gandalf cloud handler not found");
        String cloud=main.substring(cloudStart,cloudEnd);
        require(cloud,"aiExecutor.submit","Gandalf cloud background execution");
        require(cloud,"activeSnapshot.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS)","bounded background CAD-JSON projection");
        if(cloud.indexOf("activeSnapshot.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS)")<cloud.indexOf("aiExecutor.submit"))
            throw new AssertionError("Responsiveness/UI regression: CAD-JSON projection moved back onto UI thread");
        if(cloud.contains("currentAiDrawingIndex()"))
            throw new AssertionError("Responsiveness/UI regression: cloud handler must not build the full AI index on UI thread");
        require(parser,"public MusaAiDrawingIndex aiDrawingIndex(int maxItems)","bounded AI drawing index");
        require(main,"CLOUD_AI_INDEX_MAX_ITEMS=5000","bounded Gandalf projection budget");

        int generalStart=main.indexOf("private void runMusaAiGeneralProjectAnalysis");
        int generalEnd=main.indexOf("private void handleMusaAiCloudPrompt",generalStart);
        if(generalStart<0||generalEnd<=generalStart)
            throw new AssertionError("Gandalf general voice analysis handler missing");
        String general=main.substring(generalStart,generalEnd);
        require(main,"MusaAiDisciplineAnalyzer.asksGeneralProjectAnalysis(raw)","voice command routing");
        require(general,"localAiExecutor.submit","general CAD analysis uses dedicated local executor");
        require(general,"drawing.aiDrawingIndexQuickReview(","fast bounded local analysis projection");
        require(general,"MusaAiEngineeringReview.analyze(index,drawingName)","source-grounded engineering evidence output");
        require(general,"localAiHandler.postDelayed(deadline,QUICK_REVIEW_TIMEOUT_MS)","strict local watchdog");
        require(general,"finished.compareAndSet(false,true)","at most one local terminal reply");
        require(general,"reply.progress","visible local analysis progress");
        if(general.indexOf("drawing.aiDrawingIndexQuickReview(")<general.indexOf("localAiExecutor.submit"))
            throw new AssertionError("ANR regression: quick local indexing moved onto UI thread");
        require(parser,"public MusaAiDrawingIndex aiDrawingIndexQuickReview(","bounded light review projection");
        require(parser,"maxEntitiesToVisit","bounded entity visit count");
        require(main,"localAiExecutor.shutdownNow()","dedicated local executor shutdown");
        require(main,"MusaAiEngineeringReview.asksReview(raw)","engineering intent route");
        require(panel,"Mühendislik Analizi","engineer review quick action");
        if(main.indexOf("runMusaAiGeneralProjectAnalysis(reply);")>main.indexOf("if(MusaAiProjectPackage.asksPackageReview(raw))"))
            throw new AssertionError("Local voice analysis must be routed before expensive full-project logic");

        System.out.println("UI responsiveness contract OK: authoritative first-frame color, sharp vector pinch/pan, bounded background Gandalf CAD-JSON, reduced hot-frame allocations, immediate commands, asynchronous print preview and readable responsive actions are guarded.");
    }
}
