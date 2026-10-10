package com.musa.cad;

/**
 * Voice/text interpretation for automatic multimodal engineering review.
 * Specialist/edit/report export commands retain their existing routes.
 * A discipline scope is a model instruction, not a claim that other
 * visible drawing elements were omitted or rendered invisible.
 */
public final class MusaAiAnalysisIntent {
    private MusaAiAnalysisIntent(){}


    /**
     * "Keşif oluşturma" and "metraj çıkarma" can be negative imperatives.
     * Never interpret those words as authorization to generate a BOQ.
     * Includes explicit "don't" phrases; deliberately conservative for
     * ambiguous Turkish suffixes, since takeoff is not an edit-free chat.
     */
    public static boolean prohibitsTakeoff(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"kesif olusturma","kesif hazirlama","kesif cikarma",
            "kesif yapma","metraj cikarma","metraj cikartma","metraj yapma",
            "metraj hesaplama","kesif istemiyorum","metraj istemiyorum",
            "kesif gerek yok","metraj gerek yok","kesif uretme",
            "metraj uretme","metraj hazirlama");
    }

    private static String withoutTakeoffProhibitions(String q){
        if(!prohibitsTakeoff(q))return q;
        for(String negation:new String[]{"kesif olusturma","kesif hazirlama",
            "kesif cikarma","kesif yapma","metraj cikarma","metraj cikartma",
            "metraj yapma","metraj hesaplama","kesif istemiyorum",
            "metraj istemiyorum","kesif gerek yok","metraj gerek yok",
            "kesif uretme","metraj uretme","metraj hazirlama"})
            q=q.replace(negation," ");
        return q.replaceAll("\\s+"," ").trim();
    }

    public static boolean isReview(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        // Strip only negative takeoff instructions for review detection.
        // "Keşif oluşturma" must not match the positive "keşif oluştur" veto.
        String forReview=withoutTakeoffProhibitions(q);
        if(q.startsWith("mekai ")||q.startsWith("gmekai ")||
           q.startsWith("statikai ")||q.startsWith("gstatikai ")||
           q.startsWith("elkai ")||q.startsWith("gelkai "))return false;
        if(contains(forReview,"onerileri uygula","onerileri onizle","degistir","duzelt",
            "dosya kaydet","metraj cikar","kesif olustur","kesif yukle",
            "raporu pdf","raporu word","revizyon","secili alani")||
            (" "+forReview+" ").contains(" sil "))return false;
        // Natural Turkish review language: keep the entire original prompt for
        // the model. This local check chooses a READ-ONLY visual workflow only.
        // It must not interpret ambiguous natural language as a CAD edit.
        boolean operation=contains(q,"analiz","incele","denetle","kontrol et","proje kontrolu",
            "proje incele","teknik rapor hazirla","gozden gecir","bir bak",
            "bakabilir misin","bakar misin","degerlendir","tarama yap","taramak",
            "bastan sona bak","eksikleri bul","hatalari bul","sorunlari bul",
            "uygun mu","uyumlu mu","sorun var mi","eksik var mi","hata var mi","ne dersin",
            "neler yanlis","yanlislar","teknik sorun","tum paftalari tara","butun paftalari tara");
        boolean topic=contains(q,"proje","cizim","pafta","tesisat","mekanik","sihhi",
            "pis su","atik su","yangin","havalandirma","klima","isitma","statik",
            "mimari","elektrik","peyzaj","altyapi","asansor","kanal","boru",
            "hidrofor","pompa","sprinkler","hepsini","kesit","vaziyet",
            "kot","kat plani","cati plani","bodrum","zemin kat","gorunus",
            "katlar","kolon","dosya","planlar","burada","burayi","bunda","bunlari");
        return operation&&topic;
    }

    /**
     * A single compound instruction may request both real visual CAD review
     * and a read-only metraj/BOQ appendix. The original prompt must be kept:
     * converting it to "Projeden keşif oluştur" would bypass the 3x3 AI sweep.
     * Standalone takeoff commands are deliberately not matched here.
     */
    public static boolean isCombinedVisualReview(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        boolean explicitVisual=contains(q,"gorsel analiz","gorsel incele",
            "gorsel ve sayisal","9 bolge","9 gorsel bolge",
            "3x3 tarama","3x3 gorunum","3 x 3 tarama");
        boolean review=contains(q,"analiz","incele","denetle","kontrol et",
            "tara","tarama","degerlendir");
        boolean drawing=contains(q,"proje","dwg","cizim","pafta","tesisat",
            "mekanik");
        boolean quantities=contains(q,"kesif","metraj","poz","malzeme listesi",
            "maliyet");
        if(!explicitVisual||!review||!drawing||!quantities)return false;
        // Never reinterpret a drawing edit as a read-only inspection.
        return !contains(q,"onerileri uygula","nesneyi sil","cizimi sil",
            "cizimi degistir","cizimi duzelt","duzeltmeleri uygula",
            "dosya kaydet","revizyon uygula","secileni sil");
    }

    /** A request for all views in the drawing, not proof that every frame is known. */
    public static boolean wantsAllViews(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"tum pafta","butun pafta","paftalarin hepsi",
            "paftalarin tamami","tum katlar","butun katlar","tum kesitler",
            "bastan sona","projenin tamami","projenin hepsi","her pafta");
    }

    /** Keep explicit offline/yerel requests on-device; never ask for cloud consent. */
    public static boolean isLocalOnly(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"yerel","cevrimdisi","cevrim disi","internetsiz",
            "offline","internet kullanma","buluta gonderme","sunucu kullanma");
    }

    public static String scope(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(contains(q,"pis su","atik su","kanalizasyon","wastewater","sewer"))return "wastewater";
        if(contains(q,"sihhi tesisat","temiz su","sicak su","soguk su","kullanma suyu"))return "sanitary";
        if(contains(q,"yangin","sprinkler","hidrant","fire"))return "fire";
        if(contains(q,"havalandirma","hava kanali","ventilasyon","spiro","egzoz"))return "ventilation";
        if(contains(q,"kalorifer","isitma","radyator","yerden isitma"))return "heating";
        if(contains(q,"sogutma","klima","vrf","chiller","fancoil"))return "cooling";
        if(contains(q,"dogalgaz","gaz tesisati"))return "gas";
        if(contains(q,"mekanik","tesisat","boru","pompa","hidrofor"))return "mechanical";
        if(contains(q,"statik","betonarme","tasiyici"))return "structural";
        if(contains(q,"mimari","mimarilik"))return "architectural";
        if(contains(q,"elektrik","aydinlatma","pano"))return "electrical";
        if(contains(q,"peyzaj","bitkilendirme"))return "landscape";
        if(contains(q,"altyapi","rogar","sulama altyapisi"))return "infrastructure";
        if(contains(q,"asansor","elevator"))return "elevator";
        return "all";
    }

    public static String label(String scope){
        if(scope==null)return "Tüm disiplinler";
        switch(scope){
            case "wastewater":return "Pis su / atık su";
            case "sanitary":return "Sıhhi tesisat";
            case "fire":return "Yangın tesisatı";
            case "ventilation":return "Havalandırma";
            case "heating":return "Isıtma";
            case "cooling":return "Klima / soğutma";
            case "gas":return "Doğalgaz";
            case "mechanical":return "Mekanik tesisat";
            case "structural":return "Statik";
            case "architectural":return "Mimari";
            case "electrical":return "Elektrik";
            case "landscape":return "Peyzaj";
            case "infrastructure":return "Altyapı";
            case "elevator":return "Asansör";
            default:return "Tüm disiplinler";
        }
    }

    private static boolean contains(String hay,String... needles){
        for(String n:needles)if(hay.contains(MusaAiDrawingIndex.normalize(n)))return true;
        return false;
    }
}
