import com.musa.cad.MusaAiAnalysisIntent;
import com.musa.cad.MusaAiConversationalIntent;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Turkish negative commands must not trigger local metraj before Qwen. */
public final class MusaAiNegatedTakeoffRouteTest {
    private static void ok(boolean test,String message) {
        if(!test)throw new AssertionError(message);
    }
    public static void main(String[]args)throws Exception {
        String request="Açık Silivrikapı DWG projesini yalnızca görsel yapay zekâ ile "+
            "9 bölge üzerinden analiz et. Her bölgeye ait teknik bulguları "+
            "ve tamamlanan bölge sayısını raporla. "+
            "Şimdilik metraj veya keşif oluşturma.";
        ok(MusaAiAnalysisIntent.prohibitsTakeoff(request),"Detect negative keşif creation");
        ok(request.equals(MusaAiConversationalIntent.canonical(request)),
            "Never replace negative with positive generate-BOQ command");
        ok(MusaAiAnalysisIntent.isReview(request),"Negative takeoff must not veto visual review");
        ok(!MusaAiAnalysisIntent.prohibitsTakeoff("Projeden keşif oluştur"),
            "Positive BOQ must remain positive");
        ok(!MusaAiAnalysisIntent.prohibitsTakeoff("Metraj çıkar"),
            "Standalone metraj must work");
        ok(MusaAiAnalysisIntent.prohibitsTakeoff(
            "Bu proje için keşif istemiyorum, görsel analiz et"),
            "Negative choice respected");
        ok(MusaAiAnalysisIntent.isReview(
            "Bu proje için keşif istemiyorum, görsel analiz et"),
            "Negative choice still permits image analysis");
        String main=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        ok(main.contains("if(MusaAiAnalysisIntent.prohibitsTakeoff(q))return false;"),
            "Positive BOQ generator must honor negation");
        ok(main.contains("synthesis.includeTakeoff=!MusaAiAnalysisIntent.prohibitsTakeoff(raw)"),
            "Post-Qwen metraj should be opt-in and respect negatives");
        ok(main.contains("if(acceptedBatches>0)"),
            "No local detailed synthesis before cloud image results");
        ok(main.contains("MusaAiEngineeringSynthesis.compose(synthesis)"),
            "Final response must be compiled from Qwen images and CAD evidence");
        System.out.println("MusaAiNegatedTakeoffRouteTest OK");
    }
}
