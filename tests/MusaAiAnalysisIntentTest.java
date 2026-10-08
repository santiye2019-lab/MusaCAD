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
        if(MusaAiAnalysisIntent.isReview("Önerileri uygula"))
            throw new AssertionError("Unapproved edit route hijacked");
        if(MusaAiAnalysisIntent.isReview("Raporu PDF olarak çıkar"))
            throw new AssertionError("Export route hijacked");
        if(MusaAiAnalysisIntent.isReview("Projedeki pis su metrajını çıkar"))
            throw new AssertionError("Takeoff route hijacked");
        if(MusaAiAnalysisIntent.isReview("GMEKAI_FIRE kontrol et"))
            throw new AssertionError("Expert route hijacked");
        System.out.println("MusaAiAnalysisIntentTest OK");
    }
}
