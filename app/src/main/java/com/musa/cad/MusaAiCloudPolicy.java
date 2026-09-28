package com.musa.cad;

/** Explicit opt-in routing rules for MusaCAD cloud AI. */
public final class MusaAiCloudPolicy {
    public static boolean shouldUseCloud(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        if(MusaAiMechanicalExpert.isCloudExpertCommand(raw))return true;
        return q.equals("gandalf")||q.startsWith("gandalf ")||
            q.equals("cloudai")||q.startsWith("cloudai ")||
            q.contains("bulut ai")||q.contains("bulut yapay zeka")||
            q.contains("derin analiz")||q.contains("internetten kontrol")||
            q.contains("webden kontrol")||q.contains("guncel kaynaklarla");
    }

    public static String promptForCloud(String raw){
        if(raw==null)return "";
        String s=raw.trim();
        s=s.replaceFirst("(?i)^\\s*gandalf\\s*[,;:\\-]?\\s*","");
        s=s.replaceFirst("(?i)^\\s*cloudai\\s*[,;:\\-]?\\s*","");
        return s.trim().isEmpty()?"Bu projeyi mühendislik açısından derin analiz et ve raporla.":s.trim();
    }

    public static boolean allowWeb(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.contains("internet")||q.contains("web")||q.contains("guncel")||
            q.contains("kaynak")||q.contains("yonetmelik")||q.contains("standart")||
            q.contains("uretici")||q.contains("katalog");
    }

    public static boolean allowEditProposals(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(MusaAiMechanicalExpert.isCloudExpertCommand(raw))return true;
        return q.contains("duzelt")||q.contains("degistir")||q.contains("ekle")||
            q.contains("sil")||q.contains("tasi")||q.contains("ciz")||
            q.contains("revize")||q.contains("uygula");
    }

    private MusaAiCloudPolicy(){}
}
