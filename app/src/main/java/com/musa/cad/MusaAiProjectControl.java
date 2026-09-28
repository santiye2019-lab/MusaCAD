package com.musa.cad;

import java.util.*;

/** Deterministic offline CAD-quality checks for the active MusaCAD drawing. */
public final class MusaAiProjectControl {
    public static final class Result {
        public final boolean matched;
        public final String text;
        public final int findingCount;
        public final List<Integer> sourceIds;
        private Result(boolean matched,String text,int findingCount,Collection<Integer>sourceIds){
            this.matched=matched;this.text=text;this.findingCount=Math.max(0,findingCount);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Result none(){return new Result(false,"",0,Collections.emptyList());}
    }

    private enum Mode { FULL,DUPLICATE,DEGENERATE,EMPTY_TEXT,OPEN_POLYLINE,LAYER_ZERO,REVIEW_MARKER }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        Mode mode=mode(q);
        if(mode==null)return Result.none();

        LinkedHashMap<String,ArrayList<MusaAiDrawingIndex.Item>> geometry=new LinkedHashMap<>();
        ArrayList<MusaAiDrawingIndex.Item> degenerate=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item> emptyText=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item> openPolyline=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item> layerZero=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item> reviewMarker=new ArrayList<>();

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            if(!item.geometryKey.isEmpty()){
                String key=MusaAiDrawingIndex.normalize(item.layer)+"|"+item.geometryKey;
                geometry.computeIfAbsent(key,k->new ArrayList<>()).add(item);
            }
            if(isDegenerate(item))degenerate.add(item);
            if(isTextType(item.type)&&item.text.trim().isEmpty())emptyText.add(item);
            if(isPolyline(item.type)&&item.closedKnown&&!item.closed)openPolyline.add(item);
            if("0".equals(MusaAiDrawingIndex.normalize(item.layer)))layerZero.add(item);
            if(hasReviewMarker(item.text))reviewMarker.add(item);
        }

        ArrayList<ArrayList<MusaAiDrawingIndex.Item>> duplicateGroups=new ArrayList<>();
        int duplicateEntities=0;
        for(ArrayList<MusaAiDrawingIndex.Item> group:geometry.values()){
            if(group.size()<2)continue;
            duplicateGroups.add(group);duplicateEntities+=group.size();
        }

        int hiddenLayers=0;
        for(String layer:index.allLayers)if(!containsIgnoreCase(index.visibleLayers,layer))hiddenLayers++;

        LinkedHashSet<Integer>highlight=new LinkedHashSet<>();
        int findingCount=0;
        switch(mode){
            case DUPLICATE:
                addDuplicateIds(highlight,duplicateGroups);findingCount=duplicateEntities;
                return result(index,"Mükerrer geometri kontrolü",
                    "Birebir mükerrer geometri: "+duplicateGroups.size()+" grup / "+duplicateEntities+" nesne",
                    findingCount,highlight);
            case DEGENERATE:
                addIds(highlight,degenerate);findingCount=degenerate.size();
                return result(index,"Geometri bütünlük kontrolü",
                    "Sıfır uzunluk/dejenere geometri: "+degenerate.size(),
                    findingCount,highlight);
            case EMPTY_TEXT:
                addIds(highlight,emptyText);findingCount=emptyText.size();
                return result(index,"Metin bütünlük kontrolü",
                    "Boş TEXT/MTEXT/ATTRIB: "+emptyText.size(),
                    findingCount,highlight);
            case OPEN_POLYLINE:
                addIds(highlight,openPolyline);findingCount=openPolyline.size();
                return result(index,"Polyline kontrolü",
                    "Açık polyline: "+openPolyline.size()+" (tasarıma göre normal olabilir)",
                    findingCount,highlight);
            case LAYER_ZERO:
                addIds(highlight,layerZero);findingCount=layerZero.size();
                return result(index,"Katman kontrolü",
                    "0 katmanındaki görünür nesne: "+layerZero.size()+" (blok içeriğinde normal olabilir)",
                    findingCount,highlight);
            case REVIEW_MARKER:
                addIds(highlight,reviewMarker);findingCount=reviewMarker.size();
                return result(index,"Not/revizyon kontrolü",
                    "TODO / TBD / EKSİK / REVİZE / DÜZELT benzeri not: "+reviewMarker.size(),
                    findingCount,highlight);
            default:break;
        }

        addDuplicateIds(highlight,duplicateGroups);
        addIds(highlight,degenerate);
        addIds(highlight,emptyText);
        addIds(highlight,openPolyline);
        addIds(highlight,layerZero);
        addIds(highlight,reviewMarker);

        int directFindings=duplicateEntities+degenerate.size()+emptyText.size();
        int reviewFindings=openPolyline.size()+layerZero.size()+reviewMarker.size();
        findingCount=directFindings+reviewFindings;

        StringBuilder out=new StringBuilder();
        out.append("AI CAD kalite kontrolü • ").append(index.layout.isEmpty()?"aktif layout":index.layout);
        out.append("\nBulgular");
        out.append("\n• Mükerrer geometri: ").append(duplicateGroups.size()).append(" grup / ").append(duplicateEntities).append(" nesne");
        out.append("\n• Sıfır uzunluk/dejenere geometri: ").append(degenerate.size());
        out.append("\n• Boş TEXT/MTEXT/ATTRIB: ").append(emptyText.size());
        out.append("\nİnceleme adayları");
        out.append("\n• Açık polyline: ").append(openPolyline.size());
        out.append("\n• 0 katmanındaki görünür nesne: ").append(layerZero.size());
        out.append("\n• Revizyon/kontrol notu: ").append(reviewMarker.size());
        out.append("\n• Gizli katman: ").append(hiddenLayers);
        if(findingCount==0)out.append("\nOtomatik CAD kalite taramasında vurgulanacak bulgu çıkmadı.");
        out.append("\nNot: Bu tarama çizim içi geometri/metadata tutarlılığı içindir; yönetmelik veya mühendislik tasarım uygunluğu onayı değildir.");
        return new Result(true,out.toString(),findingCount,highlight);
    }

    private static Result result(MusaAiDrawingIndex index,String title,String detail,int count,Collection<Integer>ids){
        String text=title+" • "+(index.layout.isEmpty()?"aktif layout":index.layout)+
            "\n• "+detail+
            "\nNot: Sonuç CAD çizim verisinden otomatik üretilen bir kontrol bulgusudur.";
        return new Result(true,text,count,ids);
    }

    private static Mode mode(String q){
        if(q==null||q.isEmpty())return null;
        if(contains(q,"mukerrer","duplik","duplicate","tekrar nesne","ust uste nesne","cakisan nesne"))return Mode.DUPLICATE;
        if(contains(q,"sifir uzunluk","dejenere","bozuk geometri"))return Mode.DEGENERATE;
        if(contains(q,"bos metin","bos text","bos mtext"))return Mode.EMPTY_TEXT;
        if(contains(q,"acik polyline","acik poliline","kapanmamis polyline"))return Mode.OPEN_POLYLINE;
        if(contains(q,"katman 0","0 katmani","layer 0"))return Mode.LAYER_ZERO;
        if(contains(q,"todo","tbd","revizyon notu","kontrol notu","eksik not"))return Mode.REVIEW_MARKER;
        if((q.contains("proje")&&q.contains("kontrol"))||
           (q.contains("cizim")&&q.contains("kontrol"))||
           q.contains("kalite kontrol")||q.contains("hata bul")||q.contains("hatalari bul")||
           q.contains("sorun bul")||q.contains("sorunlari bul")||q.contains("uygunsuzluk"))return Mode.FULL;
        return null;
    }

    private static boolean isDegenerate(MusaAiDrawingIndex.Item item){
        if(!item.hasLength())return false;
        if(!(isPolyline(item.type)||"LINE".equals(item.type)||"CIRCLE".equals(item.type)||
             "ARC".equals(item.type)||"ELLIPSE".equals(item.type)))return false;
        return Math.abs(item.length)<=1e-9d;
    }
    private static boolean isTextType(String type){
        return "TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type);
    }
    private static boolean isPolyline(String type){
        return "POLYLINE".equals(type)||"LWPOLYLINE".equals(type);
    }
    private static boolean hasReviewMarker(String text){
        String q=MusaAiDrawingIndex.normalize(text);
        return contains(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et");
    }
    private static boolean containsIgnoreCase(Collection<String>values,String wanted){
        if(values==null)return false;
        for(String value:values)if(value!=null&&value.equalsIgnoreCase(wanted))return true;
        return false;
    }
    private static void addDuplicateIds(Set<Integer>out,Collection<ArrayList<MusaAiDrawingIndex.Item>>groups){
        for(ArrayList<MusaAiDrawingIndex.Item>group:groups)addIds(out,group);
    }
    private static void addIds(Set<Integer>out,Collection<MusaAiDrawingIndex.Item>items){
        for(MusaAiDrawingIndex.Item item:items)if(item!=null&&item.sourceId>=0)out.add(item.sourceId);
    }
    private static boolean contains(String q,String...terms){for(String t:terms)if(q.contains(t))return true;return false;}
    private MusaAiProjectControl(){}
}
