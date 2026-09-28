package com.musa.cad;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.*;

/** Offline mechanical-installation analysis over the active MusaCAD drawing index. */
public final class MusaAiMechanical {
    private enum SystemKind {
        CLEAN_WATER("Temiz / soğuk su",new String[]{"temiz su","soguk su","cold water","potable water","pprc","hidrofor","su deposu"}),
        HOT_WATER("Sıcak su / resirkülasyon",new String[]{"sicak su","hot water","resirkulasyon","resirk","boyler"}),
        WASTE_WATER("Pis / atık su",new String[]{"pis su","atik su","waste water","wastewater","foul water","kanalizasyon","pis su tesisati"}),
        RAIN_WATER("Yağmur suyu",new String[]{"yagmur suyu","yagmur","rain water","rainwater"}),
        FIRE("Yangın / sprinkler",new String[]{"yangin","sprinkler","hidrant","fire fighting","firefighting","yangin dolabi","itfaiye"}),
        HEATING("Isıtma",new String[]{"isitma","kalorifer","radyator","radiator","yerden isitma","floor heating","kazan"}),
        COOLING("VRF / klima / soğutma",new String[]{"vrf","vrv","klima","sogutma","cooling","fan coil","fancoil","chiller","dx"}),
        VENTILATION("Havalandırma",new String[]{"havalandirma","ventilasyon","ventilation","menfez","difuzor","diffuser","egzoz","exhaust","taze hava","fresh air","duman egzoz"}),
        GAS("Doğalgaz",new String[]{"dogalgaz","dogal gaz","natural gas","gaz tesisati","gaz hatti"});

        final String label;final String[]terms;
        SystemKind(String label,String[]terms){this.label=label;this.terms=terms;}
    }

    public static final class Result {
        public final boolean matched;
        public final String text;
        public final List<Integer>sourceIds;
        public final int issueCount;
        private Result(boolean matched,String text,Collection<Integer>ids,int issueCount){
            this.matched=matched;this.text=text==null?"":text;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
            this.issueCount=Math.max(0,issueCount);
        }
        public static Result none(){return new Result(false,"",Collections.emptyList(),0);}
    }

    private static final class Stats {
        int count,lengthCount,areaCount,openPolylines,degenerate,reviewNotes;
        double length,area;
        final LinkedHashSet<String>layers=new LinkedHashSet<>(),types=new LinkedHashSet<>();
        final ArrayList<MusaAiDrawingIndex.Item>items=new ArrayList<>();
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return Result.none();

        LinkedHashSet<SystemKind>requested=detectRequested(q);
        boolean mechanicalOverview=containsAny(q,"mekanik tesisat","mekanik proje","mekanik sistem","mep");
        boolean action=containsAny(q,"kontrol","incele","analiz","ozet","metraj","uzunluk","say","adet","kac","neler var","goster","bul");
        if(requested.isEmpty()&&!mechanicalOverview)return Result.none();
        if(requested.isEmpty()&&mechanicalOverview&&!action&&!(q.equals("mekanik")||q.equals("mep")))return Result.none();

        if(requested.isEmpty())return overview(index,q);

        LinkedHashSet<Integer>highlight=new LinkedHashSet<>();
        int issues=0;
        StringBuilder out=new StringBuilder();
        out.append("MusaCAD AI • Mekanik tesisat analizi");
        for(SystemKind kind:requested){
            Stats s=stats(index,kind);
            boolean check=containsAny(q,"kontrol","hata","sorun","uygunsuz","incele");
            if(out.length()>0)out.append("\n\n");
            appendSystem(out,index,kind,s,check);
            if(check){
                issues+=s.degenerate+s.openPolylines+s.reviewNotes;
                for(MusaAiDrawingIndex.Item item:s.items){
                    if(isIssueCandidate(item)&&item.sourceId>=0)highlight.add(item.sourceId);
                }
            }else{
                for(MusaAiDrawingIndex.Item item:s.items)if(item.sourceId>=0)highlight.add(item.sourceId);
            }
        }
        out.append("\n\nNot: Mekanik AI katman, metin ve CAD geometrisini tarar; boru çapı, debi, basınç kaybı, cihaz kapasitesi ve yönetmelik uygunluğu gibi tasarım kriterlerini veri mevcut değilse varsaymaz.");
        return new Result(true,out.toString(),highlight,issues);
    }

    /** Compact mechanical section for automatic project reports. */
    public static String reportSection(MusaAiDrawingIndex index){
        if(index==null)return "";
        StringBuilder b=new StringBuilder();
        int detected=0,totalItems=0,totalIssues=0;
        for(SystemKind kind:SystemKind.values()){
            Stats s=stats(index,kind);
            if(s.count==0)continue;
            detected++;totalItems+=s.count;totalIssues+=s.degenerate+s.openPolylines+s.reviewNotes;
            b.append("\n• ").append(kind.label).append(": ").append(s.count).append(" nesne");
            if(s.lengthCount>0)b.append(" • ").append(number(s.length)).append(" ").append(unit(index));
            if(s.openPolylines>0)b.append(" • açık polyline ").append(s.openPolylines);
            if(s.degenerate>0)b.append(" • dejenere ").append(s.degenerate);
            if(s.reviewNotes>0)b.append(" • kontrol notu ").append(s.reviewNotes);
        }
        if(detected==0)return "• Mekanik tesisat sistemi katman/metin adlarından güvenle sınıflandırılamadı.";
        return "• Algılanan mekanik sistem: "+detected+" • sınıflandırılan nesne: "+totalItems+
            " • inceleme adayı: "+totalIssues+b;
    }

    private static Result overview(MusaAiDrawingIndex index,String q){
        StringBuilder out=new StringBuilder("MusaCAD AI • Mekanik tesisat özeti");
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        int systems=0,total=0,totalIssues=0;
        for(SystemKind kind:SystemKind.values()){
            Stats s=stats(index,kind);
            if(s.count==0)continue;
            systems++;total+=s.count;totalIssues+=s.degenerate+s.openPolylines+s.reviewNotes;
            out.append("\n• ").append(kind.label).append(": ").append(s.count).append(" nesne");
            if(s.lengthCount>0)out.append(" • uzunluk ").append(number(s.length)).append(" ").append(unit(index));
            if(!s.layers.isEmpty())out.append(" • katman ").append(joinLimited(s.layers,3));
            for(MusaAiDrawingIndex.Item item:s.items)if(item.sourceId>=0)ids.add(item.sourceId);
        }
        if(systems==0)out.append("\nBelirgin mekanik tesisat katmanı/metni algılanmadı.");
        else out.append("\nToplam algılanan sistem: ").append(systems).append(" • sınıflandırılan nesne: ").append(total)
            .append(" • inceleme adayı: ").append(totalIssues);
        out.append("\nÖrnek komutlar: “Pis su tesisatını kontrol et”, “Yangın tesisatı metrajı”, “VRF cihazlarını say”, “Havalandırmayı analiz et”.");
        out.append("\nNot: Sistem sınıflandırması katman ve çizim metni adlarına dayanır.");
        return new Result(true,out.toString(),ids,totalIssues);
    }

    private static void appendSystem(StringBuilder out,MusaAiDrawingIndex index,SystemKind kind,Stats s,boolean check){
        out.append(kind.label);
        out.append("\n• Algılanan nesne: ").append(s.count);
        if(!s.layers.isEmpty())out.append("\n• Katmanlar: ").append(joinLimited(s.layers,8));
        if(!s.types.isEmpty())out.append("\n• Nesne türleri: ").append(joinLimited(s.types,8));
        if(s.lengthCount>0)out.append("\n• Ölçülebilir toplam uzunluk: ").append(number(s.length)).append(" ").append(unit(index))
            .append(" (").append(s.lengthCount).append(" nesne)");
        if(s.areaCount>0)out.append("\n• Ölçülebilir toplam alan: ").append(number(s.area)).append(" ").append(unit(index)).append("²");
        if(check){
            out.append("\nKontrol adayları");
            out.append("\n• Sıfır uzunluk/dejenere geometri: ").append(s.degenerate);
            out.append("\n• Açık polyline: ").append(s.openPolylines).append(" (hat/kanal sürekliliği açısından incelenmeli; tasarıma göre normal olabilir)");
            out.append("\n• EKSİK/REVİZE/KONTROL benzeri not: ").append(s.reviewNotes);
        }
        if(s.count==0)out.append("\nBu sistem için katman veya metin eşleşmesi bulunmadı.");
    }

    private static Stats stats(MusaAiDrawingIndex index,SystemKind kind){
        Stats s=new Stats();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(!matches(kind,item))continue;
            s.items.add(item);s.count++;
            if(!item.layer.isEmpty())s.layers.add(item.layer);
            if(!item.type.isEmpty())s.types.add(item.type);
            if(item.hasLength()){s.length+=item.length;s.lengthCount++;}
            if(item.hasArea()){s.area+=item.area;s.areaCount++;}
            if(isDegenerate(item))s.degenerate++;
            if(isPolyline(item.type)&&item.closedKnown&&!item.closed)s.openPolylines++;
            if(hasReviewMarker(item.text))s.reviewNotes++;
        }
        return s;
    }

    private static LinkedHashSet<SystemKind>detectRequested(String q){
        LinkedHashSet<SystemKind>out=new LinkedHashSet<>();
        for(SystemKind kind:SystemKind.values()){
            for(String term:kind.terms){
                String n=MusaAiDrawingIndex.normalize(term);
                if(containsPhrase(q,n)){out.add(kind);break;}
            }
        }
        if(containsAny(q,"sihhi tesisat","sihhi")){out.add(SystemKind.CLEAN_WATER);out.add(SystemKind.HOT_WATER);out.add(SystemKind.WASTE_WATER);}
        if(containsAny(q,"hvac")){out.add(SystemKind.HEATING);out.add(SystemKind.COOLING);out.add(SystemKind.VENTILATION);}
        return out;
    }

    private static boolean matches(SystemKind kind,MusaAiDrawingIndex.Item item){
        String layer=MusaAiDrawingIndex.normalize(item.layer),text=MusaAiDrawingIndex.normalize(item.text);
        for(String raw:kind.terms){
            String term=MusaAiDrawingIndex.normalize(raw);
            if(containsPhrase(layer,term)||containsPhrase(text,term))return true;
        }
        return false;
    }

    private static boolean isIssueCandidate(MusaAiDrawingIndex.Item item){
        return isDegenerate(item)||(isPolyline(item.type)&&item.closedKnown&&!item.closed)||hasReviewMarker(item.text);
    }
    private static boolean isDegenerate(MusaAiDrawingIndex.Item item){
        if(!item.hasLength())return false;
        return ("LINE".equals(item.type)||isPolyline(item.type)||"CIRCLE".equals(item.type)||"ARC".equals(item.type)||"ELLIPSE".equals(item.type))
            &&Math.abs(item.length)<=1e-9d;
    }
    private static boolean isPolyline(String type){return "POLYLINE".equals(type)||"LWPOLYLINE".equals(type);}
    private static boolean hasReviewMarker(String text){
        String q=MusaAiDrawingIndex.normalize(text);
        return containsAny(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et");
    }
    private static boolean containsPhrase(String hay,String needle){
        if(hay==null||needle==null||needle.isEmpty())return false;
        return (" "+hay+" ").contains(" "+needle+" ");
    }
    private static boolean containsAny(String q,String...terms){for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;}
    private static String joinLimited(Collection<String>items,int limit){
        ArrayList<String>x=new ArrayList<>(items);
        return String.join(", ",x.subList(0,Math.min(limit,x.size())))+(x.size()>limit?" …":"");
    }
    private static String unit(MusaAiDrawingIndex i){return i.unitName.isEmpty()?"çizim birimi":i.unitName;}
    private static String number(double v){
        DecimalFormatSymbols s=DecimalFormatSymbols.getInstance(new Locale("tr","TR"));
        return new DecimalFormat("#,##0.###",s).format(v);
    }
    private MusaAiMechanical(){}
}
