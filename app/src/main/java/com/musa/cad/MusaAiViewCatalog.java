package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/**
 * Candidate view inventory for a multi-plan DWG. Uses positioned CAD TEXT/MTEXT/
 * ATTRIB labels rather than assuming that each layout is only one floor plan.
 *
 * This intentionally does NOT infer the true extents of the view frames:
 * title positions, plan borders and the footprint of a section may differ.
 * Never present a found label as proof that its full engineering view was read.
 */
public final class MusaAiViewCatalog {
    public enum Kind { SITE, BASEMENT, GROUND, FLOOR, ROOF, SECTION, ELEVATION, DETAIL }

    public static final class View {
        public final Kind kind;
        public final String title,normalizedTitle;
        public final double x,y;
        public final int sourceId;
        private View(Kind kind,String title,double x,double y,int sourceId){
            this.kind=kind;this.title=title;this.normalizedTitle=MusaAiDrawingIndex.normalize(title);
            this.x=x;this.y=y;this.sourceId=sourceId;
        }
        public boolean positioned(){return Double.isFinite(x)&&Double.isFinite(y);}
    }

    public static final class LevelMark {
        public final String raw,context;
        public final int sourceId;
        public final double x,y;
        private LevelMark(String raw,String context,int sourceId,double x,double y){
            this.raw=raw;this.context=context;this.sourceId=sourceId;this.x=x;this.y=y;
        }
    }

    public static final class Result {
        public final List<View> views;
        public final List<LevelMark> levels;
        public final boolean sampled,truncated;
        public final int positionedViews,unpositionedViews;
        public final String report;
        private Result(List<View> views,List<LevelMark> levels,boolean sampled,boolean truncated,
                       int positionedViews,int unpositionedViews,String report){
            this.views=Collections.unmodifiableList(new ArrayList<>(views));
            this.levels=Collections.unmodifiableList(new ArrayList<>(levels));
            this.sampled=sampled;this.truncated=truncated;
            this.positionedViews=positionedViews;this.unpositionedViews=unpositionedViews;
            this.report=report;
        }
        public int count(Kind kind){int n=0;for(View v:views)if(v.kind==kind)n++;return n;}
    }

    private static final int MAX_VIEWS=30;
    private static final int MAX_LEVELS=50;
    private static final int MAX_SCANNED_TEXTS=8000;
    private static final Pattern SIGNED_LEVEL=Pattern.compile(
        "(?<![\\d])(?:\\+/-|±|\\+|-)\\s*\\d{1,3}[.,]\\d{1,3}(?!\\d)");
    private static final Pattern NAMED_LEVEL=Pattern.compile(
        "(?i)(?:\\bKOT\\b|\\bLEVEL\\b|\\bFFL\\b|\\bEL\\b)\\s*[:=]?\\s*[+-]?\\s*\\d{1,3}[.,]\\d{1,3}(?!\\d)");
    private static final Pattern NUMBERED_FLOOR=Pattern.compile(
        "(?:^|\\s)\\d{1,2}\\s*(?:normal\\s*)?kat(?:\\s|$)");
    private static final Pattern SECTION_ID=Pattern.compile(
        "(?:\\b[a-z0-9]{1,3}\\s*[-–]\\s*[a-z0-9]{1,3}\\s+kesit\\b|\\bkesit\\s+[a-z0-9]{1,3}\\b)");

    public static Result analyze(MusaAiDrawingIndex index){
        if(index==null)return new Result(Collections.emptyList(),Collections.emptyList(),true,false,0,0,
            "\n\nPLAN / KESİT / VAZİYET / KOT ENVANTERİ: DWG vektör indeksi hazır değil.");
        ArrayList<View> views=new ArrayList<>();
        ArrayList<LevelMark> levels=new ArrayList<>();
        int visited=0,unpositioned=0;boolean truncated=false;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null||item.text.isEmpty())continue;
            if(++visited>MAX_SCANNED_TEXTS){truncated=true;break;}
            final String raw=item.text.replace('\r',' ').replace('\n',' ').replaceAll("\\s+"," ").trim();
            final String normalized=MusaAiDrawingIndex.normalize(raw);
            Kind kind=classify(normalized);
            if(kind!=null){
                if(views.size()>=MAX_VIEWS){truncated=true;continue;}
                // Co-located duplicate text at identical drawing coordinates is one title.
                boolean duplicate=false;
                for(View previous:views){
                    if(previous.sourceId>=0&&previous.sourceId==item.sourceId){duplicate=true;break;}
                    if(previous.normalizedTitle.equals(normalized)&&previous.positioned()&&item.hasCenter()&&
                        Math.abs(previous.x-item.centerX)<0.0001 &&
                        Math.abs(previous.y-item.centerY)<0.0001){duplicate=true;break;}
                }
                if(!duplicate){
                    if(!item.hasCenter())unpositioned++;
                    views.add(new View(kind,raw.length()>160?raw.substring(0,160):raw,
                        item.centerX,item.centerY,item.sourceId));
                }
            }
            if(levels.size()<MAX_LEVELS){
                LinkedHashSet<String> rawMarks=new LinkedHashSet<>();
                // Prefer the complete named kot note, not two duplicate
                // records for "KOT: -3.20" and its nested "-3.20" substring.
                ArrayList<int[]> namedSpans=new ArrayList<>();
                Matcher named=NAMED_LEVEL.matcher(raw);
                while(named.find()&&rawMarks.size()<8){
                    rawMarks.add(named.group().trim());
                    namedSpans.add(new int[]{named.start(),named.end()});
                }
                Matcher signed=SIGNED_LEVEL.matcher(raw);
                while(signed.find()&&rawMarks.size()<8){
                    boolean inside=false;
                    for(int[] span:namedSpans)
                        if(signed.start()>=span[0]&&signed.end()<=span[1]){inside=true;break;}
                    if(!inside)rawMarks.add(signed.group().trim());
                }
                // Numeric expressions are only POTENTIAL levels; distinguish
                // pipe elevations, structural datum and linear dimensions later.
                for(String mark:rawMarks){
                    if(levels.size()>=MAX_LEVELS){truncated=true;break;}
                    levels.add(new LevelMark(mark,raw.length()>120?raw.substring(0,120):raw,
                        item.sourceId,item.centerX,item.centerY));
                }
            }else if(SIGNED_LEVEL.matcher(raw).find()||NAMED_LEVEL.matcher(raw).find()){
                truncated=true;
            }
        }
        views.sort((a,b)->{
            int k=Integer.compare(order(a.kind),order(b.kind));
            if(k!=0)return k;
            return a.title.compareToIgnoreCase(b.title);
        });

        int positioned=0;
        for(View v:views)if(v.positioned())positioned++;
        StringBuilder report=new StringBuilder("\n\nPLAN / KESİT / VAZİYET / KOT ENVANTERİ (DWG yazılarından aday tespiti)");
        report.append("\n• Açık düzen: ").append(index.layout)
            .append("; ").append(views.size()).append(" görünüm başlığı adayı, ")
            .append(levels.size()).append(" kot/irtifa notu adayı.");
        if(views.isEmpty())report.append("\n• Kat planı, kesit veya vaziyet başlığı güvenilir biçimde tespit edilemedi.");
        for(int i=0;i<Math.min(24,views.size());i++){
            View v=views.get(i);
            report.append("\n• ").append(kindName(v.kind)).append(": ").append(v.title);
            if(v.sourceId>=0)report.append(" [kaynak ").append(v.sourceId).append("]");
            if(!v.positioned())report.append(" [koordinatsız]");
        }
        if(views.size()>24)report.append("\n• Diğer görünüm başlığı adayları rapor sınırı dışında.");
        for(int i=0;i<Math.min(12,levels.size());i++){
            LevelMark mark=levels.get(i);
            report.append("\n• Kot adayı: ").append(mark.raw);
            if(mark.sourceId>=0)report.append(" [kaynak ").append(mark.sourceId).append("]");
        }
        if(levels.size()>12)report.append("\n• Diğer kot adayları özet dışında.");
        report.append("\n• Kontrol gündemi: kat planları ile kesitlerin kot/düşey süreklilik uyumu;")
              .append(" vaziyet planında bina–şebeke bağlantı yerleri; çatı planında havalık/yağmur suyu;")
              .append(" bodrumda atık su tahliyesi ve ekipman seviyeleri.");
        report.append("\n• Bu liste bir BAŞLIK ENVANTERİDİR. Sadece başlık metninden")
              .append(" tüm kat, kesit veya vaziyet planının görsel olarak tarandığı iddia edilemez.");
        report.append(" Kotların referans sistemi, birimi, işaret ve ölçü doğruluğu doğrulanmadan çelişki kararı verilemez.");
        if(truncated)report.append("\n• Örnekleme sınırına ulaşıldı; bu envanter eksik olabilir.");
        return new Result(views,levels,true,truncated,positioned,unpositioned,report.toString());
    }

    private static void collect(Pattern p,String raw,Set<String> into){
        Matcher m=p.matcher(raw);
        while(m.find()&&into.size()<8)into.add(m.group().trim());
    }
    public static Kind classify(String normalized){
        if(normalized==null||normalized.isEmpty())return null;
        if(normalized.contains("vaziyet plani")||normalized.contains("yerlesim plani")||
            normalized.contains("site plan")||normalized.contains("site layout"))
            return Kind.SITE;
        if(normalized.contains("kesit")||
            SECTION_ID.matcher(normalized).find())return Kind.SECTION;
        if(normalized.contains("gorunus")||normalized.contains("elevation view"))
            return Kind.ELEVATION;
        if(normalized.contains("cati")&&(normalized.contains("plan")||
            normalized.contains("tesisat")||normalized.contains("drenaj")))return Kind.ROOF;
        if(normalized.contains("bodrum")&&(normalized.contains("kat")||
            normalized.contains("plan")))return Kind.BASEMENT;
        if(normalized.contains("zemin kat")||normalized.contains("giris kat")||
            normalized.contains("giris plani")||
            (normalized.contains("zemin")&&normalized.contains("tesisat")&&
                normalized.contains("plan")))return Kind.GROUND;
        if(normalized.contains("detay")&&
            (normalized.contains("plan")||normalized.contains("kesit")||normalized.contains("olcek")))
            return Kind.DETAIL;
        boolean namedFloor=(normalized.contains("normal kat")||
            normalized.contains("tip kat")||normalized.contains("asma kat")||
            normalized.contains("teras kat")||NUMBERED_FLOOR.matcher(normalized).find());
        boolean aPlan=normalized.contains("plan")||normalized.contains("plani")||normalized.contains("tesisat");
        if((namedFloor&&aPlan)||
            (normalized.contains("kat plani")&&normalized.length()<135))
            return Kind.FLOOR;
        return null;
    }
    private static int order(Kind kind){
        switch(kind){
            case SITE:return 0;case SECTION:return 1;case ROOF:return 2;
            case BASEMENT:return 3;case GROUND:return 4;case FLOOR:return 5;
            case ELEVATION:return 6;default:return 7;
        }
    }
    private static String kindName(Kind kind){
        switch(kind){
            case SITE:return "Vaziyet / yerleşim";case BASEMENT:return "Bodrum kat";
            case GROUND:return "Zemin kat";case FLOOR:return "Kat planı";
            case ROOF:return "Çatı planı";case SECTION:return "Kesit";
            case ELEVATION:return "Görünüş";default:return "Detay";
        }
    }
    private MusaAiViewCatalog(){}
}
