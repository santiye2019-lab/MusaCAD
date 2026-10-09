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
        scope("Şu paftaya bir bakabilir misin?","all");
        scope("Bu DWG dosyasını baştan sona değerlendir","all");
        scope("Bütün paftaları gözden geçir","all");
        scope("Kesitler ve kat planları uyumlu mu?","all");
        scope("Buradaki teknik sorunları bul","all");
        scope("Çizimde hata var mı?","all");
        scope("Tesisattaki eksikleri bul","mechanical");
        scope("Mutfak havalandırmasında sorun var mı?","ventilation");
        scope("Bu projedeki yanlışlar neler?","all");
        if(!MusaAiAnalysisIntent.wantsAllViews("Tüm paftaları incele"))
            throw new AssertionError("all-pafta request scope missed");
        if(!MusaAiAnalysisIntent.wantsAllViews("Bu dosyayı baştan sona gözden geçir"))
            throw new AssertionError("whole DWG request scope missed");
        if(MusaAiAnalysisIntent.wantsAllViews("Sadece pis su paftasını incele"))
            throw new AssertionError("single pafta must not count as all views");
        if(MusaAiAnalysisIntent.isReview("Bu çerçeveyi değiştir ve çizimi incele"))
            throw new AssertionError("explicit edit cannot become read-only inspection");
        if(MusaAiAnalysisIntent.isReview("Poz 25.100.1005 fiyatını ara"))
            throw new AssertionError("catalog lookup cannot become visual review");
        if(MusaAiAnalysisIntent.isReview("Merhaba, nasılsın?"))
            throw new AssertionError("casual chat cannot trigger CAD review");

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
