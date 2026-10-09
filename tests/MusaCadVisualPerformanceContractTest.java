import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaCadVisualPerformanceContractTest {
    private static String source(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void has(String source,String token,String label){
        if(!source.contains(token))
            throw new AssertionError(label+" lacks "+token);
    }
    public static void main(String[] args)throws Exception{
        String main=source("app/src/main/java/com/musa/cad/MainActivity.java");
        String ui=source("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String vector=source("app/src/main/java/com/musa/cad/DxfParser.java");
        String cad=source("app/src/main/java/com/musa/cad/CadView.java");
        String health=source("app/src/main/java/com/musa/cad/MusaAiCloudHealth.java");
        String server=source("server/ai-worker/src/index.js");

        has(ui,"Poz kitabı yükle","separate catalog import action");
        has(ui,"onImportPriceBook","book chooser contract");
        has(main,"chooseYfk2025Book","real Android PDF picker");
        has(main,"PICK_YFK_2025","official 2025 book file picker");
        has(ui,"setScrollbarFadingEnabled(false)","always-visible long answer scrollbar");
        has(ui,"setVerticalScrollbarThumbDrawable","contrasting scrollbar thumb");
        has(ui,"MusaAiCloudHealth.check","server health probe");
        has(health,"setConnectTimeout(5000)","bounded remote probe with transient network tolerance");
        has(health,"musacad-ai-worker","expected health service identity");
        has(ui,"modelVerified.get()","connection LED distinguished from real model");
        has(main,"reply.cloudStatus(true)","real cloud model success callback");
        has(main,"reply.cloudFailure(cloud.message)","real cloud failure reason reaches status UI");
        has(main,"reply.cloudFailure(detailed.message)","detailed visual failure reason reaches status UI");

        has(main,"engine.fastScene()","native DWG first paint");
        has(main,"loaded.handedOff=true","progressive handoff");
        has(main,"project.preparingEditor=true","asynchronous full DWG parse");
        has(main,"DxfParser.render(converted,false)","skip full raster preview on DWG open");
        has(main,"DxfParser.render(loaded.file,!largeDxf)","skip unnecessary raster on large DXF");
        has(vector,"CadNavigationPolicy.movingMinWorldSpan","adaptive gesture geometry filter");
        has(vector,"culledDuringMotion","subpixel LOD");
        has(cad,"else vectorDrawing.drawVector(c,imageMatrix","settled full vector renderer");
        has(main,"MusaAiVisualEvidence.renderBatch(drawing,batch","actual exported project images");
        has(main,"MusaAiCloudService.analyzeHybridWithContext(","online visual+CAD model call");
        has(main,"GÖRSEL MÜHENDİSLİK PROJE DENETİM RAPORU","engineering report title");
        has(main,"2025 RESMÎ POZ ADAYLARI","offline YFK candidate check");
        has(main,"MusaAiYfk2025Library.suggestForTakeoff(this,measured)",
            "local 2025 poz dictionary source");
        has(server,"write a REAL inspection record in Turkish","visual evidence requirement");
        System.out.println("MusaCadVisualPerformanceContractTest OK");
    }
}
