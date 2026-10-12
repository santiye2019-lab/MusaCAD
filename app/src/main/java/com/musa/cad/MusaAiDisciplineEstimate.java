package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/**
 * Discipline-by-discipline, evidence-first CAD quantity candidates.
 * 2D DWG lines and blocks are NOT automatically billable construction quantities.
 * A price book is read separately; a matched keyword is NOT an approved poz.
 */
public final class MusaAiDisciplineEstimate {
    private static final int MAX_ITEMS=70000,MAX_ROWS=150;
    private static final Pattern MATERIAL=Pattern.compile(
        "(?i)(pprc|pvc|polipropilen|polietilen|celik|bakir|seramik|granit|mermer|"+
        "alcipan|alci levha|parke|beton|demir|donati|kalip|radyator|sprinkler|"+
        "hidrant|vana|hidrofor|boyler|klima|vrf|chiller|fan coil|fan|menfez|"+
        "kablo tavasi|kablo|boru|data|cctv|kamera|otomasyon|bms|ddc|sensor|"+
        "asansor|pano|armatür|armatur)");
    private static final String[] LINE_TAGS={"boru","hat","kanal","kablo","tava",
        "guzergah","guzergahi","tesisat","kolektor","line","pipe","duct"};
    private static final String[] AREA_TAGS={"seramik","kaplama","parke","boya",
        "doseme","zemin","yalitim","kalip","beton","alcipan","alci levha"};
    private static final String[] COUNT_TAGS={"vana","pompa","hidrofor","boyler",
        "klozet","lavabo","fan","klima","vrf","menfez","sprinkler","hidrant",
        "pano","kamera","armatür","armatur","sensor","dedektor","asansor",
        "kapi","pencere","yangin algilama"};

    public static final class Row {
        public final String section,kind,description,unit,searchTerm,layer;
        public final double quantity;
        public final List<Integer> sourceIds;
        public final boolean measured;
        Row(String section,String kind,String description,String unit,double qty,
            String term,String layer,Collection<Integer> ids,boolean measured){
            this.section=section;this.kind=kind;this.description=description;
            this.unit=unit;this.quantity=qty;this.searchTerm=term;this.layer=layer;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
            this.measured=measured;
        }
    }
    public static final class Result {
        public final String title,report;
        public final List<Row> rows;
        public final List<Integer> sourceIds;
        public final boolean completeIndex;
        Result(String title,String report,List<Row> rows,Set<Integer> ids,
               boolean completeIndex){
            this.title=title;this.report=report;
            this.rows=Collections.unmodifiableList(new ArrayList<>(rows));
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
            this.completeIndex=completeIndex;
        }
    }
    private static final class Group{
        String section,kind,description,unit,term,layer;
        double qty;
        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        Group(String section,String kind,String description,String unit,String term,String layer){
            this.section=section;this.kind=kind;this.description=description;this.unit=unit;
            this.term=term;this.layer=layer;
        }
    }

    /** Natural estimate request; never intercept book upload, lookup, or negated commands. */
    public static boolean wantsEstimate(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||MusaAiAnalysisIntent.prohibitsTakeoff(raw))return false;
        if(q.contains("kitabini yukle")||q.contains("kitap yukle")||
           q.contains("kitabini indir")||q.contains("katalog durumu")||
           q.contains("pozunu ara")||q.contains("poz ara"))return false;
        boolean boq=contains(q,"kesif","metraj","yaklasik maliyet","poz eslestir",
            "birim fiyatla hesapla","maliyet analizi","quantity takeoff","boq");
        boolean action=contains(q,"analiz","hazirla","cikar","hesapla","olustur",
            "raporla","kontrol","incele","eslestir","fiyatlandir","yap");
        return boq&&action;
    }

    public static Result analyze(MusaAiDrawingIndex index,MusaAiDisciplinePlan.Plan plan){
        if(plan==null)plan=MusaAiDisciplinePlan.resolve("","all");
        String title=plan.label+" • DİSİPLİN BAZLI KEŞİF ÖN ANALİZİ";
        if(index==null)return new Result(title,title+"\nDWG açılmadığı için metraj oluşturulmadı.",
            Collections.emptyList(),Collections.emptySet(),false);
        double factor=unitFactor(index.unitName);
        boolean validUnits=Double.isFinite(factor)&&factor>0;
        boolean complete=index.items().size()>=index.entityCount&&index.entityCount>0;
        LinkedHashMap<String,Group> groups=new LinkedHashMap<>();
        HashSet<Integer> used=new HashSet<>();
        int visited=0,uncertain=0;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(Thread.currentThread().isInterrupted())break;
            if(visited++>=MAX_ITEMS)break;
            if(item==null||item.sourceId<0||!used.add(item.sourceId))continue;
            String section=null;
            for(String key:plan.sections){
                if(MusaAiDisciplinePlan.matchesSection(key,item)){section=key;break;}
            }
            if(section==null)continue;
            String layer=MusaAiDrawingIndex.normalize(item.layer.replace('-',' ').replace('_',' '));
            String note=MusaAiDrawingIndex.normalize(item.text);
            String combined=layer+" "+note;
            String material=material(combined);
            boolean line=("LINE".equals(item.type)||"LWPOLYLINE".equals(item.type)||
                "POLYLINE".equals(item.type));
            boolean area=("HATCH".equals(item.type)||"LWPOLYLINE".equals(item.type)||
                "POLYLINE".equals(item.type))&&item.closedKnown&&item.closed;
            boolean block=("INSERT".equals(item.type)||"BLOCK".equals(item.type));
            String kind="",unit="";double qty=0;
            // Only directly marked lines from system/material layers count as
            // vector-length candidates. CAD symbols and schematic duplicates
            // still require independent verification before a BOQ is priced.
            if(line&&item.hasLength()&&item.length>0&&validUnits&&
               hasAny(layer,LINE_TAGS)&&!material.isEmpty()){
                kind="ÇİZGİ UZUNLUĞU ADAYI";unit="m";qty=item.length*factor*item.quantity;
            }else if(area&&item.hasArea()&&item.area>0&&validUnits&&
                     hasAny(layer,AREA_TAGS)&&!material.isEmpty()){
                kind="KAPALI ALAN ADAYI";unit="m²";qty=item.area*factor*factor*item.quantity;
            }else if(block&&hasAny(combined,COUNT_TAGS)&&!material.isEmpty()){
                kind="ETİKETLİ BLOK ADEDİ";unit="Ad";qty=item.quantity;
            }else {uncertain++;continue;}
            if(!Double.isFinite(qty)||qty<=0)continue;
            String key=section+"|"+kind+"|"+item.layer+"|"+material;
            Group g=groups.get(key);
            if(g==null){
                if(groups.size()>=MAX_ROWS)break;
                g=new Group(section,kind,material+" • "+item.layer,unit,material,item.layer);
                groups.put(key,g);
            }
            g.qty+=qty;
            if(g.ids.size()<25)g.ids.add(item.sourceId);
        }
        ArrayList<Row> rows=new ArrayList<>();
        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        for(Group g:groups.values()){
            rows.add(new Row(g.section,g.kind,g.description,g.unit,g.qty,
                g.term,g.layer,g.ids,true));
            ids.addAll(g.ids);
        }
        StringBuilder out=new StringBuilder(title)
            .append("\nDüzen: ").append(index.layout)
            .append("\nDWG indeks kapsamı: ").append(index.items().size())
            .append("/").append(index.entityCount)
            .append(" (").append(complete?"kayıt sayısı karşılandı":"KISMİ / eşleme doğrulanmadı").append(")")
            .append("\nÖlçek/birim: ").append(index.unitName.isEmpty()?"belirsiz":index.unitName)
            .append(validUnits?"":" — uzunluk/alan metrajı üretilmedi")
            .append("\nAyıklanamayan teknik öğe: ").append(uncertain);
        for(String section:plan.sections){
            out.append("\n\n").append(MusaAiDisciplinePlan.sectionLabel(section));
            int n=0;
            for(Row row:rows)if(section.equals(row.section)){
                n++;
                out.append("\n• ").append(row.description)
                    .append(" | ").append(row.kind).append(": ")
                    .append(String.format(Locale.ROOT,"%.3f",row.quantity))
                    .append(" ").append(row.unit);
                if(!row.sourceIds.isEmpty())
                    out.append(" [DWG kaynak ").append(row.sourceIds.get(0)).append("]");
                out.append(" [malzeme/imalat kapsamı teyitsiz]");
            }
            if(n==0)out.append("\n• Fiyatlandırılabilir miktar için yeterli CAD kanıtı bulunamadı.");
        }
        out.append("\n\nKEŞİF SINIRLARI: Bunlar yerel çizimden türetilmiş ÖN METRAJ ")
           .append("ADAYLARIDIR; onaylı imalat ölçüsü değildir. Plan/kesit tekrarı, ")
           .append("şematik çizgi, düşey hat, görünmeyen pafta ve malzeme cinsi ayrıca denetlenmelidir.")
           .append("\nPoz eşleştirmesi için kaynak PDF sayfası, poz açıklaması, ")
           .append("birim, montaj dahil/hariç kapsamı ve geçerli fiyat dönemi ")
           .append("doğrulanmadan TL genel toplamı oluşturulmaz.");
        if(visited>=MAX_ITEMS)
            out.append("\nUYARI: ").append(MAX_ITEMS)
                .append(" öğe işlem sınırı; kalanlar değerlendirilmedi.");
        return new Result(title,out.toString(),rows,ids,complete&&visited<MAX_ITEMS);
    }

    private static String material(String s){
        Matcher m=MATERIAL.matcher(s);
        return m.find()?m.group().toLowerCase(Locale.ROOT):"";
    }
    private static boolean hasAny(String q,String[] terms){
        for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;
        return false;
    }
    private static boolean contains(String q,String... terms){
        for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;
        return false;
    }
    private static double unitFactor(String unit){
        if(unit==null)return Double.NaN;
        switch(unit.toLowerCase(Locale.ROOT).trim()){
            case "mm":return 0.001d;
            case "cm":return 0.01d;
            case "m":return 1d;
            case "in":return 0.0254d;
            case "ft":return 0.3048d;
            default:return Double.NaN;
        }
    }
    private MusaAiDisciplineEstimate(){}
}
