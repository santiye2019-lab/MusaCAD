import com.musa.cad.MusaAiAnalysisIntent;
import com.musa.cad.MusaAiConversationalIntent;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Regression: full Turkish Qwen→local→PDF workflow never becomes text-only. */
public final class MusaAiVisionFirstRoutingTest {
    private static void require(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    private static void contains(String text,String term){
        require(text.contains(term),"Missing safety/route assertion: "+term);
    }
    public static void main(String[] args)throws Exception {
        String full="Açık Silivrikapı Spor Köyü Kafe–Mutfak DWG projesini kapsamlı "+
            "mühendislik denetimine al.\n"+
            "1. Önce Qwen çevrimiçi görsel yapay zekâ motoruyla çizimi 3×3 "+
            "düzeninde 9 bölge olarak incele.\n"+
            "2. Tesisat güzergâhlarını, vanaları, kotları ve kesitleri görsel "+
            "olarak değerlendir.\n"+
            "3. Çevrimiçi görsel inceleme bittikten sonra telefondaki yerel "+
            "DWG analiz motorunu çalıştır.\n"+
            "4. Doğrulanan ölçülerden ÇŞİDB poz keşif ön kontrolü yap.\n"+
            "5. Her bölgeyi kaynaklarıyla raporla; DWG dosyasında değişiklik yapma.\n"+
            "Raporu Word ve PDF olarak dışa aktarmaya hazırla.";
        require(MusaAiAnalysisIntent.isVisionFirstReview(full),
            "Full user request must select Qwen visual evidence first");
        require(MusaAiConversationalIntent.isPdfRequest(full),
            "Confirm downstream PDF output would otherwise hijack route");
        require(full.equals(MusaAiConversationalIntent.canonical(full)),
            "PDF output is downstream, not a replacement for the 9 images");
        require(MusaAiAnalysisIntent.isLocalOnly(full),
            "Confirm local secondary step could otherwise hijack route");
        require(MusaAiAnalysisIntent.isVisionFirstReview(
            "Açık DWG projesini 9 bölgede Qwen ile analiz et, yerel sonra"),
            "Short Qwen visual route must work");
        require(!MusaAiAnalysisIntent.isVisionFirstReview(
            "Raporu PDF olarak çıkar"),"PDF-only export must stay export");
        require(!MusaAiAnalysisIntent.isVisionFirstReview(
            "Projeden keşif oluştur"),"Takeoff-only must stay local");
        require(!MusaAiAnalysisIntent.isVisionFirstReview(
            "Qwen kullanma. Bu DWG projesini yerel analiz et"),
            "Explicit no-Qwen request must stay off cloud");
        require(!MusaAiAnalysisIntent.isVisionFirstReview(
            "Açık DWG'yi Qwen ile analiz et, çizimi sil ve revizyon uygula"),
            "Unsafe CAD edits never auto-route to read-only review");
        String main=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        int handler=main.indexOf("private void handleMusaAiPrompt(");
        int priority=main.indexOf("if(MusaAiAnalysisIntent.isVisionFirstReview(prompt))",handler);
        int generic=main.indexOf("String aiControl=",handler);
        require(handler>=0&&priority>handler&&generic>priority,
            "Visual dispatch must precede local/composite generic commands");
        contains(main,"handleMusaAiCloudPrompt(prompt.trim(),reply,recentContext);");
        contains(main,"boolean packageMode=!visionFirst&&");
        contains(main,"boolean hybridVisual=(visionFirst||");
        contains(main,"if(MusaAiAnalysisIntent.isVisionFirstReview(raw)){");
        contains(main,"GÖRSEL DENETİM BAŞARISIZ — 0/9");
        contains(main,"MusaAiEngineeringSynthesis.compose(synthesis)");
        contains(main,"if(acceptedBatches>0)");
        contains(main,"runMusaAiWholeSheetVision(");
        require(main.indexOf("GÖRSEL DENETİM BAŞARISIZ — 0/9")<
                main.indexOf("Gandalf yerel vektör analiziyle devam etti"),
            "Fail-closed vision branch must precede generic local fallback");
        System.out.println("MusaAiVisionFirstRoutingTest OK");
    }
}
