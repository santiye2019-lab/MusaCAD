package com.musa.cad;

import java.util.*;

/**
 * Multi-file project-package audit for simultaneously open MusaCAD drawings.
 * Cross-drawing geometry checks are deliberately conservative: proximity is
 * reported as a coordination candidate, never as a definitive clash.
 */
public final class MusaAiProjectPackage {
    public enum Severity { HIGH,MEDIUM,INFO }

    public static final class Drawing {
        public final String name;
        public final MusaAiDrawingIndex index;
        public final MusaAiBoq.Model boq;
        public final MusaAiDiscipline discipline;
        public final int disciplineScore;
        public Drawing(String name,MusaAiDrawingIndex index,MusaAiBoq.Model boq){
            this.name=clean(name).isEmpty()?"Adsız çizim":clean(name);
            this.index=index;this.boq=boq;
            Detection d=detectDiscipline(this.name,index);
            this.discipline=d.discipline;this.disciplineScore=d.score;
        }
    }

    public static final class Finding {
        public final String id;
        public final Severity severity;
        public final String title,detail;
        public final String drawingA,drawingB;
        Finding(String id,Severity severity,String title,String detail,String drawingA,String drawingB){
            this.id=id;this.severity=severity;this.title=clean(title);this.detail=clean(detail);
            this.drawingA=clean(drawingA);this.drawingB=clean(drawingB);
        }
    }

    public static final class Result {
        public final boolean matched;
        public final String title,text;
        public final List<Finding> findings;
        public final int drawingCount,detectedDisciplineCount;
        Result(boolean matched,String title,String text,Collection<Finding>findings,int drawingCount,int detectedDisciplineCount){
            this.matched=matched;this.title=clean(title);this.text=text==null?"":text;
            this.findings=Collections.unmodifiableList(new ArrayList<>(findings==null?Collections.emptyList():findings));
            this.drawingCount=drawingCount;this.detectedDisciplineCount=detectedDisciplineCount;
        }
        public static Result none(){return new Result(false,"","",Collections.emptyList(),0,0);}
    }

    private static final class Detection {
        final MusaAiDiscipline discipline;final int score;
        Detection(MusaAiDiscipline discipline,int score){this.discipline=discipline;this.score=score;}
    }

    private static final class Unit {
        final String key;final double meters;
        Unit(String key,double meters){this.key=key;this.meters=meters;}
        boolean known(){return meters>0d;}
    }

    private static final class PointItem {
        final MusaAiDrawingIndex.Item item;final double xMeters,yMeters;
        PointItem(MusaAiDrawingIndex.Item item,double xMeters,double yMeters){
            this.item=item;this.xMeters=xMeters;this.yMeters=yMeters;
        }
    }

    public static boolean asksPackageReview(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        return q.contains("proje paketi")||
            q.contains("tum acik proje")||
            q.contains("tum acik cizim")||
            q.contains("disiplinler arasi tam")||
            q.contains("tum disiplin dosya")||
            q.contains("tam proje denetim raporu")||
            q.equals("tam proje denetimi")||
            q.equals("tam proje denetim");
    }

    public static Result generate(Collection<Drawing>source,String raw){
        ArrayList<Drawing>drawings=new ArrayList<>();
        if(source!=null)for(Drawing d:source)if(d!=null&&d.index!=null)drawings.add(d);
        if(drawings.isEmpty())return Result.none();

        ArrayList<Finding>findings=new ArrayList<>();
        EnumMap<MusaAiDiscipline,Integer>disciplineCounts=new EnumMap<>(MusaAiDiscipline.class);
        for(Drawing d:drawings)disciplineCounts.put(d.discipline,disciplineCounts.getOrDefault(d.discipline,0)+1);
        int seq=1;

        // Coverage means "loaded/detected in the current package", not "missing from the actual project".
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            if(disciplineCounts.getOrDefault(d,0)==0){
                findings.add(new Finding("PKG-"+pad(seq++),Severity.INFO,
                    d.label+" dosyası yüklenmedi / algılanmadı",
                    "Açık proje paketinde "+d.label+" olarak güvenilir biçimde sınıflandırılan vektör çizim bulunmuyor. Bu, gerçek projede bu disiplinin olmadığı anlamına gelmez.",
                    "",""));
            }
        }

        for(Drawing d:drawings){
            if(d.discipline==MusaAiDiscipline.UNKNOWN){
                findings.add(new Finding("PKG-"+pad(seq++),Severity.INFO,
                    "Disiplin sınıflandırması belirsiz",
                    d.name+" dosyası katman, dosya adı ve görünür metinlerden güvenilir biçimde tek bir disipline atanamadı.",
                    d.name,""));
                continue;
            }
            MusaAiDisciplineAnalyzer.Result analysis=MusaAiDisciplineAnalyzer.analyzeDiscipline(d.index,d.discipline);
            int high=0,medium=0;
            for(MusaAiDisciplineAnalyzer.Finding f:analysis.findings){
                if(f.severity==MusaAiDisciplineAnalyzer.Severity.CRITICAL||f.severity==MusaAiDisciplineAnalyzer.Severity.HIGH)high++;
                else if(f.severity==MusaAiDisciplineAnalyzer.Severity.MEDIUM)medium++;
            }
            if(high>0||medium>0){
                findings.add(new Finding("PKG-"+pad(seq++),high>0?Severity.HIGH:Severity.MEDIUM,
                    d.discipline.label+" paftasında kontrol adayları",
                    d.name+" • yüksek/kritik: "+high+" • orta: "+medium+
                    ". Ayrıntılar disiplin raporunda ve ilgili paftada doğrulanmalıdır.",
                    d.name,""));
            }
        }

        // Per-file BOQ comparison.
        for(Drawing d:drawings){
            if(d.boq==null||d.boq.isEmpty())continue;
            MusaAiBoq.Model generated=MusaAiBoq.generate(d.index,d.name+" • otomatik proje metrajı");
            MusaAiBoq.Comparison cmp=MusaAiBoq.compare(d.boq,generated);
            if(cmp.different>0||cmp.unmatchedBoq>0||cmp.projectOnly>0){
                findings.add(new Finding("PKG-"+pad(seq++),Severity.MEDIUM,
                    "Proje–keşif inceleme adayı",
                    d.name+" • eşleşen: "+cmp.compared+" • fark/birim incelemesi: "+cmp.different+
                    " • keşifte eşleşmeyen: "+cmp.unmatchedBoq+" • projede eşleşmeyen: "+cmp.projectOnly+".",
                    d.name,""));
            }
        }

        // Cross-drawing coordination.
        for(int i=0;i<drawings.size();i++){
            for(int j=i+1;j<drawings.size();j++){
                Drawing a=drawings.get(i),b=drawings.get(j);
                if(!relevantPair(a.discipline,b.discipline))continue;

                Unit ua=unit(a.index.unitName),ub=unit(b.index.unitName);
                if(!ua.known()||!ub.known()){
                    findings.add(new Finding("PKG-"+pad(seq++),Severity.INFO,
                        "Geometrik çapraz kontrol için birim doğrulaması gerekli",
                        a.name+" ("+unitLabel(a.index.unitName)+") ↔ "+b.name+" ("+unitLabel(b.index.unitName)+
                        "): çizim birimlerinden en az biri güvenilir biçimde metreye çevrilemiyor. Otomatik yakınlık/çakışma sonucu üretilmedi.",
                        a.name,b.name));
                    continue;
                }

                int candidates=proximityCandidates(a,b,ua,ub);
                if(candidates>0){
                    findings.add(new Finding("PKG-"+pad(seq++),Severity.MEDIUM,
                        "Disiplinler arası yakınlık / koordinasyon adayları",
                        a.discipline.label+" • "+a.name+" ↔ "+b.discipline.label+" • "+b.name+
                        ": aynı koordinat sisteminde olduğu varsayımıyla "+candidates+
                        " adet yakınlık adayı bulundu. Bu sonuç kesin çakışma değildir; kat, kot, eleman geometrisi ve ortak referans noktası doğrulanmalıdır.",
                        a.name,b.name));
                }else if(!sameUnitScale(ua,ub)){
                    findings.add(new Finding("PKG-"+pad(seq++),Severity.INFO,
                        "Dosya birimleri farklı ancak dönüştürülebilir",
                        a.name+" ("+unitLabel(a.index.unitName)+") ↔ "+b.name+" ("+unitLabel(b.index.unitName)+
                        "): birimler metreye normalize edilerek karşılaştırıldı; yakınlık adayı bulunmadı.",
                        a.name,b.name));
                }
            }
        }

        String title="Proje Paketi • Tam Proje Denetim Raporu";
        StringBuilder out=new StringBuilder();
        out.append("MUSACAD AI\n").append(title.toUpperCase(new Locale("tr","TR")));
        out.append("\n========================================");
        out.append("\nAçık vektör çizim: ").append(drawings.size());
        int detected=0;for(MusaAiDiscipline d:MusaAiDiscipline.engineering())if(disciplineCounts.getOrDefault(d,0)>0)detected++;
        out.append("\nAlgılanan disiplin türü: ").append(detected);
        out.append("\nRapor tipi: çoklu açık proje paketi ön denetimi");

        out.append("\n\n1. PROJE PAKETİ ENVANTERİ");
        for(Drawing d:drawings){
            out.append("\n• ").append(d.name)
               .append(" → ").append(d.discipline.label)
               .append(" • layout: ").append(blank(d.index.layout,"aktif layout"))
               .append(" • nesne: ").append(d.index.entityCount)
               .append(" • birim: ").append(unitLabel(d.index.unitName));
            if(d.boq!=null&&!d.boq.isEmpty())out.append(" • keşif: ").append(d.boq.rows.size()).append(" satır");
        }

        out.append("\n\n2. DİSİPLİN KAPSAMI");
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            int count=disciplineCounts.getOrDefault(d,0);
            out.append("\n• ").append(d.label).append(": ").append(count>0?count+" açık çizim":"yüklenmedi / algılanmadı");
        }

        out.append("\n\n3. DOSYA BAZLI TEKNİK ÖN KONTROL");
        for(Drawing d:drawings){
            if(d.discipline==MusaAiDiscipline.UNKNOWN){
                out.append("\n• ").append(d.name).append(": disiplin belirsiz.");
                continue;
            }
            MusaAiDisciplineAnalyzer.Result analysis=MusaAiDisciplineAnalyzer.analyzeDiscipline(d.index,d.discipline);
            out.append("\n• ").append(d.name).append(" • ").append(d.discipline.label)
               .append(": ").append(analysis.findings.size()).append(" inceleme adayı.");
        }

        out.append("\n\n4. PROJE–KEŞİF UYUMLULUĞU");
        boolean anyBoq=false;
        for(Drawing d:drawings){
            if(d.boq==null||d.boq.isEmpty())continue;anyBoq=true;
            MusaAiBoq.Comparison cmp=MusaAiBoq.compare(d.boq,MusaAiBoq.generate(d.index,d.name+" • otomatik proje metrajı"));
            out.append("\n• ").append(d.name)
               .append(": eşleşen ").append(cmp.compared)
               .append(" • fark/birim ").append(cmp.different)
               .append(" • keşifte eşleşmeyen ").append(cmp.unmatchedBoq)
               .append(" • projede eşleşmeyen ").append(cmp.projectOnly);
        }
        if(!anyBoq)out.append("\n• Açık çizimlerin hiçbirine keşif bağlanmamış.");

        out.append("\n\n5. DİSİPLİNLER ARASI KOORDİNASYON");
        int cross=0;
        for(Finding f:findings){
            if(f.drawingB.isEmpty())continue;cross++;
            out.append("\n• [").append(f.id).append("] ").append(severity(f.severity)).append(" • ")
               .append(f.title).append("\n  ").append(f.detail);
        }
        if(cross==0)out.append("\n• Otomatik çapraz kontrol adayı oluşmadı. Bu durum disiplinlerin uyumlu olduğunu tek başına kanıtlamaz.");

        out.append("\n\n6. TÜM BULGULAR");
        int shown=0;
        for(Finding f:findings){
            if(shown++>=80){out.append("\n• … kalan bulgular rapor ekinde ayrıntılandırılabilir.");break;}
            out.append("\n• [").append(f.id).append("] ").append(severity(f.severity)).append(" • ")
               .append(f.title).append(" — ").append(f.detail);
        }

        out.append("\n\n7. SINIRLAR VE SONUÇ");
        out.append("\n• Çoklu dosya geometrik karşılaştırması yalnız bilinen çizim birimleri ve mevcut koordinat bilgisiyle yapılır.");
        out.append("\n• Yakınlık adayı kesin çakışma değildir; ortak koordinat sistemi, kat/kot, gerçek eleman sınırı ve referans noktaları doğrulanmalıdır.");
        out.append("\n• Yüklenmeyen bir disiplin “projede yok” olarak yorumlanmaz.");
        out.append("\n• Proje–keşif sonuçları taslak metraj ve otomatik metin/katman eşleştirmesidir; ihale, hakediş veya resmi onay değildir.");
        out.append("\n• Statik güvenlik, yangın mevzuatı, elektrik güvenliği, asansör uygunluğu ve diğer resmi mühendislik kararları yetkili kişilerce doğrulanmalıdır.");

        return new Result(true,title,out.toString(),findings,drawings.size(),detected);
    }

    static MusaAiDiscipline primaryDiscipline(String name,MusaAiDrawingIndex index){
        return detectDiscipline(name,index).discipline;
    }

    private static Detection detectDiscipline(String name,MusaAiDrawingIndex index){
        EnumMap<MusaAiDiscipline,Integer>score=new EnumMap<>(MusaAiDiscipline.class);
        String normalizedName=MusaAiDrawingIndex.normalize(name);
        MusaAiDiscipline file=MusaAiDiscipline.classify(normalizedName);
        if(file!=MusaAiDiscipline.UNKNOWN)score.put(file,score.getOrDefault(file,0)+18);
        // Extra filename hints are intentionally high-weight.
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            if(filenameHint(d,normalizedName))score.put(d,score.getOrDefault(d,0)+24);
        }
        if(index!=null){
            for(String layer:index.allLayers){
                MusaAiDiscipline d=MusaAiDiscipline.classify(layer);
                if(d!=MusaAiDiscipline.UNKNOWN)score.put(d,score.getOrDefault(d,0)+3);
            }
            int sampled=0;
            for(MusaAiDrawingIndex.Item item:index.items()){
                if(item==null)continue;
                MusaAiDiscipline d=MusaAiDiscipline.classify(item.layer+" "+item.text);
                if(d!=MusaAiDiscipline.UNKNOWN)score.put(d,score.getOrDefault(d,0)+1);
                if(++sampled>=12000)break;
            }
        }
        MusaAiDiscipline best=MusaAiDiscipline.UNKNOWN;int bestScore=0,second=0;
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            int s=score.getOrDefault(d,0);
            if(s>bestScore){second=bestScore;bestScore=s;best=d;}
            else if(s>second)second=s;
        }
        if(bestScore<3)return new Detection(MusaAiDiscipline.UNKNOWN,bestScore);
        // Ambiguous low-margin classification without a strong filename cue remains unknown.
        if(bestScore-second<2&&!filenameHint(best,normalizedName))return new Detection(MusaAiDiscipline.UNKNOWN,bestScore);
        return new Detection(best,bestScore);
    }

    private static boolean filenameHint(MusaAiDiscipline d,String q){
        switch(d){
            case ARCHITECTURAL:return has(q,"mimari","architect","architecture");
            case STRUCTURAL:return has(q,"statik","structural","betonarme");
            case MECHANICAL:return has(q,"mekanik","mechanical","hvac","sihhi");
            case ELECTRICAL:return has(q,"elektrik","electrical","kuvvetli akim","zayif akim");
            case LANDSCAPE:return has(q,"peyzaj","landscape");
            case INFRASTRUCTURE:return has(q,"altyapi","infrastructure");
            case ELEVATOR:return has(q,"asansor","elevator","lift");
            case FIRE_SAFETY:return has(q,"yangin","fire","sprinkler");
            default:return false;
        }
    }

    private static boolean relevantPair(MusaAiDiscipline a,MusaAiDiscipline b){
        if(a==MusaAiDiscipline.UNKNOWN||b==MusaAiDiscipline.UNKNOWN||a==b)return false;
        return pair(a,b,MusaAiDiscipline.STRUCTURAL,MusaAiDiscipline.MECHANICAL)||
            pair(a,b,MusaAiDiscipline.STRUCTURAL,MusaAiDiscipline.ELECTRICAL)||
            pair(a,b,MusaAiDiscipline.STRUCTURAL,MusaAiDiscipline.FIRE_SAFETY)||
            pair(a,b,MusaAiDiscipline.STRUCTURAL,MusaAiDiscipline.ELEVATOR)||
            pair(a,b,MusaAiDiscipline.ARCHITECTURAL,MusaAiDiscipline.STRUCTURAL)||
            pair(a,b,MusaAiDiscipline.ARCHITECTURAL,MusaAiDiscipline.MECHANICAL)||
            pair(a,b,MusaAiDiscipline.ARCHITECTURAL,MusaAiDiscipline.ELECTRICAL)||
            pair(a,b,MusaAiDiscipline.ARCHITECTURAL,MusaAiDiscipline.ELEVATOR)||
            pair(a,b,MusaAiDiscipline.ARCHITECTURAL,MusaAiDiscipline.FIRE_SAFETY)||
            pair(a,b,MusaAiDiscipline.MECHANICAL,MusaAiDiscipline.ELECTRICAL)||
            pair(a,b,MusaAiDiscipline.LANDSCAPE,MusaAiDiscipline.INFRASTRUCTURE)||
            pair(a,b,MusaAiDiscipline.ELECTRICAL,MusaAiDiscipline.ELEVATOR)||
            pair(a,b,MusaAiDiscipline.ELECTRICAL,MusaAiDiscipline.FIRE_SAFETY);
    }

    private static boolean pair(MusaAiDiscipline a,MusaAiDiscipline b,MusaAiDiscipline x,MusaAiDiscipline y){
        return (a==x&&b==y)||(a==y&&b==x);
    }

    private static int proximityCandidates(Drawing a,Drawing b,Unit ua,Unit ub){
        // 25 cm is a review radius, not a clash tolerance.
        final double tolerance=0.25d;
        List<PointItem>aa=points(a.index,ua,4000),bb=points(b.index,ub,4000);
        if(aa.isEmpty()||bb.isEmpty())return 0;

        // Bucket B to avoid an O(n²) all-pairs scan.
        final double cell=tolerance;
        HashMap<Long,ArrayList<PointItem>>grid=new HashMap<>();
        for(PointItem p:bb){
            long gx=(long)Math.floor(p.xMeters/cell),gy=(long)Math.floor(p.yMeters/cell);
            grid.computeIfAbsent(hash(gx,gy),k->new ArrayList<>()).add(p);
        }
        int count=0;
        for(PointItem p:aa){
            long gx=(long)Math.floor(p.xMeters/cell),gy=(long)Math.floor(p.yMeters/cell);
            boolean hit=false;
            for(long dx=-1;dx<=1&&!hit;dx++)for(long dy=-1;dy<=1&&!hit;dy++){
                List<PointItem>bucket=grid.get(hash(gx+dx,gy+dy));if(bucket==null)continue;
                for(PointItem q:bucket){
                    double x=p.xMeters-q.xMeters,y=p.yMeters-q.yMeters;
                    if(x*x+y*y<=tolerance*tolerance){hit=true;break;}
                }
            }
            if(hit&&++count>=250)return count;
        }
        return count;
    }

    private static List<PointItem>points(MusaAiDrawingIndex index,Unit unit,int max){
        ArrayList<PointItem>out=new ArrayList<>();
        if(index==null||unit==null||!unit.known())return out;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null||!item.hasCenter())continue;
            MusaAiDiscipline d=MusaAiDiscipline.classify(item.layer+" "+item.text);
            if(d==MusaAiDiscipline.UNKNOWN&&item.geometryKey.isEmpty())continue;
            out.add(new PointItem(item,item.centerX*unit.meters,item.centerY*unit.meters));
            if(out.size()>=max)break;
        }
        return out;
    }

    private static Unit unit(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).replace(" ","");
        if(q.equals("m")||q.equals("metre")||q.equals("meter"))return new Unit("m",1d);
        if(q.equals("cm")||q.equals("santimetre")||q.equals("centimeter"))return new Unit("cm",0.01d);
        if(q.equals("mm")||q.equals("milimetre")||q.equals("millimeter"))return new Unit("mm",0.001d);
        return new Unit(q,Double.NaN);
    }

    private static boolean sameUnitScale(Unit a,Unit b){
        return a!=null&&b!=null&&a.known()&&b.known()&&Math.abs(a.meters-b.meters)<1e-12;
    }

    private static long hash(long x,long y){
        return (x*73856093L)^(y*19349663L);
    }

    private static String severity(Severity s){
        switch(s){case HIGH:return "YÜKSEK";case MEDIUM:return "ORTA";default:return "BİLGİ";}
    }
    private static String unitLabel(String s){return clean(s).isEmpty()?"belirsiz":clean(s);}
    private static String blank(String s,String fallback){return clean(s).isEmpty()?fallback:clean(s);}
    private static String clean(String s){return s==null?"":s.trim();}
    private static String pad(int n){return String.format(Locale.ROOT,"%03d",n);}
    private static boolean has(String q,String...terms){
        if(q==null)return false;for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;
    }
    private MusaAiProjectPackage(){}
}
