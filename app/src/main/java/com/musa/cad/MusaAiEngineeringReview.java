package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/**
 * Engineering evidence reader for vector DWG/DXF data.
 *
 * This is not image recognition or a design-code certification. Claims are
 * limited to source text, block names, identifiable layers and measured
 * CAD centerlines. In particular, a nearby label is NOT silently assigned
 * to every pipe/duct, and unknown drawing units are never guessed.
 */
public final class MusaAiEngineeringReview {
    private static final Pattern DIAMETER=Pattern.compile(
        "(?iu)(?:\\bDN\\s*|[ØøΦϕ]\\s*|\\bSPIRO\\s*[-:]?\\s*[Øø]?\\s*)(\\d{2,4})(?!\\d)");
    private static final Pattern DUCT=Pattern.compile(
        "(?iu)(?<![\\d])([1-9]\\d{1,3})\\s*[xX×*]\\s*([1-9]\\d{1,3})(?!\\d)");
    private static final Pattern FLOW=Pattern.compile(
        "(?iu)\\bQ\\s*[:=]\\s*(\\d+(?:[.,]\\d+)?)\\s*(m\\s*[³3]\\s*/\\s*h|l\\s*/\\s*s|l\\s*/\\s*dk)");
    private static final Pattern HEAD=Pattern.compile(
        "(?iu)\\bH\\s*[:=]\\s*(\\d+(?:[.,]\\d+)?)\\s*(mss|m\\s*ss|m\\.?)\\b");
    private static final Pattern POWER=Pattern.compile(
        "(?iu)(\\d+(?:[.,]\\d+)?)\\s*kW\\b");

    public static final class Result {
        public final String text;
        public final List<Integer> sourceIds;
        public final int equipmentLabels,dimensions,measuredRuns;
        Result(String text,Collection<Integer> ids,int labels,int dimensions,int measured){
            this.text=text;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
            this.equipmentLabels=labels;this.dimensions=dimensions;this.measuredRuns=measured;
        }
    }

    private enum System {
        WASTE("Pis su"), WATER("Temiz / sıcak su"), FIRE("Yangın"), VENT("Havalandırma"),
        HEAT("Isıtma"), COOL("Klima / soğutma"), GAS("Doğalgaz"), RAIN("Yağmur / drenaj");
        final String label;
        System(String label){this.label=label;}
    }
    private static final class Run {
        int count,diameterSourceCount;
        double drawingLength;
        final LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        void add(MusaAiDrawingIndex.Item i){
            count+=i.quantity;
            drawingLength+=i.length*i.quantity;
            if(i.sourceId>=0)ids.add(i.sourceId);
        }
    }
    private static final class Equipment {
        final String label,annotation;
        final int sourceId;
        final boolean pump;
        Equipment(String label,String annotation,int sourceId,boolean pump){
            this.label=label;this.annotation=annotation;this.sourceId=sourceId;this.pump=pump;
        }
    }

    /** Ordinary engineering-review phrases; preserves special CAD edit/tool commands. */
    public static boolean asksReview(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||q.startsWith("gmekai")||q.startsWith("mekai")||
            q.contains("derin analiz")||q.contains("projeden kesif")||
            q.contains("kesif yukle")||q.contains("revizyon")||
            q.contains("metraj cikar")||q.contains("raporu word")||q.contains("raporu pdf"))return false;
        // Focused architectural/structural/electrical workflows retain their own expert routing.
        if(has(q,"statik","mimari","elektrik","peyzaj","asansor")&&
            !has(q,"mekanik","pis su","temiz su","yangin","tesisat","havalandirma"))return false;
        boolean request=has(q,"analiz","incele","kontrol","denet","metrajini oku","olculeri oku");
        boolean context=has(q,"proje","cizim","pafta","tesisat","boru","kanal","yangin","pompa",
            "hidrofor","havalandirma","sihhi","pis su","sprinkler");
        return request&&context;
    }

    public static Result analyze(MusaAiDrawingIndex index,String fileName){
        if(index==null)return new Result("Tam vektör DWG/DXF çizimi hazır değil.",Collections.emptySet(),0,0,0);
        EnumMap<System,Run> runs=new EnumMap<>(System.class);
        for(System s:System.values())runs.put(s,new Run());
        LinkedHashMap<String,LinkedHashSet<Integer>> dimensions=new LinkedHashMap<>();
        ArrayList<Equipment> equipment=new ArrayList<>();
        LinkedHashSet<Integer> evidenceIds=new LinkedHashSet<>();
        LinkedHashSet<String> seenLabels=new LinkedHashSet<>();
        int seenText=0;

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(Thread.currentThread().isInterrupted())break;
            if(item==null)continue;
            String raw=(item.layer+" "+item.text).trim();
            String normalized=MusaAiDrawingIndex.normalize(raw);
            System system=classify(normalized);
            if(isLinear(item)&&system!=null&&item.hasLength()&&item.length>0){
                runs.get(system).add(item);
                // This diameter is assigned to a measured segment by its *own*
                // explicit layer identifier; nearby free-floating notes are not.
                Matcher assigned=DIAMETER.matcher(item.layer.replace('_',' '));
                if(assigned.find()&&dimensions.size()<60){
                    addDimension(dimensions,"DN"+assigned.group(1)+" • hat katmanı",item.sourceId);
                }
            }
            // Read technical sizes from actual TEXT / MTEXT / ATTRIB or named BLOCKs,
            // not from arbitrary line identifiers.
            boolean annotated=isAnnotation(item)||"BLOCK".equals(item.type)||"INSERT".equals(item.type);
            if(!annotated||item.text.isEmpty())continue;
            seenText++;
            String note=item.text.replace('\n',' ').replace('\r',' ').trim();
            if(note.length()>200)note=note.substring(0,200)+"…";

            Matcher d=DIAMETER.matcher(note);
            while(d.find()&&dimensions.size()<60){
                String n=d.group(1);
                String kind=note.toUpperCase(Locale.ROOT).contains("DN")?"DN":"Ø";
                String label=(kind.equals("DN")?"DN":"Ø")+n;
                if(system==System.VENT&&has(normalized,"spiro","spiral"))label+=" • spiro kanal";
                addDimension(dimensions,label,item.sourceId);
            }
            if(system==System.VENT||has(MusaAiDrawingIndex.normalize(note),"kanal","duct","hava kanali")){
                Matcher rect=DUCT.matcher(note);
                while(rect.find()&&dimensions.size()<60){
                    addDimension(dimensions,rect.group(1)+"×"+rect.group(2)+" dikdörtgen kanal (etiketteki birim doğrulanmalı)",item.sourceId);
                }
            }
            String kind=equipmentName(normalized);
            if(kind!=null&&seenLabels.add(kind+"|"+note+"|"+item.sourceId)){
                if(equipment.size()<100)
                    equipment.add(new Equipment(kind,note,item.sourceId,
                        kind.contains("Pompa")||kind.contains("Hidrofor")));
            }
            if(item.sourceId>=0&&(!note.isEmpty())&&
                (kind!=null||d.reset().find()))evidenceIds.add(item.sourceId);
        }

        double metersPerUnit=metersPerUnit(index.unitName);
        StringBuilder report=new StringBuilder("GANDALF • MÜHENDİSLİK PAFTA ÖN İNCELEMESİ");
        report.append("\nProje: ").append(fileName==null?"aktif çizim":fileName);
        report.append("\nLayout: ").append(index.layout.isEmpty()?"Model":index.layout);
        report.append("\nKapsam: ").append(index.items().size()).append(" örneklenmiş CAD öğesi");
        if(index.entityCount>index.items().size())
            report.append(" / ").append(index.entityCount).append(" kayıtlı nesne (tam tarama değil)");
        report.append("\n\nOKUNAN EKİPMAN / SEMBOL AÇIKLAMALARI");
        if(equipment.isEmpty())report.append("\n• Bu örneklemde adı doğrulanmış ekipman etiketi bulunamadı.");
        else{
            int shown=0;
            for(Equipment e:equipment){
                if(shown++>=16){report.append("\n• Diğer ekipman etiketleri raporda sınırlandırıldı.");break;}
                report.append("\n• ").append(e.label).append(" [").append(source(e.sourceId)).append("]");
                String details=capacity(e.annotation);
                if(!details.isEmpty())report.append(" — ").append(details);
                else if(e.pump)report.append(" — Q/H değerleri bu etiket üzerinde okunamadı");
            }
        }
        report.append("\n\nOKUNAN ÇAP VE KANAL KESİTLERİ");
        if(dimensions.isEmpty())report.append("\n• Bu örneklemde DN, Ø veya hava kanalı kesit etiketi okunamadı.");
        else{
            int shown=0;
            for(Map.Entry<String,LinkedHashSet<Integer>> entry:dimensions.entrySet()){
                if(shown++>=22){report.append("\n• Diğer etiketler özet dışı bırakıldı.");break;}
                report.append("\n• ").append(entry.getKey());
                if(!entry.getValue().isEmpty())report.append(" [kaynak ").append(entry.getValue().iterator().next()).append("]");
            }
        }
        report.append("\n\nÇİZİMDEN ÖLÇÜLEBİLEN HAT METRAJI");
        int measured=0;
        for(Map.Entry<System,Run> entry:runs.entrySet()){
            Run run=entry.getValue();
            if(run.count==0)continue;
            measured+=run.count;
            report.append("\n• ").append(entry.getKey().label).append(": ");
            if(Double.isFinite(metersPerUnit))
                report.append(number(run.drawingLength*metersPerUnit)).append(" m");
            else
                report.append("CAD birimi bilinmediği için metreye çevrilmedi");
            report.append(" (").append(run.count).append(" ölçülebilir vektör parçası)");
            for(Integer id:run.ids){if(evidenceIds.size()>=80)break;evidenceIds.add(id);}
        }
        if(measured==0)report.append("\n• Ölçülebilir tesisat merkez hattı bulunamadı.");
        report.append("\n\nMÜHENDİSLİK KONTROL ADAYLARI");
        int missing=0;
        for(Equipment e:equipment){
            if(!e.pump)continue;
            String v=e.annotation;
            if(!FLOW.matcher(v).find()||!HEAD.matcher(v).find()){
                if(missing++<8)report.append("\n• ").append(e.label).append(" [").append(source(e.sourceId))
                    .append("]: Debi (Q) ve basma yüksekliği (H) birlikte bu ekipman açıklamasında doğrulanamadı.")
                    .append(" Cihaz çizelgesi veya başka paftada olabilir.");
                if(e.sourceId>=0)evidenceIds.add(e.sourceId);
            }
        }
        if(missing==0)report.append("\n• Açık bir Q/H etiket kontrol adayı bulunmadı; bu, projeye uygunluk onayı değildir.");
        report.append("\n\nÖNEMLİ SINIRLAR: Bu rapor CAD yazısı, blok adı ve vektör merkez hattından üretilen kanıtlara dayanır.");
        report.append(" Sembol çiziminden görsel nesne tanıma, çizgi bağlantı topolojisi,");
        report.append(" çapın tüm boru boyunca atanması, hidrolik hesap ve mevzuata uygunluk kontrolü henüz yapılmadı.");
        report.append(" Aynı hatta çakışan veya yardımcı çizgi olabileceği için bulunan metraj kesin keşif değildir.");
        report.append(" İhtiyaç duyulan revizyonlar çizime otomatik işlenmez; kullanıcı onayı gerekir.");
        return new Result(report.toString(),evidenceIds,equipment.size(),dimensions.size(),measured);
    }

    private static boolean isLinear(MusaAiDrawingIndex.Item item){
        return "LINE".equals(item.type)||"LWPOLYLINE".equals(item.type)||
            "POLYLINE".equals(item.type)||"ARC".equals(item.type);
    }
    private static boolean isAnnotation(MusaAiDrawingIndex.Item item){
        return "TEXT".equals(item.type)||"MTEXT".equals(item.type)||
            "ATTRIB".equals(item.type)||"ATTDEF".equals(item.type);
    }
    private static System classify(String q){
        if(has(q,"pis su","atik su","kanalizasyon","waste","sewer"))return System.WASTE;
        if(has(q,"temiz su","soguk su","sicak su","sihhi","pprc","domestic water"))return System.WATER;
        if(has(q,"yangin","sprinkler","hidrant","fire","jokey"))return System.FIRE;
        if(has(q,"havalandirma","hava kanali","duct","spiro","ventilasyon","egzoz kanali"))return System.VENT;
        if(has(q,"kalorifer","isitma","radyator","heating"))return System.HEAT;
        if(has(q,"klima","vrf","sogutma","chiller","fancoil","fan coil"))return System.COOL;
        if(has(q,"dogalgaz","dogal gaz","gas"))return System.GAS;
        if(has(q,"yagmur suyu","drenaj","rainwater","storm"))return System.RAIN;
        return null;
    }
    private static String equipmentName(String q){
        if(has(q,"yangin pompasi","yangin pompa","fire pump"))return "Yangın Pompası";
        if(has(q,"jokey pompa","jockey pump"))return "Jokey Pompa";
        if(has(q,"hidrofor","booster set"))return "Hidrofor";
        if(has(q,"pompa","pump"))return "Pompa";
        if(has(q,"lavabo","washbasin","wash basin"))return "Lavabo";
        if(has(q,"klozet","water closet","toilet"))return "Klozet";
        if(has(q,"pisuvar","urinal"))return "Pisuvar";
        if(has(q,"evye","sink"))return "Evye";
        if(has(q,"yangin dolabi","fire cabinet"))return "Yangın Dolabı";
        if(has(q,"menfez","difuzor","diffuser"))return "Menfez / Difüzör";
        if(has(q,"klima santrali","ahu"))return "Klima Santrali";
        if(has(q,"rooftop"))return "Rooftop";
        if(has(q,"fan coil","fancoil"))return "Fan Coil";
        if(has(q,"vrf"))return "VRF";
        if(has(q,"kazan","boiler"))return "Kazan";
        if(has(q,"boyler"))return "Boyler";
        return null;
    }
    private static String capacity(String raw){
        ArrayList<String> values=new ArrayList<>();
        Matcher q=FLOW.matcher(raw);
        if(q.find())values.add("Q="+q.group(1)+" "+q.group(2).replace(" ",""));
        Matcher h=HEAD.matcher(raw);
        if(h.find())values.add("H="+h.group(1)+" "+h.group(2).replace(" ",""));
        Matcher power=POWER.matcher(raw);
        if(power.find())values.add(power.group(1)+" kW");
        return String.join(", ",values);
    }
    private static void addDimension(Map<String,LinkedHashSet<Integer>> map,String label,int sourceId){
        LinkedHashSet<Integer> sources=map.get(label);
        if(sources==null){sources=new LinkedHashSet<>();map.put(label,sources);}
        if(sourceId>=0)sources.add(sourceId);
    }
    private static String source(int id){return id>=0?"kaynak "+id:"blok / metin etiketi";}
    private static boolean has(String q,String... words){
        String hay=" "+q+" ";
        for(String word:words)
            if(hay.contains(" "+MusaAiDrawingIndex.normalize(word)+" "))return true;
        return false;
    }
    private static double metersPerUnit(String u){
        if(u==null)return Double.NaN;
        switch(u.trim().toLowerCase(Locale.ROOT)){
            case "mm":return 0.001d;case "cm":return 0.01d;case "m":return 1d;
            case "km":return 1000d;case "in":return 0.0254d;case "ft":return 0.3048d;
            default:return Double.NaN;
        }
    }
    private static String number(double value){return String.format(Locale.ROOT,"%.2f",value);}
    private MusaAiEngineeringReview(){}
}
