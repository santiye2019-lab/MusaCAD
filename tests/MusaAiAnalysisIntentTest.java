import com.musa.cad.MusaAiAnalysisIntent;

public final class MusaAiAnalysisIntentTest {
    private static void scope(String prompt,String scope){
        if(!MusaAiAnalysisIntent.isReview(prompt))
            throw new AssertionError("Review not recognized: "+prompt);
        String result=MusaAiAnalysisIntent.scope(prompt);
        if(!scope.equals(result))throw new AssertionError(prompt+" => "+result+" expected "+scope);
    }
    public static void main(String[]args){
        scope("Gandalf, projeyi analiz et","all");
        scope("Bu paftayı tüm disiplinlerde incele","all");
        scope("Mekanik olarak analiz et","mechanical");
        scope("Sıhhi tesisat olarak analiz et","sanitary");
        scope("Pis su tesisatını kontrol et","wastewater");
        scope("Yangın projesini incele","fire");
        scope("Havalandırma kanalını analiz et","ventilation");
        scope("Kalorifer tesisatını analiz et","heating");
        scope("Klima projesini kontrol et","cooling");
        scope("Statik olarak analiz et","structural");
        scope("Mimari proje analizi yap","architectural");
        scope("Elektrik projesini denetle","electrical");
        scope("Asansör projesini analiz et","elevator");
        scope("Kesitleri kontrol et","all");
        scope("Vaziyet planını incele","all");
        scope("Kotları denetle","all");
        scope("Çatı planını analiz et","all");
        scope("Bodrum katı incele","all");
        if(MusaAiAnalysisIntent.isReview("Önerileri uygula"))
            throw new AssertionError("Unapproved edit route hijacked");
        if(MusaAiAnalysisIntent.isReview("Raporu PDF olarak çıkar"))
            throw new AssertionError("Export route hijacked");
        if(MusaAiAnalysisIntent.isReview("Projedeki pis su metrajını çıkar"))
            throw new AssertionError("Takeoff route hijacked");
        if(MusaAiAnalysisIntent.isReview("GMEKAI_FIRE kontrol et"))
            throw new AssertionError("Expert route hijacked");
        if(!MusaAiAnalysisIntent.isReview("Silivrikapı Spor Köyü projesini analiz et"))
            throw new AssertionError("Place name must not trigger destructive sil command");
        if(!MusaAiAnalysisIntent.isLocalOnly("Projeyi çevrim dışı analiz et"))
            throw new AssertionError("Offline Turkish command not recognized");
        if(!MusaAiAnalysisIntent.isLocalOnly("Projeyi yerel olarak incele"))
            throw new AssertionError("Local Turkish command not recognized");
        if(!MusaAiAnalysisIntent.isLocalOnly("Projeyi analiz et internet kullanma"))
            throw new AssertionError("Explicit no-internet intent ignored");
        if(MusaAiAnalysisIntent.isLocalOnly("Projeyi mühendislik açısından analiz et"))
            throw new AssertionError("Regular hybrid request mistaken for local");
        if(MusaAiAnalysisIntent.isReview("Bu çizgiyi sil"))
            throw new AssertionError("Direct edit/delete intent misrouted");
        System.out.println("MusaAiAnalysisIntentTest OK");
    }
}
