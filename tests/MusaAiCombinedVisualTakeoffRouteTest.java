import com.musa.cad.MusaAiAnalysisIntent;
import com.musa.cad.MusaAiConversationalIntent;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiCombinedVisualTakeoffRouteTest {
    static void check(boolean ok,String msg) {
        if(!ok)throw new AssertionError(msg);
    }
    static void preserved(String prompt) {
        check(MusaAiAnalysisIntent.isCombinedVisualReview(prompt)||
              MusaAiAnalysisIntent.isReview(prompt),
              "Visual review not recognized");
        check(prompt.equals(MusaAiConversationalIntent.canonical(prompt)),
              "The full visual review and BOQ instructions were replaced");
    }
    public static void main(String[]args)throws Exception {
        String original="Açık olan Silivrikapı Spor Köyü Kafe–Mutfak DWG projesini "+
            "kapsamlı görsel ve sayısal olarak analiz et. Projeyi 3×3 düzeninde "+
            "toplam 9 görsel bölgeye ayır ve tamamını sırayla incele. "+
            "Her bölgedeki mekanik tesisat hatlarını ve cihazları tanımla. "+
            "Doğrulanabilen ölçüler üzerinden metraj ve ÇŞİDB poz keşif ön raporu hazırla. "+
            "DWG dosyasında değişiklik yapma.";
        preserved(original);
        preserved("Açık DWG projesini görsel analiz et ve keşif oluştur.");
        preserved("Çizimi 9 bölgelik görsel analizle tara ve metraj çıkar.");
        preserved("Mekanik tesisat projesini görsel incele, malzeme listesi ve keşif hazırla.");
        check("Projeden keşif oluştur".equals(
              MusaAiConversationalIntent.canonical("Projeden keşif hazırla")),
              "Standalone BOQ generation must stay local");
        check("Bu projede metraj çıkar".equals(
              MusaAiConversationalIntent.canonical("Bu projede metraj çıkar")),
              "Standalone takeoff must remain unchanged");
        check(!MusaAiAnalysisIntent.isCombinedVisualReview(
            "Çizimi değiştir ve görsel analiz et, keşif oluştur"),
            "Unsafe drawing changes must not enter read-only AI workflow");
        check(!MusaAiAnalysisIntent.isCombinedVisualReview(
            "Raporu PDF olarak çıkar"),"Report export remains separate");
        String activity=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        check(activity.contains("MusaAiAnalysisIntent.isCombinedVisualReview(raw)"),
            "Main dispatcher must explicitly support combined visual and BOQ intent");
        check(activity.contains("runMusaAiWholeSheetVision("),
            "Actual image sweep path must remain routed");
        check(activity.contains("acceptedTiles==total"),
            "No invented 9/9 results");
        System.out.println("MusaAiCombinedVisualTakeoffRouteTest OK");
    }
}
