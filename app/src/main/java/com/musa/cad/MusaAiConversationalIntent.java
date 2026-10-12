package com.musa.cad;

/**
 * Conservative local paraphrase bridge. No approximate spelling or model
 * prediction may authorize edits, overwrite files or commit a unit price.
 * Open-ended questions that local engines cannot answer are offered to the
 * consent-gated cloud interpreter rather than answered with a canned failure.
 */
public final class MusaAiConversationalIntent {
    public static String canonical(String raw){
        if(raw==null)return "";
        String original=raw.trim();
        String q=MusaAiDrawingIndex.normalize(original);
        if(q.isEmpty()||MusaAiAnalysisIntent.isLocalOnly(original))return original;
        // Preserve the full engineering instruction when report export,
        // offline CAD verification, and visual Qwen review appear together.
        // PDF formatting is a downstream output request, not a replacement
        // for the requested 9-region image review.
        if(MusaAiAnalysisIntent.isVisionFirstReview(original))return original;
        if(isPdfRequest(original))return "Raporu PDF olarak çıkar";
        // Negated quantity commands are never executable BOQ requests.
        // This must precede substring matching against "keşif oluştur".
        if(MusaAiAnalysisIntent.prohibitsTakeoff(original))return original;
        // Keep full instructions for a hybrid visual CAD review WITH a BOQ.
        // Otherwise the following generic BOQ shortcut erases all 9 visual
        // regions and turns a requested review into an unrelated local report.
        if(MusaAiAnalysisIntent.isCombinedVisualReview(original)||
           (MusaAiAnalysisIntent.isReview(original)&&isGenericBoq(q)))
            return original;
        // Standalone document/BOQ commands still use the established route.
        if(isGenericBoq(q))return "Projeden keşif oluştur";
        if(editLike(q))return original;
        // Keep the user's discipline and detailed measurements intact.
        if(isGenericTakeoff(q))return "Bu projede metraj çıkar";
        if(isGenericReview(q))return original+"; projeyi mühendislik açısından analiz et";
        return original;
    }
    public static boolean isPdfRequest(String text){
        String q=MusaAiDrawingIndex.normalize(text);
        return (q.contains("pdf")&&(
            q.contains("goruntule")||q.contains("ac")||q.contains("goster")||
            q.contains("cikar")||q.contains("aktar")||q.contains("kaydet")))
            &&(q.contains("rapor")||q.contains("yanit")||q.contains("sonuc")||
                q.equals("pdf olarak goruntule")||q.equals("pdf ac")||
                q.equals("pdf goster"));
    }
    /**
     * Definition/explanation requests are natural chat, not a DWG inspection.
     * Keep project-relative engineering questions on their original CAD route.
     * This applies equally to keyboard and Android speech recognition.
     */
    public static boolean isInformationalQuestion(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||MusaAiAnalysisIntent.isLocalOnly(raw))return false;
        if(q.contains("bu projede")||q.contains("bu cizimde")||
           q.contains("bu paftada")||q.contains("acik projede")||
           q.contains("acik cizimde")||q.contains("cizimdeki")||
           q.contains("paftadaki")||q.contains("projedeki")||
           q.contains("projemde")||q.contains("dwg dosyasinda")||
           q.contains("cad dosyasinda"))return false;
        if(MusaAiAnalysisIntent.isVisionFirstReview(raw)||
           MusaAiAnalysisIntent.isCombinedVisualReview(raw))return false;
        if(editLike(q)||isGenericBoq(q)||isGenericTakeoff(q))return false;
        boolean asksDefinition=q.endsWith(" nedir")||q.equals("nedir")||
            q.endsWith(" ne demek")||q.contains(" ne anlama gelir")||
            q.endsWith(" ne ise yarar")||q.endsWith(" ne ise yariyor")||
            q.endsWith(" nasil yapilir")||q.endsWith(" nasil calisir")||
            q.endsWith(" neden olur")||q.endsWith(" aciklar misin")||
            q.endsWith(" anlatir misin")||q.endsWith(" acikla");
        return asksDefinition;
    }
    public static boolean shouldCloudInterpret(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||MusaAiAnalysisIntent.isLocalOnly(raw))return false;
        // Cloud only supplies reasoning, never directly performs ambiguous edits.
        if(editLike(q))return false;
        return q.length()>=3;
    }
    private static boolean isGenericTakeoff(String q){
        if(q.contains("kesif olustur")||q.contains("kesif hazirla")||
           q.contains("kesif yukle")||q.contains("kesif karsilastir")||
           q.contains("poz")||q.contains("rapor"))return false;
        if(q.contains("kac tane")||q.contains("kac adet")||
           q.contains("kac metre")||q.contains("uzunlugu")||
           q.contains("cap")||q.contains("sadece")||
           q.contains("pis su")||q.contains("temiz su"))return false;
        return q.contains("metraj")||q.contains("malzeme miktarlari")||
            q.contains("malzeme miktarlarini")||q.contains("malzeme sayimi")||
            q.contains("ne kadar malzeme var");
    }
    private static boolean isGenericBoq(String q){
        if(q.contains("kesif yukle")||q.contains("kesif karsilastir")||
            q.contains("kesif farki"))return false;
        return (q.contains("kesif")&&
            (q.contains("hazirla")||q.contains("olustur")||q.contains("cikar")||
             q.contains("yap")))||
            q.contains("imalat listesi olustur");
    }
    private static boolean isGenericReview(String q){
        boolean topic=q.contains("proje")||q.contains("cizim")||q.contains("pafta")||
            q.contains("tesisat")||q.contains("burada")||q.contains("bunda");
        boolean question=q.contains("eksik var")||q.contains("hata var")||
            q.contains("sorun var")||q.contains("uygun mu")||
            q.contains("sikinti var")||q.contains("neresi hatali")||
            q.contains("nerelerde hata")||q.contains("gozden gecir")||
            q.contains("ne dusunuyorsun");
        return topic&&question;
    }
    private static boolean editLike(String q){
        String padded=" "+q+" ";
        for(String token:new String[]{" sil "," kaldir "," degistir "," duzelt ",
            " uygula "," tas i "," tasi "," kaydet "," kopyala "," kes ",
            " olustur "," ekle "," ciz "," geri al "," yeniden yap "})
            if(padded.contains(token))return true;
        return false;
    }
    private MusaAiConversationalIntent(){}
}
