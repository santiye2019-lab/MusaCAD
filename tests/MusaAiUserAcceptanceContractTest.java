import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiUserAcceptanceContractTest {
    private static String read(String file)throws Exception{return Files.readString(Path.of(file),StandardCharsets.UTF_8);}
    private static void need(String s,String fragment,String feature){
        if(!s.contains(fragment))throw new AssertionError(feature+" missing "+fragment);
    }
    public static void main(String[]args)throws Exception{
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String worker=read("server/ai-worker/src/index.js");
        String workflow=read(".github/workflows/android.yml");
        need(panel,"bağlantı doğrulanmadı","no unverified AI identity");
        need(panel,"MusaAiProviderChoice.selected(activity)","persistent on-device model selection");
        need(panel,"setTitle(\"Çevrim içi yapay zekâ motoru\")","user-selected provider dialog");
        need(panel,"connectProvider.setOnClickListener","explicit connect button");
        need(panel,"host.onVerifyProvider(new Reply()","real authenticated provider probe");
        need(panel,"modelVerified.set(false)","switch resets stale online state");
        need(panel,"online&&modelVerified.get()","health alone never turns online LED green");
        need(main,"MusaAiCloudService.verifySelectedProvider(","no-DWG connection verification");
        String client=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        need(client,"body.put(\"providerPreference\",selectedProvider)","backend receives user choice");
        need(client,"selectedProvider.equals(actualProvider)","selected model response verified");
        need(worker,"selectAiProvider(env, requestedProvider)","provider routing via fixed server credentials");
        need(worker,"[\"\", \"gemini\", \"cloudflare\"]","reject arbitrary upstream destinations");
        need(panel,"MusaAiEngineLabel.display(provider,model)","real provider indicator");
        need(panel,"sweepProgress(int completed,int total,String stage)","persistent tile callback");
        need(panel,"sweepBar.setProgress","tile progress bar");
        need(panel,"bookStatus.setText","local price book status");
        need(main,"reply.modelInfo(cloud.provider,cloud.model)","real Cloudflare or Gemini attribution");
        need(main,"reply.sweepProgress(acceptedTiles,total","9 tile completed indicator");
        need(main,"engineeringBrief.addRegion(","deduplicated evidence register");
        need(main,"if(reply!=null)reply.progress(message);","book parsing update in AI panel");
        need(main,"chooseYfk2025Book(reply)","file picker never interpreted as chat text");
        need(parser,"cachedScreenEffect","pan stroke cache");
        need(parser,"layer.intersects(visibleWorld)","viewport geometry culling");
        need(worker,"BULGU | ÖNCELİK:","AI server concise engineering format");
        need(worker,"not a generic draft or invented example","no boilerplate");
        need(workflow,"inputs.release_approved == true","APK remains blocked during development");
        System.out.println("MusaAiUserAcceptanceContractTest OK");
    }
}
