package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;

/**
 * Deterministic offline mechanical-installation checks over the active drawing.
 *
 * This engine intentionally reports "review candidates" instead of certifying
 * engineering or code compliance. It uses visible CAD layer/text metadata and
 * lightweight geometry already indexed by MusaCAD AI.
 */
public final class MusaAiMechanicalControl {
    public static final class Result {
        public final boolean matched;
        public final String text;
        public final int findingCount;
        public final List<Integer> sourceIds;
        private Result(boolean matched,String text,int findingCount,Collection<Integer>sourceIds){
            this.matched=matched;
            this.text=text;
            this.findingCount=Math.max(0,findingCount);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Result none(){return new Result(false,"",0,Collections.emptyList());}
    }

    private enum Mode { FULL,WASTE,RAIN,WATER,HEATING,COOLING,VENTILATION,FIRE,GAS,EQUIPMENT }

    private enum SystemType {
        WASTE("Pis su / atık su"),
        RAIN("Yağmur suyu"),
        WATER("Temiz / sıcak-soğuk su"),
        HEATING("Isıtma"),
        COOLING("Soğutma / VRF"),
        VENTILATION("Havalandırma"),
        FIRE("Yangın / sprinkler"),
        GAS("Doğalgaz"),
        EQUIPMENT("Mekanik ekipman");

        final String label;
        SystemType(String label){this.label=label;}
    }

    private static final class Stats {
        int count,lengthCount,areaCount;
        double length,area;
        boolean diameterMarker,slopeMarker;
        void add(MusaAiDrawingIndex.Item item){
            count++;
            if(item.hasLength()){length+=item.length;lengthCount++;}
            if(item.hasArea()){area+=item.area;areaCount++;}
            diameterMarker|=hasDiameterMarker(item.layer)||hasDiameterMarker(item.text);
            slopeMarker|=hasSlopeMarker(item.layer)||hasSlopeMarker(item.text);
        }
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        Mode mode=mode(q);
        if(mode==null)return Result.none();

        EnumMap<SystemType,Stats>stats=new EnumMap<>(SystemType.class);
        for(SystemType s:SystemType.values())stats.put(s,new Stats());

        LinkedHashSet<Integer>highlight=new LinkedHashSet<>();
        ArrayList<MusaAiDrawingIndex.Item>degenerate=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item>openRuns=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item>reviewMarkers=new ArrayList<>();

        boolean fireSupportMarker=false;
        boolean gasControlMarker=false;
        boolean ventilationTerminalMarker=false;

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            EnumSet<SystemType>systems=classify(hay);
            if(systems.isEmpty())continue;

            for(SystemType system:systems)stats.get(system).add(item);

            if(item.hasLength()&&item.length<=1e-9d){
                degenerate.add(item);
                addId(highlight,item);
            }

            if(isPipingSystem(systems)&&isPolyline(item.type)&&item.closedKnown&&!item.closed){
                openRuns.add(item);
                addId(highlight,item);
            }

            if(hasReviewMarker(item.text)){
                reviewMarkers.add(item);
                addId(highlight,item);
            }

            if(systems.contains(SystemType.FIRE)&&contains(hay,
                "pompa","pump","jokey","jockey","depo","tank","hidrofor","yangin dolabi","hidrant","itfaiye"))
                fireSupportMarker=true;

            if(systems.contains(SystemType.GAS)&&contains(hay,
                "vana","valve","regulator","sayac","meter","solenoid"))
                gasControlMarker=true;

            if(systems.contains(SystemType.VENTILATION)&&contains(hay,
                "menfez","difuzor","diffuser","damper","fan","ahu","santral","egzoz","exhaust","taze hava"))
                ventilationTerminalMarker=true;
        }

        EnumSet<SystemType>selected=selectedSystems(mode);
        int detected=0;
        for(SystemType s:selected)if(stats.get(s).count>0)detected++;

        StringBuilder out=new StringBuilder();
        out.append("Mekanik tesisat AI kontrolü • ")
           .append(index.layout.isEmpty()?"aktif layout":index.layout);

        out.append("\nTespit edilen sistemler");
        if(detected==0){
            out.append("\n• Seçilen kapsamda katman/metin eşleşmesi bulunamadı.");
        }else{
            for(SystemType s:selected){
                Stats st=stats.get(s);
                if(st.count<=0)continue;
                out.append("\n• ").append(s.label).append(": ").append(st.count).append(" nesne");
                if(st.lengthCount>0)out.append(" • ").append(number(st.length)).append(" ").append(unit(index)).append(" çizgisel");
                if(st.areaCount>0)out.append(" • ").append(number(st.area)).append(" ").append(areaUnit(index)).append(" alan");
            }
        }

        int findingCount=0;
        out.append("\nİnceleme adayları");

        int degenerateCount=countRelevant(degenerate,selected);
        int openCount=countRelevant(openRuns,selected);
        int reviewCount=countRelevant(reviewMarkers,selected);

        if(degenerateCount>0){
            out.append("\n• Sıfır uzunluk/dejenere mekanik geometri: ").append(degenerateCount);
            findingCount+=degenerateCount;
        }
        if(openCount>0){
            out.append("\n• Açık hat polyline: ").append(openCount)
               .append(" (hat çiziminde normal olabilir; bağlantı sürekliliğini kontrol edin)");
            findingCount+=openCount;
        }
        if(reviewCount>0){
            out.append("\n• TODO / EKSİK / REVİZE benzeri mekanik not: ").append(reviewCount);
            findingCount+=reviewCount;
        }

        int metadataFindings=0;
        for(SystemType s:selected){
            Stats st=stats.get(s);
            if(st.count<=0)continue;

            if(isPipeLike(s)&&st.lengthCount>0&&!st.diameterMarker){
                out.append("\n• ").append(s.label)
                   .append(": görünür metin/katmanlarda çap veya DN etiketi bulunamadı.");
                metadataFindings++;
            }
            if(s==SystemType.WASTE&&st.lengthCount>0&&!st.slopeMarker){
                out.append("\n• Pis su / atık su: görünür metinlerde eğim etiketi bulunamadı.");
                metadataFindings++;
            }
        }

        if(selected.contains(SystemType.FIRE)&&stats.get(SystemType.FIRE).count>0&&!fireSupportMarker){
            out.append("\n• Yangın sistemi: pompa/depo/jokey/hidrant/itfaiye bağlantısı etiketi bulunamadı; ilgili pafta veya ekipman listesini kontrol edin.");
            metadataFindings++;
        }
        if(selected.contains(SystemType.GAS)&&stats.get(SystemType.GAS).count>0&&!gasControlMarker){
            out.append("\n• Doğalgaz sistemi: vana/regülatör/sayaç etiketi bulunamadı; ilgili paftayı kontrol edin.");
            metadataFindings++;
        }
        if(selected.contains(SystemType.VENTILATION)&&stats.get(SystemType.VENTILATION).count>0&&!ventilationTerminalMarker){
            out.append("\n• Havalandırma sistemi: menfez/difüzör/damper/fan/santral etiketi bulunamadı.");
            metadataFindings++;
        }

        findingCount+=metadataFindings;
        if(findingCount==0){
            out.append("\n• Otomatik mekanik taramada belirgin bir kontrol adayı oluşmadı.");
        }

        out.append("\nNot: Bu kontrol görünür CAD katmanları, metinleri ve geometri metadata'sına dayanır. ")
           .append("Hidrolik hesap, ısı yükü, basınç kaybı, cihaz kapasitesi, yönetmelik ve saha uygunluğu ayrıca mühendis tarafından doğrulanmalıdır.");

        LinkedHashSet<Integer>relevantHighlight=new LinkedHashSet<>();
        collectRelevantIds(relevantHighlight,degenerate,selected);
        collectRelevantIds(relevantHighlight,openRuns,selected);
        collectRelevantIds(relevantHighlight,reviewMarkers,selected);
        return new Result(true,out.toString(),findingCount,relevantHighlight);
    }

    private static Mode mode(String q){
        if(q==null||q.isEmpty())return null;
        if(q.equals("mekai")||q.contains("ai mekanik kontrol")||
           q.contains("mekanik tesisat kontrol")||q.contains("mekanik proje kontrol")||
           q.contains("mekanik acidan kontrol")||
           (q.contains("mekanik")&&asksControl(q)))return Mode.FULL;

        if(!asksControl(q))return null;
        if(contains(q,"pis su","atik su","kanalizasyon","waste","sewer","drenaj","drain"))return Mode.WASTE;
        if(contains(q,"yagmur suyu","rain","storm"))return Mode.RAIN;
        if(contains(q,"temiz su","kullanma suyu","sicak su","soguk su","potable","domestic water"))return Mode.WATER;
        if(contains(q,"isitma","kalorifer","radyator","yerden isitma","heating"))return Mode.HEATING;
        if(contains(q,"sogutma","vrf","vrv","chiller","fancoil","fan coil","klima","cooling"))return Mode.COOLING;
        if(contains(q,"havalandirma","ventilasyon","duct","kanal","menfez","damper","ahu","santral","egzoz","taze hava"))return Mode.VENTILATION;
        if(contains(q,"yangin","sprinkler","hidrant","fire","jokey","jockey"))return Mode.FIRE;
        if(contains(q,"dogalgaz","natural gas","gaz tesisati"))return Mode.GAS;
        if(contains(q,"pompa","hidrofor","depo","boyler","esanj","kazan","mekanik ekipman"))return Mode.EQUIPMENT;
        return null;
    }

    private static boolean asksControl(String q){
        return contains(q,"kontrol","incele","tara","hata","eksik","uygunsuz","denetle","check");
    }

    private static EnumSet<SystemType>selectedSystems(Mode mode){
        switch(mode){
            case WASTE:return EnumSet.of(SystemType.WASTE);
            case RAIN:return EnumSet.of(SystemType.RAIN);
            case WATER:return EnumSet.of(SystemType.WATER);
            case HEATING:return EnumSet.of(SystemType.HEATING);
            case COOLING:return EnumSet.of(SystemType.COOLING);
            case VENTILATION:return EnumSet.of(SystemType.VENTILATION);
            case FIRE:return EnumSet.of(SystemType.FIRE);
            case GAS:return EnumSet.of(SystemType.GAS);
            case EQUIPMENT:return EnumSet.of(SystemType.EQUIPMENT);
            default:return EnumSet.allOf(SystemType.class);
        }
    }

    private static EnumSet<SystemType>classify(String hay){
        EnumSet<SystemType>out=EnumSet.noneOf(SystemType.class);
        if(contains(hay,"pis su","atik su","kanalizasyon","waste","sewer","foul","soil","drenaj","drain"))out.add(SystemType.WASTE);
        if(contains(hay,"yagmur","rain","storm"))out.add(SystemType.RAIN);
        if(contains(hay,"temiz su","kullanma suyu","sicak su","soguk su","potable","domestic water","cold water","hot water"))out.add(SystemType.WATER);
        if(contains(hay,"isitma","kalorifer","radyator","yerden isitma","heating"))out.add(SystemType.HEATING);
        if(contains(hay,"sogutma","vrf","vrv","chiller","fancoil","fan coil","klima","cooling","refrigerant"))out.add(SystemType.COOLING);
        if(contains(hay,"havalandirma","ventilasyon","duct","menfez","difuzor","diffuser","damper","ahu","santral","egzoz","exhaust","taze hava","fresh air"))out.add(SystemType.VENTILATION);
        if(contains(hay,"yangin","sprinkler","hidrant","fire","jokey","jockey","itfaiye"))out.add(SystemType.FIRE);
        if(contains(hay,"dogalgaz","natural gas","gaz tesisati","gas line"))out.add(SystemType.GAS);
        if(contains(hay,"pompa","pump","hidrofor","depo","tank","boyler","boiler","esanj","kazan","chiller","ahu","santral"))out.add(SystemType.EQUIPMENT);
        return out;
    }

    private static boolean isPipingSystem(EnumSet<SystemType>systems){
        return systems.contains(SystemType.WASTE)||systems.contains(SystemType.RAIN)||
            systems.contains(SystemType.WATER)||systems.contains(SystemType.HEATING)||
            systems.contains(SystemType.COOLING)||systems.contains(SystemType.FIRE)||
            systems.contains(SystemType.GAS);
    }

    private static boolean isPipeLike(SystemType system){
        return system==SystemType.WASTE||system==SystemType.RAIN||system==SystemType.WATER||
            system==SystemType.HEATING||system==SystemType.COOLING||
            system==SystemType.FIRE||system==SystemType.GAS;
    }

    private static boolean isPolyline(String type){
        return "POLYLINE".equals(type)||"LWPOLYLINE".equals(type);
    }

    private static boolean hasReviewMarker(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et");
    }

    private static boolean hasDiameterMarker(String raw){
        if(raw==null||raw.trim().isEmpty())return false;
        String upper=raw.toUpperCase(Locale.ROOT).replace('Ø','D');
        if(upper.matches(".*\\bDN\\s*[-:]?\\s*\\d+.*"))return true;
        if(upper.matches(".*\\bD\\s*[-:]?\\s*\\d+.*"))return true;
        if(upper.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*MM\\b.*"))return true;
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"cap ","diameter ","diam ");
    }

    private static boolean hasSlopeMarker(String raw){
        if(raw==null||raw.trim().isEmpty())return false;
        if(raw.contains("%")||raw.contains("‰"))return true;
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"egim","slope","meyil");
    }

    private static int countRelevant(Collection<MusaAiDrawingIndex.Item>items,EnumSet<SystemType>selected){
        int n=0;
        for(MusaAiDrawingIndex.Item item:items)if(isRelevant(item,selected))n++;
        return n;
    }

    private static void collectRelevantIds(Set<Integer>out,Collection<MusaAiDrawingIndex.Item>items,EnumSet<SystemType>selected){
        for(MusaAiDrawingIndex.Item item:items)if(isRelevant(item,selected)&&item.sourceId>=0)out.add(item.sourceId);
    }

    private static boolean isRelevant(MusaAiDrawingIndex.Item item,EnumSet<SystemType>selected){
        if(item==null)return false;
        EnumSet<SystemType>systems=classify(MusaAiDrawingIndex.normalize(item.layer+" "+item.text));
        for(SystemType s:systems)if(selected.contains(s))return true;
        return false;
    }

    private static void addId(Set<Integer>out,MusaAiDrawingIndex.Item item){
        if(item!=null&&item.sourceId>=0)out.add(item.sourceId);
    }

    private static boolean contains(String q,String...terms){
        for(String term:terms)if(q.contains(term))return true;
        return false;
    }

    private static String unit(MusaAiDrawingIndex index){
        return index.unitName.isEmpty()?"çizim birimi":index.unitName;
    }

    private static String areaUnit(MusaAiDrawingIndex index){return unit(index)+"²";}

    private static String number(double value){
        DecimalFormatSymbols symbols=DecimalFormatSymbols.getInstance(new Locale("tr","TR"));
        return new DecimalFormat("#,##0.###",symbols).format(value);
    }

    private MusaAiMechanicalControl(){}
}
