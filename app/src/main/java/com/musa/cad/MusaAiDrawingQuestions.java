package com.musa.cad;

import java.util.*;

/** Deterministic offline question-answering over the active drawing index. */
public final class MusaAiDrawingQuestions {
    public static final class Answer {
        public final boolean matched;
        public final String text;
        private Answer(boolean matched,String text){this.matched=matched;this.text=text;}
        public static Answer none(){return new Answer(false,"");}
        public static Answer of(String text){return new Answer(true,text);}
    }

    public static Answer answer(MusaAiDrawingIndex index,String raw){
        if(index==null)return Answer.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return Answer.none();

        if(any(q,"cizimde neler var","cizim ozeti","proje ozeti","hangi nesneler var","nesneler neler")){
            return Answer.of(summary(index));
        }

        if(any(q,"hangi katmanlar var","katmanlari say","katman listesi","layer listesi")){
            return Answer.of("Katmanlar ("+index.allLayers.size()+"):\n"+join(index.allLayers,18));
        }

        if(any(q,"hangi yazilar var","cizimdeki yazilar","metinleri goster","yazilari goster")){
            List<String>samples=index.textSamples(16);
            return Answer.of(samples.isEmpty()
                ?"Bu layoutta okunabilir TEXT/MTEXT içeriği bulunamadı."
                :"Çizimdeki yazılardan örnekler ("+index.textEntityCount()+" metin nesnesi):\n• "+String.join("\n• ",samples));
        }

        String type=askedType(q);
        if(type!=null&&asksCount(q)){
            int n="TEXT_ANY".equals(type)?index.textEntityCount():index.countType(type);
            return Answer.of(typeLabel(type)+" adedi: "+n+" • aktif layout: "+index.layout);
        }

        String layerTerm=extractLayerTerm(q);
        if(layerTerm!=null){
            List<String>layers=index.layersMatching(layerTerm);
            return Answer.of(layers.isEmpty()
                ?"“"+layerTerm+"” ifadesiyle eşleşen katman veya katman üzeri yazı bulunamadı."
                :"“"+layerTerm+"” ile ilişkili katmanlar ("+layers.size()+"):\n• "+String.join("\n• ",limit(layers,18)));
        }

        String occurrence=extractOccurrenceTerm(q);
        if(occurrence!=null){
            int n=index.textOccurrenceCount(occurrence);
            List<String>layers=index.layersMatching(occurrence);
            String extra=layers.isEmpty()?"":"\nİlişkili katmanlar: "+String.join(", ",limit(layers,8));
            return Answer.of("“"+occurrence+"” çizim metinlerinde "+n+" kez geçiyor."+extra+
                "\nNot: Bu sayı metin eşleşmesidir; blok/sembol adedi ayrı nesne sayım modülünde hesaplanır.");
        }

        String presence=extractPresenceTerm(q);
        if(presence!=null){
            boolean yes=index.containsPhrase(presence);
            return Answer.of("“"+presence+"” için "+(yes?"çizimde metin veya katman eşleşmesi bulundu.":"metin/katman eşleşmesi bulunamadı."));
        }

        return Answer.none();
    }

    private static String summary(MusaAiDrawingIndex i){
        ArrayList<Map.Entry<String,Integer>>types=new ArrayList<>(i.typeCounts().entrySet());
        types.sort((a,b)->Integer.compare(b.getValue(),a.getValue()));
        ArrayList<String>top=new ArrayList<>();
        for(int n=0;n<Math.min(8,types.size());n++)top.add(types.get(n).getKey()+": "+types.get(n).getValue());
        return "Çizim özeti\n• Layout: "+i.layout+
            "\n• Nesne: "+i.entityCount+
            "\n• Katman: "+i.allLayers.size()+" (görünür "+i.visibleLayers.size()+")"+
            "\n• Metin nesnesi: "+i.textEntityCount()+
            "\n• OLE: "+i.oleObjectCount+
            (top.isEmpty()?"":"\n• Başlıca türler: "+String.join(" • ",top));
    }

    private static boolean asksCount(String q){
        return q.contains("kac ")||q.endsWith(" kac")||q.contains(" adedi")||q.contains(" sayisi");
    }

    private static String askedType(String q){
        if(containsAny(q,"yazi","metin","text","mtext"))return "TEXT_ANY";
        if(containsAny(q,"polyline","poliline","coklu cizgi"))return "LWPOLYLINE";
        if(containsAny(q,"daire","cember","circle"))return "CIRCLE";
        if(containsAny(q,"yay","arc"))return "ARC";
        if(containsAny(q,"elips","ellipse"))return "ELLIPSE";
        if(containsAny(q,"nokta","point"))return "POINT";
        if(containsAny(q,"hatch","tarama"))return "HATCH";
        if(containsAny(q,"dimension","olcu","olculendirme"))return "DIMENSION";
        if(containsAny(q,"leader","oklu aciklama"))return "MULTILEADER";
        if(containsAny(q,"xline","sonsuz cizgi"))return "XLINE";
        if(containsAny(q,"cizgi","line"))return "LINE";
        return null;
    }

    private static String typeLabel(String type){
        switch(type){
            case "TEXT_ANY":return "Metin";
            case "LINE":return "Çizgi";
            case "LWPOLYLINE":return "Polyline";
            case "CIRCLE":return "Daire";
            case "ARC":return "Yay";
            case "ELLIPSE":return "Elips";
            case "POINT":return "Nokta";
            case "HATCH":return "Hatch";
            case "DIMENSION":return "Ölçülendirme";
            case "MULTILEADER":return "Multileader";
            case "XLINE":return "XLine";
            default:return type;
        }
    }

    private static String extractLayerTerm(String q){
        if(!q.contains("katman")&&!q.contains("layer"))return null;
        String s=q
            .replace("hangi katmanlarda"," ").replace("hangi katmanlar"," ")
            .replace("gecen katmanlari goster"," ").replace("gecen katmanlar"," ")
            .replace("katmanlari goster"," ").replace("katmanlarda"," ")
            .replace("katmanlar"," ").replace("katman"," ")
            .replace("layerlarda"," ").replace("layerlar"," ").replace("layer"," ")
            .replace("goster"," ").replace("bul"," ").replace("geciyor"," ").replace("gecen"," ");
        s=cleanQuery(s);
        return s.isEmpty()?null:s;
    }

    private static String extractOccurrenceTerm(String q){
        if(!q.contains("kac yerde")&&!q.contains("kac kez")&&!q.contains("kelimesi kac")&&!q.contains("ifadesi kac"))return null;
        String s=q.replace("kelimesi kac yerde geciyor"," ").replace("kelimesi kac kez geciyor"," ")
            .replace("ifadesi kac yerde geciyor"," ").replace("ifadesi kac kez geciyor"," ")
            .replace("kac yerde geciyor"," ").replace("kac kez geciyor"," ")
            .replace("kac yerde var"," ").replace("kac kez var"," ");
        s=cleanQuery(s);return s.isEmpty()?null:s;
    }

    private static String extractPresenceTerm(String q){
        if(!(q.endsWith(" var mi")||q.contains(" geciyor mu")||q.contains(" bulunuyor mu")))return null;
        String s=q.replace("cizimde"," ").replace("projede"," ").replace(" var mi"," ")
            .replace(" geciyor mu"," ").replace(" bulunuyor mu"," ");
        s=cleanQuery(s);return s.isEmpty()?null:s;
    }

    private static String cleanQuery(String s){
        return s.replaceAll("\\b(bu|cizimde|projede|olan|ile|ilgili|nerede|nerelerde)\\b"," ")
            .trim().replaceAll("\\s+"," ");
    }

    private static boolean any(String q,String...phrases){for(String p:phrases)if(q.equals(p)||q.contains(p))return true;return false;}
    private static boolean containsAny(String q,String...terms){for(String t:terms)if(q.contains(t))return true;return false;}
    private static List<String>limit(Collection<String>items,int max){ArrayList<String>out=new ArrayList<>();for(String s:items){out.add(s);if(out.size()>=max)break;}return out;}
    private static String join(Collection<String>items,int max){List<String>list=limit(items,max);if(list.isEmpty())return "—";return "• "+String.join("\n• ",list)+(items.size()>list.size()?"\n… +"+(items.size()-list.size())+" katman":"");}
    private MusaAiDrawingQuestions(){}
}
