package com.musa.cad;

import java.util.*;

/** Offline natural-language smart-selection planner for MusaCAD AI. */
public final class MusaAiSmartSelection {
    public static final class Result {
        public final boolean matched;
        public final String description;
        public final int totalMatches;
        public final List<Integer> sourceIds;
        private Result(boolean matched,String description,int totalMatches,List<Integer>sourceIds){
            this.matched=matched;this.description=description;this.totalMatches=totalMatches;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Result none(){return new Result(false,"",0,Collections.emptyList());}
    }

    public static Result plan(MusaAiDrawingIndex index,String raw,String selectedType,String selectedLayer){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||!(q.contains("sec")||q.contains("vurgula")||q.contains("goster")))return Result.none();

        if(q.contains("bununla ayni")||q.contains("ayni olanlar")){
            String type=cleanType(selectedType),layer=selectedLayer==null?"":selectedLayer.trim();
            if(type.isEmpty())return new Result(true,"Önce referans olacak bir nesneyi normal seçim aracıyla seçin.",0,Collections.emptyList());
            return match(index,"Seçili nesneyle aynı tür ve katmandakiler",item->
                type.equals(item.type)&&(layer.isEmpty()||layer.equalsIgnoreCase(item.layer)));
        }

        if(q.contains("ayni tur")||q.contains("bu turdek")){
            String type=cleanType(selectedType);
            if(type.isEmpty())return new Result(true,"Önce referans olacak bir nesneyi normal seçim aracıyla seçin.",0,Collections.emptyList());
            return match(index,type+" türündeki nesneler",item->type.equals(item.type));
        }

        String requestedType=typeFrom(q);
        if(requestedType!=null){
            final String wanted=requestedType;
            return match(index,typeLabel(wanted)+" nesneleri",item->{
                if("POLYLINE_ANY".equals(wanted))return "LWPOLYLINE".equals(item.type)||"POLYLINE".equals(item.type);
                if("TEXT_ANY".equals(wanted))return !item.text.isEmpty();
                return wanted.equals(item.type);
            });
        }

        String layer=extractLayer(q);
        if(layer!=null){
            final String wanted=MusaAiDrawingIndex.normalize(layer);
            return match(index,"“"+layer+"” katmanı",item->MusaAiDrawingIndex.normalize(item.layer).contains(wanted));
        }

        String text=extractText(q);
        if(text!=null){
            final String wanted=MusaAiDrawingIndex.normalize(text);
            return match(index,"“"+text+"” geçen öğeler",item->MusaAiDrawingIndex.normalize(item.text).contains(wanted));
        }

        return Result.none();
    }

    private interface Rule{boolean matches(MusaAiDrawingIndex.Item item);}

    private static Result match(MusaAiDrawingIndex index,String description,Rule rule){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();int total=0;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(!rule.matches(item))continue;
            total++;
            if(item.sourceId>=0)ids.add(item.sourceId);
        }
        return new Result(true,description,total,new ArrayList<>(ids));
    }

    private static String typeFrom(String q){
        if(contains(q,"polyline","poliline","coklu cizgi"))return "POLYLINE_ANY";
        if(contains(q,"daire","cember","circle"))return "CIRCLE";
        if(contains(q,"yay","arc"))return "ARC";
        if(contains(q,"elips","ellipse"))return "ELLIPSE";
        if(contains(q,"hatch","tarama"))return "HATCH";
        if(contains(q,"olculendirme","dimension","olculeri"))return "DIMENSION";
        if(contains(q,"metin","yazi","text","mtext"))return "TEXT_ANY";
        if(contains(q,"xline","sonsuz cizgi"))return "XLINE";
        if(contains(q,"nokta","point"))return "POINT";
        if(contains(q,"cizgi","line"))return "LINE";
        return null;
    }

    private static String extractLayer(String q){
        if(!q.contains("katman")&&!q.contains("layer"))return null;
        String s=q.replace("katmanindakileri"," ").replace("katmanindaki"," ")
            .replace("katmanini"," ").replace("katmani"," ").replace("katman"," ")
            .replace("layerindakileri"," ").replace("layerindaki"," ").replace("layer"," ")
            .replace("tum"," ").replace("hepsini"," ").replace("sec"," ").replace("vurgula"," ").replace("goster"," ");
        s=clean(s);return s.isEmpty()?null:s;
    }

    private static String extractText(String q){
        if(!(q.contains("gecen")||q.contains("yazan")||q.contains("metin")))return null;
        String s=q.replace("gecenleri"," ").replace("gecen"," ").replace("yazanlari"," ").replace("yazan"," ")
            .replace("metinlerde"," ").replace("metin"," ").replace("olanlari"," ")
            .replace("tum"," ").replace("hepsini"," ").replace("sec"," ").replace("vurgula"," ").replace("goster"," ");
        s=clean(s);return s.isEmpty()?null:s;
    }

    private static String cleanType(String s){return s==null?"":s.trim().toUpperCase(Locale.ROOT);}
    private static boolean contains(String q,String...values){for(String v:values)if(q.contains(v))return true;return false;}
    private static String clean(String s){return s.replaceAll("\\b(olan|olanlar|ogeleri|nesneleri|nesne|cizimde|projede|bu)\\b"," ").trim().replaceAll("\\s+"," ");}
    private static String typeLabel(String type){
        switch(type){
            case "POLYLINE_ANY":return "Polyline";case "CIRCLE":return "Daire";case "ARC":return "Yay";
            case "ELLIPSE":return "Elips";case "HATCH":return "Hatch";case "DIMENSION":return "Ölçülendirme";
            case "TEXT_ANY":return "Metin";case "XLINE":return "XLine";case "POINT":return "Nokta";case "LINE":return "Çizgi";
            default:return type;
        }
    }
    private MusaAiSmartSelection(){}
}
