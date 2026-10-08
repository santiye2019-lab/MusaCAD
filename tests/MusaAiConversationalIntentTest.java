import com.musa.cad.MusaAiConversationalIntent;
import com.musa.cad.MusaAiAnalysisIntent;
public final class MusaAiConversationalIntentTest {
    private static void equal(String a,String b){
        if(!a.equals(b))throw new AssertionError("Expected "+b+" got "+a);
    }
    private static void check(boolean value,String reason){
        if(!value)throw new AssertionError(reason);
    }
    public static void main(String[] args){
        for(String prompt:new String[]{
            "Metraj çıkar","Bana metrajını göster","Projede metraj yapar mısın?",
            "Bu çizimin malzeme miktarlarını bul","Ne kadar malzeme var?"
        })equal(MusaAiConversationalIntent.canonical(prompt),"Bu projede metraj çıkar");
        for(String prompt:new String[]{
            "Keşif hazırla","Projeye keşif oluştur","Keşif çıkarabilir misin",
            "İmalat listesi oluştur"
        })equal(MusaAiConversationalIntent.canonical(prompt),"Projeden keşif oluştur");
        check(MusaAiConversationalIntent.canonical("Bu projede eksik var mı?")
            .contains("mühendislik açısından analiz et"),"Review paraphrase");
        check(MusaAiAnalysisIntent.isReview(
            MusaAiConversationalIntent.canonical("Bu projede eksik var mı?")),"Review must be routed");
        equal(MusaAiConversationalIntent.canonical("Son raporu PDF olarak görüntüle"),
            "Raporu PDF olarak çıkar");
        equal(MusaAiConversationalIntent.canonical("PDF olarak görüntüle"),
            "Raporu PDF olarak çıkar");
        equal(MusaAiConversationalIntent.canonical("Klozeti sil"),"Klozeti sil");
        equal(MusaAiConversationalIntent.canonical("Çizgiyi taşı"),"Çizgiyi taşı");
        equal(MusaAiConversationalIntent.canonical("2025 kitabını yükle"),"2025 kitabını yükle");
        equal(MusaAiConversationalIntent.canonical("2025 2026 yeni pozları tara"),
            "2025 2026 yeni pozları tara");
        equal(MusaAiConversationalIntent.canonical("Pis su borularının uzunluğu kaç metre?"),
            "Pis su borularının uzunluğu kaç metre?");
        equal(MusaAiConversationalIntent.canonical("Projeyi çevrim dışı analiz et"),
            "Projeyi çevrim dışı analiz et");
        check(!MusaAiConversationalIntent.shouldCloudInterpret(
            "Projeyi çevrim dışı analiz et"),"Explicit local only");
        check(!MusaAiConversationalIntent.shouldCloudInterpret(
            "Şu kapıyı sil"),"Ambiguous edits cannot cloud-execute");
        check(MusaAiConversationalIntent.shouldCloudInterpret(
            "Bu tesisatın teknik eksiklerini açıklar mısın?"),"Open question reaches consent gate");
        System.out.println("MusaAiConversationalIntentTest OK");
    }
}
