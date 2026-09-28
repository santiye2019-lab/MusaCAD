package com.musa.cad;

import java.util.*;

/** Builds a deterministic offline project report from MusaCAD AI analysis modules. */
public final class MusaAiAutoReport {
    public static final class Result {
        public final boolean matched;
        public final String text;
        public final List<Integer> sourceIds;
        public final int findingCount;
        private Result(boolean matched,String text,Collection<Integer>sourceIds,int findingCount){
            this.matched=matched;this.text=text==null?"":text;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
            this.findingCount=Math.max(0,findingCount);
        }
        public static Result none(){return new Result(false,"",Collections.emptyList(),0);}
    }

    public static boolean asksReport(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        return q.contains("proje raporu")||q.contains("ai raporu")||q.contains("teknik rapor")||
            q.contains("rapor olustur")||q.contains("raporu olustur")||q.contains("rapor hazirla")||
            q.contains("raporu hazirla")||q.equals("rapor")||q.equals("raporu");
    }

    public static Result generate(MusaAiDrawingIndex current,String currentName,
                                  MusaAiDrawingIndex baseline,String baselineName){
        if(current==null)return Result.none();
        LinkedHashSet<Integer>highlight=new LinkedHashSet<>();
        StringBuilder out=new StringBuilder();
        out.append("MUSACAD AI • OTOMATİK PROJE RAPORU");
        out.append("\n================================");
        out.append("\nDosya: ").append(clean(currentName,"Aktif çizim"));
        out.append("\nLayout: ").append(clean(current.layout,"aktif layout"));
        out.append("\nÇizim birimi: ").append(current.unitName.isEmpty()?"belirsiz":current.unitName);
        out.append("\nToplam nesne: ").append(current.entityCount);
        out.append("\nAI indekslenen görünür nesne: ").append(current.items().size());
        out.append("\nKatman: ").append(current.allLayers.size())
           .append(" • görünür: ").append(current.visibleLayers.size());
        out.append("\nMetin içeren nesne: ").append(current.textEntityCount());
        out.append("\nOLE / gömülü belge: ").append(current.oleObjectCount)
           .append(" • önizlemeli: ").append(current.olePreviewCount);

        appendTypeSummary(out,current.typeCounts());

        MusaAiQuantityTakeoff.Answer takeoff=MusaAiQuantityTakeoff.answer(current,"Bu projede metraj çıkar");
        if(takeoff.matched&&!takeoff.text.isEmpty()){
            out.append("\n\n1. METRAJ ÖZETİ\n").append(stripAdvice(takeoff.text));
        }

        MusaAiProjectControl.Result control=MusaAiProjectControl.analyze(current,"Projeyi kontrol et");
        int findingCount=0;
        if(control.matched){
            findingCount=control.findingCount;
            highlight.addAll(control.sourceIds);
            out.append("\n\n2. CAD KALİTE KONTROLÜ\n").append(stripFinalNote(control.text));
        }

        if(current.oleObjectCount>0){
            MusaAiTableOleAnalysis.Answer ole=MusaAiTableOleAnalysis.answer(current,"OLE nesnelerini özetle");
            if(ole.matched&&!ole.text.isEmpty()){
                out.append("\n\n3. GÖMÜLÜ BELGE / OLE\n").append(limitLines(ole.text,18));
            }
        }else{
            out.append("\n\n3. GÖMÜLÜ BELGE / OLE\n• Görünür OLE2FRAME nesnesi yok.");
        }

        out.append("\n\n4. REVİZYON DURUMU");
        if(baseline!=null){
            MusaAiRevisionCompare.Result revision=MusaAiRevisionCompare.compare(
                baseline,current,"Revizyonları karşılaştır",
                clean(baselineName,"Referans"),clean(currentName,"Güncel"));
            if(revision.matched){
                highlight.addAll(revision.sourceIds);
                out.append("\n").append(stripFinalNote(revision.text));
            }else out.append("\n• Revizyon karşılaştırması üretilemedi.");
        }else{
            out.append("\n• Referans revizyon tanımlı değil. Bu bölüm karşılaştırma yapılmadan üretildi.");
        }

        out.append("\n\n5. RAPOR SONUCU");
        out.append("\n• Otomatik CAD kalite bulgusu/inceleme adayı: ").append(findingCount);
        out.append("\n• Çizimde vurgulanabilir bulgu/değişiklik: ").append(highlight.size());
        out.append("\n• Rapor MusaCAD içindeki vektör çizim verisi ve gömülü metadata üzerinden çevrimdışı oluşturuldu.");
        out.append("\n• Bu rapor mühendislik hesabı, yönetmelik uygunluk belgesi veya resmi proje onayı yerine geçmez.");

        return new Result(true,out.toString(),highlight,findingCount);
    }

    private static void appendTypeSummary(StringBuilder out,Map<String,Integer>counts){
        if(counts==null||counts.isEmpty())return;
        ArrayList<Map.Entry<String,Integer>>entries=new ArrayList<>(counts.entrySet());
        entries.sort((a,b)->{
            int byCount=Integer.compare(b.getValue(),a.getValue());
            return byCount!=0?byCount:a.getKey().compareTo(b.getKey());
        });
        ArrayList<String>parts=new ArrayList<>();
        for(Map.Entry<String,Integer>e:entries){
            parts.add(e.getKey()+" "+e.getValue());
            if(parts.size()>=8)break;
        }
        if(!parts.isEmpty())out.append("\nBaşlıca nesne türleri: ").append(String.join(" • ",parts));
    }

    private static String stripAdvice(String text){
        if(text==null)return "";
        int at=text.indexOf("\nBelirli bir katman/tür için");
        return at>=0?text.substring(0,at).trim():text.trim();
    }
    private static String stripFinalNote(String text){
        if(text==null)return "";
        int at=text.lastIndexOf("\nNot:");
        return at>=0?text.substring(0,at).trim():text.trim();
    }
    private static String limitLines(String text,int max){
        if(text==null)return "";
        String[]lines=text.replace('\r','\n').split("\n");
        StringBuilder b=new StringBuilder();
        for(String line:lines){
            if(line.trim().isEmpty())continue;
            if(b.length()>0)b.append('\n');
            b.append(line);
            if(--max<=0){b.append("\n…");break;}
        }
        return b.toString();
    }
    private static String clean(String value,String fallback){
        return value==null||value.trim().isEmpty()?fallback:value.trim();
    }
    private MusaAiAutoReport(){}
}
