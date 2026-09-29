package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;
import java.util.regex.*;

/** Offline quantity/takeoff engine over the active MusaCAD drawing index. */
public final class MusaAiQuantityTakeoff {
    public static final class Answer {
        public final boolean matched;
        public final String text;
        private Answer(boolean matched,String text){this.matched=matched;this.text=text;}
        public static Answer none(){return new Answer(false,"");}
        public static Answer of(String text){return new Answer(true,text);}
    }

    private static final class Filter {
        String layer,type,text,equipment;
        boolean matches(MusaAiDrawingIndex.Item item){
            if(layer!=null&&!MusaAiDrawingIndex.normalize(item.layer).equals(MusaAiDrawingIndex.normalize(layer)))return false;
            if(type!=null&&!matchesType(type,item.type))return false;
            if(text!=null&&!MusaAiDrawingIndex.normalize(item.text).contains(MusaAiDrawingIndex.normalize(text)))return false;
            if(equipment!=null&&!equipment.equals(equipmentCategory(item)))return false;
            return true;
        }
        String label(){
            ArrayList<String>p=new ArrayList<>();
            if(layer!=null)p.add("katman: "+layer);
            if(type!=null)p.add(typeLabel(type));
            if(text!=null)p.add("metin: “"+text+"”");
            if(equipment!=null)p.add(equipment);
            return p.isEmpty()?"aktif görünür çizim":String.join(" • ",p);
        }
    }

    private static final class Stats {
        int matched,lengthCount,areaCount;
        double length,area;
    }

    private static final class LayerStats {
        final String layer;
        int entities,lengthCount,areaCount;
        double length,area;
        LayerStats(String layer){this.layer=layer==null||layer.trim().isEmpty()?"0":layer.trim();}
    }

    private static final class DiameterStats {
        final String discipline;
        final int diameter;
        int parts;
        double length;
        DiameterStats(String discipline,int diameter){this.discipline=discipline;this.diameter=diameter;}
    }

    private static final class EquipmentStats {
        final String label;
        int count;
        EquipmentStats(String label){this.label=label;}
    }

    private static final Pattern DIAMETER_PATTERN=Pattern.compile(
        "(?iu)(?:^|[^A-Z0-9ĞÜŞİÖÇ])(?:DN|D|Ø|⌀|ÇAP|CAPI|CAP)\\s*[-_=:]?\\s*(\\d{2,4})(?=$|[^0-9])");
    private static final Pattern MM_PATTERN=Pattern.compile(
        "(?iu)(?:^|[^0-9])(\\d{2,4})\\s*MM(?=$|[^A-Z0-9])");

    public static Answer answer(MusaAiDrawingIndex index,String raw){
        if(index==null)return Answer.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return Answer.none();

        boolean genericTakeoff=(q.contains("metraj")||q.contains("kesif"))&&
            !asksLength(q)&&!asksArea(q)&&!asksCount(q)&&detectLayer(index,q)==null&&typeFrom(q)==null;
        if(genericTakeoff)return Answer.of(summary(index));

        boolean length=asksLength(q),area=asksArea(q),count=asksCount(q);
        if(!length&&!area&&!count)return Answer.none();

        Filter filter=new Filter();
        filter.layer=detectLayer(index,q);
        filter.type=typeFrom(q);
        filter.text=extractTextTerm(q);
        if(count){
            String equipment=equipmentFromQuery(q);
            if(equipment!=null){filter.equipment=equipment;filter.type="BLOCK";filter.text=null;}
        }
        Stats s=stats(index,filter);

        if(length){
            if(s.matched==0)return Answer.of(filter.label()+" için eşleşen nesne bulunamadı.");
            if(s.lengthCount==0)return Answer.of(filter.label()+" için doğrudan ölçülebilir uzunluk bulunamadı.");
            return Answer.of(filter.label()+"\nToplam uzunluk: "+number(s.length)+" "+unit(index)+
                "\nÖlçülebilen nesne: "+s.lengthCount+(s.matched>s.lengthCount?" • Eşleşen toplam: "+s.matched:""));
        }
        if(area){
            if(s.matched==0)return Answer.of(filter.label()+" için eşleşen nesne bulunamadı.");
            if(s.areaCount==0)return Answer.of(filter.label()+" için kapalı/ölçülebilir alan bulunamadı.");
            return Answer.of(filter.label()+"\nToplam alan: "+number(s.area)+" "+areaUnit(index)+
                "\nAlanı hesaplanan nesne: "+s.areaCount+(s.matched>s.areaCount?" • Eşleşen toplam: "+s.matched:""));
        }
        if(count){
            return Answer.of(filter.label()+"\nAdet: "+s.matched+" • aktif layout: "+index.layout);
        }
        return Answer.none();
    }

    private static String summary(MusaAiDrawingIndex index){
        LinkedHashMap<String,LayerStats> byLayer=new LinkedHashMap<>();
        LinkedHashMap<String,DiameterStats> byDiameter=new LinkedHashMap<>();
        LinkedHashMap<String,EquipmentStats> equipment=new LinkedHashMap<>();

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null||isAnnotation(item.type))continue;
            String key=item.layer==null?"":item.layer.trim();
            LayerStats s=byLayer.get(key);
            if(s==null){s=new LayerStats(key);byLayer.put(key,s);}
            s.entities+=item.quantity;

            if(isLinearTakeoff(item)){
                s.length+=item.length*item.quantity;
                s.lengthCount+=item.quantity;
                int diameter=diameterMm(item);
                if(diameter>0){
                    String discipline=disciplineLabel(item.layer+" "+item.text);
                    String dkey=discipline+"|"+diameter;
                    DiameterStats d=byDiameter.get(dkey);
                    if(d==null){d=new DiameterStats(discipline,diameter);byDiameter.put(dkey,d);}
                    d.length+=item.length*item.quantity;
                    d.parts+=item.quantity;
                }
            }
            if(isAreaTakeoff(item)){
                s.area+=item.area*item.quantity;
                s.areaCount+=item.quantity;
            }
            if(isCountable(item.type)){
                String label=equipmentDisplayLabel(item);
                EquipmentStats e=equipment.get(label);
                if(e==null){e=new EquipmentStats(label);equipment.put(label,e);}
                e.count+=item.quantity;
            }
        }

        double metersPerUnit=metersPerUnit(index.unitName);
        boolean physical=Double.isFinite(metersPerUnit)&&metersPerUnit>0d;
        ArrayList<LayerStats> linear=new ArrayList<>(),areas=new ArrayList<>();
        for(LayerStats s:byLayer.values()){
            if(s.lengthCount>0)linear.add(s);
            if(s.areaCount>0)areas.add(s);
        }
        linear.sort((a,b)->Double.compare(b.length,a.length));
        areas.sort((a,b)->Double.compare(b.area,a.area));

        ArrayList<DiameterStats> diameters=new ArrayList<>(byDiameter.values());
        diameters.sort((a,b)->{
            int d=a.discipline.compareToIgnoreCase(b.discipline);
            return d!=0?d:Integer.compare(a.diameter,b.diameter);
        });
        ArrayList<EquipmentStats> equipmentRows=new ArrayList<>(equipment.values());
        equipmentRows.sort((a,b)->Integer.compare(b.count,a.count));

        StringBuilder out=new StringBuilder();
        out.append("Metraj raporu • ").append(index.layout);
        if(!physical){
            out.append("\n⚠ DWG birimi tanımsız. Uzunluk/alanı metreye çevirmeden ham toplam göstermiyorum.");
            out.append("\nBirim/ölçek doğrulandıktan sonra boru, kanal ve alan metrajı m / m² olarak hesaplanacak.");
        }else{
            appendDiameters(out,diameters,metersPerUnit);
            appendLinear(out,linear,metersPerUnit);
            appendAreas(out,areas,metersPerUnit*metersPerUnit);
        }
        appendEquipment(out,equipmentRows);
        if(linear.isEmpty()&&areas.isEmpty()&&equipmentRows.isEmpty())
            out.append("\nMetraja uygun çizgisel, kapalı alan veya sayılabilir blok bulunamadı.");
        out.append("\nNot: Metin, ölçülendirme, hatch ve yardımcı geometri genel metraja dahil edilmez.");
        return out.toString();
    }

    private static void appendDiameters(StringBuilder out,List<DiameterStats> rows,double scale){
        if(rows.isEmpty())return;
        out.append("\n\nÇAP BAZLI BORU / KANAL METRAJI");
        int shown=0;
        for(DiameterStats s:rows){
            if(shown++>=18){out.append("\n• … diğer çaplar");break;}
            out.append("\n• ").append(s.discipline).append(" Ø").append(s.diameter).append(": ")
                .append(number(s.length*scale)).append(" m")
                .append(" • ").append(s.parts).append(" parça");
        }
    }

    private static void appendLinear(StringBuilder out,List<LayerStats> rows,double scale){
        if(rows.isEmpty())return;
        out.append("\n\nKATMAN BAZLI ÇİZGİSEL METRAJ");
        int shown=0;
        for(LayerStats s:rows){
            if(shown++>=12){out.append("\n• … diğer katmanlar");break;}
            out.append("\n• ").append(layerLabel(s.layer)).append(": ")
                .append(number(s.length*scale)).append(" m")
                .append(" • ").append(s.lengthCount).append(" parça");
        }
    }

    private static void appendAreas(StringBuilder out,List<LayerStats> rows,double scale){
        if(rows.isEmpty())return;
        out.append("\n\nALAN METRAJI");
        int shown=0;
        for(LayerStats s:rows){
            if(shown++>=8){out.append("\n• … diğer katmanlar");break;}
            out.append("\n• ").append(layerLabel(s.layer)).append(": ")
                .append(number(s.area*scale)).append(" m²")
                .append(" • ").append(s.areaCount).append(" kapalı sınır");
        }
    }

    private static void appendEquipment(StringBuilder out,List<EquipmentStats> rows){
        if(rows.isEmpty())return;
        out.append("\n\nADET / EKİPMAN");
        int shown=0;
        for(EquipmentStats s:rows){
            if(shown++>=18){out.append("\n• … diğer ekipmanlar");break;}
            out.append("\n• ").append(s.label).append(": ").append(s.count).append(" adet");
        }
    }

    private static boolean isAnnotation(String type){
        return "TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)||
            "DIMENSION".equals(type)||"HATCH".equals(type)||"XLINE".equals(type)||"RAY".equals(type)||
            "VIEWPORT".equals(type)||"OLE2FRAME".equals(type)||"IMAGE".equals(type);
    }

    private static boolean isLinearTakeoff(MusaAiDrawingIndex.Item item){
        if(item==null||!item.hasLength())return false;
        return "LINE".equals(item.type)||"LWPOLYLINE".equals(item.type)||"POLYLINE".equals(item.type)||
            "ARC".equals(item.type)||"SPLINE".equals(item.type)||"ELLIPSE".equals(item.type);
    }

    private static boolean isAreaTakeoff(MusaAiDrawingIndex.Item item){
        if(item==null||!item.hasArea())return false;
        return item.closedKnown&&item.closed&&("LWPOLYLINE".equals(item.type)||"POLYLINE".equals(item.type));
    }

    private static boolean isCountable(String type){
        return "BLOCK".equals(type)||"INSERT".equals(type)||"MINSERT".equals(type);
    }

    private static String layerLabel(String layer){
        String clean=layer==null||layer.trim().isEmpty()?"0":layer.trim();
        String semantic=disciplineLabel(clean);
        return "Diğer".equals(semantic)?clean:semantic+" • "+clean;
    }

    private static String disciplineLabel(String raw){
        String n=MusaAiDrawingIndex.normalize(raw);
        if(contains(n,"pis su","atiksu","atik su","waste","sewage"))return "Pis su";
        if(contains(n,"temiz su","soguk su","sicak su","domestic","pprc"))return "Temiz su";
        if(contains(n,"yangin","sprinkler","fire","hydrant"))return "Yangın";
        if(contains(n,"dogalgaz","dogal gaz","gaz"))return "Doğalgaz";
        if(contains(n,"havalandirma","hava kanali","kanal","duct"))return "Havalandırma";
        if(contains(n,"vrf","klima","iklimlendirme","hvac","fancoil","fan coil"))return "İklimlendirme";
        if(contains(n,"drenaj","kondens","condens"))return "Drenaj";
        return "Diğer";
    }

    private static int diameterMm(MusaAiDrawingIndex.Item item){
        String raw=(item.layer+" "+item.text).trim();
        Matcher m=DIAMETER_PATTERN.matcher(raw);
        if(m.find())return validDiameter(m.group(1));
        m=MM_PATTERN.matcher(raw);
        if(m.find())return validDiameter(m.group(1));
        return 0;
    }

    private static int validDiameter(String raw){
        try{int value=Integer.parseInt(raw);return value>=10&&value<=4000?value:0;}
        catch(Exception e){return 0;}
    }

    private static String equipmentDisplayLabel(MusaAiDrawingIndex.Item item){
        String category=equipmentCategory(item);
        String block=item.text==null?"":item.text.trim();
        if(category==null)return block.isEmpty()?layerLabel(item.layer):block+" • "+layerLabel(item.layer);
        if(block.isEmpty()||MusaAiDrawingIndex.normalize(block).equals(MusaAiDrawingIndex.normalize(category)))return category;
        return category+" • "+block;
    }

    private static String equipmentCategory(MusaAiDrawingIndex.Item item){
        if(item==null)return null;
        String n=MusaAiDrawingIndex.normalize((item.text==null?"":item.text)+" "+(item.layer==null?"":item.layer));
        if(contains(n,"lavabo","wash basin","washbasin"))return "Lavabo";
        if(contains(n,"klozet","toilet","water closet"," wc "))return "Klozet";
        if(contains(n,"pisuvar","urinal"))return "Pisuvar";
        if(contains(n,"suzgec","floor drain","yer suzgeci"))return "Süzgeç";
        if(contains(n,"evye","kitchen sink"))return "Evye";
        if(contains(n,"dus","shower"))return "Duş";
        if(contains(n,"batarya","musluk","faucet"))return "Batarya / Musluk";
        if(contains(n,"yangin dolabi","fire cabinet","hose reel"))return "Yangın dolabı";
        if(contains(n,"sprinkler"))return "Sprinkler";
        if(contains(n,"vana","valf","valve"))return "Vana";
        if(contains(n,"pompa","pump"))return "Pompa";
        if(contains(n,"hidrofor","booster"))return "Hidrofor";
        if(contains(n,"menfez","grille","diffuser","difuzor"))return "Menfez / Difüzör";
        if(contains(n,"fan coil","fancoil"))return "Fan-coil";
        if(contains(n,"vrf"))return "VRF";
        if(contains(n,"radyator","radiator"))return "Radyatör";
        if(contains(n,"kazan","boiler"))return "Kazan";
        if(contains(n,"fan"))return "Fan";
        return null;
    }

    private static String equipmentFromQuery(String q){
        String n=" "+MusaAiDrawingIndex.normalize(q)+" ";
        if(contains(n,"lavabo","wash basin","washbasin"))return "Lavabo";
        if(contains(n,"klozet","toilet","water closet"," wc "))return "Klozet";
        if(contains(n,"pisuvar","urinal"))return "Pisuvar";
        if(contains(n,"suzgec","floor drain","yer suzgeci"))return "Süzgeç";
        if(contains(n,"evye","kitchen sink"))return "Evye";
        if(contains(n,"dus","shower"))return "Duş";
        if(contains(n,"batarya","musluk","faucet"))return "Batarya / Musluk";
        if(contains(n,"yangin dolabi","fire cabinet","hose reel"))return "Yangın dolabı";
        if(contains(n,"sprinkler"))return "Sprinkler";
        if(contains(n,"vana","valf","valve"))return "Vana";
        if(contains(n,"pompa","pump"))return "Pompa";
        if(contains(n,"hidrofor","booster"))return "Hidrofor";
        if(contains(n,"menfez","grille","diffuser","difuzor"))return "Menfez / Difüzör";
        if(contains(n,"fan coil","fancoil"))return "Fan-coil";
        if(contains(n,"vrf"))return "VRF";
        if(contains(n,"radyator","radiator"))return "Radyatör";
        if(contains(n,"kazan","boiler"))return "Kazan";
        if(contains(n,"fan"))return "Fan";
        return null;
    }

    private static double metersPerUnit(String raw){
        String u=raw==null?"":raw.trim().toLowerCase(Locale.ROOT);
        switch(u){
            case "mm":return .001d;case "cm":return .01d;case "m":return 1d;case "km":return 1000d;
            case "in":return .0254d;case "ft":return .3048d;case "yd":return .9144d;
            case "us-in":return .0254000508001016d;case "us-ft":return .3048006096012192d;case "us-yd":return .9144018288036576d;
            default:return Double.NaN;
        }
    }

    private static Stats stats(MusaAiDrawingIndex index,Filter filter){
        Stats s=new Stats();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(!filter.matches(item))continue;
            s.matched+=item.quantity;
            if(item.hasLength()){s.length+=item.length*item.quantity;s.lengthCount+=item.quantity;}
            if(item.hasArea()){s.area+=item.area*item.quantity;s.areaCount+=item.quantity;}
        }
        return s;
    }

    private static String detectLayer(MusaAiDrawingIndex index,String q){
        String best=null;int bestLen=0;
        for(String layer:index.allLayers){
            String n=MusaAiDrawingIndex.normalize(layer);
            if(n.length()<2||!containsPhrase(q,n))continue;
            if(n.length()>bestLen){best=layer;bestLen=n.length();}
        }
        return best;
    }

    private static boolean containsPhrase(String hay,String needle){
        return (" "+hay+" ").contains(" "+needle+" ");
    }

    private static String extractTextTerm(String q){
        if(!(q.contains("gecen")||q.contains("yazan")))return null;
        String s=q
            .replace("gecenlerin"," ").replace("gecenleri"," ").replace("gecen"," ")
            .replace("yazanlarin"," ").replace("yazanlari"," ").replace("yazan"," ")
            .replace("toplam"," ").replace("uzunlugu"," ").replace("uzunluk"," ")
            .replace("alani"," ").replace("alan"," ").replace("adedi"," ").replace("adet"," ")
            .replace("sayisi"," ").replace("kac"," ").replace("metraj"," ").replace("nedir"," ")
            .replace("ne kadar"," ").replace("cizimde"," ").replace("projede"," ");
        s=s.replaceAll("\\b(olan|olanlar|nesne|nesneler|oge|ogeler|var|bir)\\b"," ").trim().replaceAll("\\s+"," ");
        return s.isEmpty()?null:s;
    }

    private static boolean asksLength(String q){
        return q.contains("toplam uzunluk")||q.contains("uzunlugu")||q.contains("uzunluklari")||
            q.contains("metraj ne")||q.contains("metraji")||q.contains("kac metre")||q.contains("kac m ");
    }
    private static boolean asksArea(String q){
        return q.contains("toplam alan")||q.contains("alani")||q.contains("alanlari")||
            q.contains("metrekare")||q.contains("m2");
    }
    private static boolean asksCount(String q){
        return q.contains("kac ")||q.endsWith(" kac")||q.contains(" adedi")||q.contains(" adet")||q.contains(" sayisi");
    }

    private static String typeFrom(String q){
        if(contains(q,"polyline","poliline","coklu cizgi"))return "POLYLINE_ANY";
        if(contains(q,"daire","cember","circle"))return "CIRCLE";
        if(contains(q,"yay","arc"))return "ARC";
        if(contains(q,"elips","ellipse"))return "ELLIPSE";
        if(contains(q,"hatch","tarama"))return "HATCH";
        if(contains(q,"olculendirme","dimension"))return "DIMENSION";
        if(contains(q,"metin","yazi","text","mtext"))return "TEXT_ANY";
        if(contains(q,"xline","sonsuz cizgi"))return "XLINE";
        if(contains(q,"blok","block","ekipman"))return "BLOCK";
        if(contains(q,"nokta","point"))return "POINT";
        if(contains(q,"cizgi","line"))return "LINE";
        return null;
    }
    private static boolean matchesType(String wanted,String actual){
        if("POLYLINE_ANY".equals(wanted))return "LWPOLYLINE".equals(actual)||"POLYLINE".equals(actual);
        if("TEXT_ANY".equals(wanted))return "TEXT".equals(actual)||"MTEXT".equals(actual)||"ATTRIB".equals(actual)||"ATTDEF".equals(actual);
        return wanted.equals(actual);
    }
    private static boolean contains(String q,String...terms){for(String t:terms)if(q.contains(t))return true;return false;}
    private static String typeLabel(String t){
        if("POLYLINE_ANY".equals(t))return "Polyline";if("TEXT_ANY".equals(t))return "Metin";
        switch(t){case "CIRCLE":return "Daire";case "ARC":return "Yay";case "ELLIPSE":return "Elips";case "HATCH":return "Hatch";case "LINE":return "Çizgi";case "POINT":return "Nokta";case "DIMENSION":return "Ölçülendirme";case "XLINE":return "XLine";default:return t;}
    }
    private static String unit(MusaAiDrawingIndex i){return i.unitName.isEmpty()?"çizim birimi":i.unitName;}
    private static String areaUnit(MusaAiDrawingIndex i){return unit(i)+"²";}
    private static String number(double v){
        DecimalFormatSymbols s=DecimalFormatSymbols.getInstance(new Locale("tr","TR"));
        DecimalFormat f=new DecimalFormat("#,##0.###",s);return f.format(v);
    }
    private MusaAiQuantityTakeoff(){}
}
