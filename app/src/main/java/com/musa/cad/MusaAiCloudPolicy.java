package com.musa.cad;

/** Explicit opt-in routing rules for MusaCAD cloud AI. */
public final class MusaAiCloudPolicy {
    public static boolean shouldUseCloud(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        if(MusaAiMechanicalExpert.isCloudExpertCommand(raw)||MusaAiDisciplineExpert.isCloudExpertCommand(raw))return true;
        // "Gandalf" is the central assistant identity, not a synonym for cloud.
        // Ordinary Gandalf voice/text commands should first use local CAD tools.
        // Cloud is reserved for explicit deep/web/current-source intent.
        return q.equals("cloudai")||q.startsWith("cloudai ")||
            q.contains("bulut ai")||q.contains("bulut yapay zeka")||
            q.contains("derin analiz")||q.contains("internetten kontrol")||
            q.contains("webden kontrol")||q.contains("guncel kaynaklarla");
    }

    public static boolean shouldUseProjectPackage(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.contains("proje paketi")||
            q.contains("tum disiplin")||
            q.contains("tum acik proje")||
            q.contains("tum acik cizim")||
            q.contains("disiplinler arasi");
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
        if(MusaAiMechanicalExpert.isCloudExpertCommand(raw)||MusaAiDisciplineExpert.isCloudExpertCommand(raw))return true;
        return q.contains("duzelt")||q.contains("degistir")||q.contains("ekle")||
            q.contains("sil")||q.contains("tasi")||q.contains("ciz")||
            q.contains("revize")||q.contains("uygula");
    }

    private MusaAiCloudPolicy(){}
}
