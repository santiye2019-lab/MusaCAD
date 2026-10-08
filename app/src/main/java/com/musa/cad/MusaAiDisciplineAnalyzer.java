package com.musa.cad;

import java.util.*;

/**
 * Multi-discipline deterministic review over the visible CAD index.
 *
 * The engine intentionally reports review candidates. It never certifies
 * structural safety or statutory compliance from a drawing alone.
 */
public final class MusaAiDisciplineAnalyzer {
    public enum Severity { CRITICAL,HIGH,MEDIUM,INFO }

    public static final class Finding {
        public final String id,title,detail,suggestion;
        public final MusaAiDiscipline discipline;
        public final Severity severity;
        public final List<Integer> sourceIds;
        Finding(String id,MusaAiDiscipline discipline,Severity severity,String title,String detail,String suggestion,Collection<Integer>ids){
            this.id=id;this.discipline=discipline;this.severity=severity;this.title=title;this.detail=detail;this.suggestion=suggestion;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
        }
    }

    public static final class Result {
        public final boolean matched;
        public final String text;
        public final List<Finding> findings;
        public final List<Integer> sourceIds;
        public final MusaAiDiscipline discipline;
        Result(boolean matched,String text,MusaAiDiscipline discipline,Collection<Finding>findings,Collection<Integer>ids){
            this.matched=matched;this.text=text==null?"":text;this.discipline=discipline;
            this.findings=Collections.unmodifiableList(new ArrayList<>(findings));
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));
        }
        public static Result none(){return new Result(false,"",MusaAiDiscipline.UNKNOWN,Collections.emptyList(),Collections.emptyList());}
    }

    /** Recognizes ordinary voice/text requests to review the active drawing as a whole. */
    public static boolean asksGeneralProjectAnalysis(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||MusaAiDiscipline.fromQuery(raw)!=MusaAiDiscipline.UNKNOWN)return false;
        boolean project=q.contains("proje")||q.contains("cizim")||q.contains("pafta");
        boolean review=q.contains("analiz")||q.contains("incele")||q.contains("kontrol")||q.contains("denet");
        return project&&review;
    }

    public static boolean asksAnalysis(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        if(q.contains("tam proje denet")||q.contains("tum disiplin")||q.contains("disiplinler arasi"))return true;
        MusaAiDiscipline d=MusaAiDiscipline.fromQuery(q);
        return d!=MusaAiDiscipline.UNKNOWN&&(q.contains("kontrol")||q.contains("incele")||q.contains("analiz")||q.contains("rapor")||q.contains("denet"));
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        if(index==null||!asksAnalysis(raw))return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        boolean all=q.contains("tam proje")||q.contains("tum disiplin")||q.contains("disiplinler arasi")||q.contains("multidisiplin");
        if(all)return analyzeAll(index);
        MusaAiDiscipline d=MusaAiDiscipline.fromQuery(raw);
        if(d==MusaAiDiscipline.UNKNOWN)return Result.none();
        return analyzeDiscipline(index,d);
    }

    public static Result analyzeAll(MusaAiDrawingIndex index){
        if(index==null)return Result.none();
        ArrayList<Finding> all=new ArrayList<>();LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        StringBuilder out=new StringBuilder("MUSACAD AI • ÇOK DİSİPLİNLİ PROJE DENETİMİ");
        out.append("\nLayout: ").append(blank(index.layout,"aktif layout"));
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            Result r=analyzeDiscipline(index,d);
            all.addAll(r.findings);ids.addAll(r.sourceIds);
            out.append("\n\n").append(d.label.toUpperCase(new Locale("tr","TR"))).append("\n");
            out.append(summaryLine(r));
        }
        out.append("\n\nToplam inceleme adayı: ").append(all.size());
        out.append("\nNot: Otomatik çizim/metadata taramasıdır; mühendislik hesabı, statik güvenlik onayı veya resmi mevzuat uygunluk belgesi değildir.");
        return new Result(true,out.toString(),MusaAiDiscipline.UNKNOWN,all,ids);
    }

    public static Result analyzeDiscipline(MusaAiDrawingIndex index,MusaAiDiscipline discipline){
        if(index==null||discipline==null||discipline==MusaAiDiscipline.UNKNOWN)return Result.none();
        ArrayList<MusaAiDrawingIndex.Item> matched=new ArrayList<>();
        ArrayList<MusaAiDrawingIndex.Item> reviewMarkers=new ArrayList<>();
        EnumMap<Severity,Integer> severityCount=new EnumMap<>(Severity.class);
        ArrayList<Finding> findings=new ArrayList<>();LinkedHashSet<Integer>ids=new LinkedHashSet<>();

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=item.layer+" "+item.text;
            MusaAiDiscipline inferred=MusaAiDiscipline.classify(hay);
            if(inferred==discipline || crossBelongs(discipline,hay)){
                matched.add(item);
                if(hasReviewMarker(item.text+" "+item.layer))reviewMarkers.add(item);
            }
        }

        int seq=1;
        if(matched.isEmpty()){
            findings.add(finding(discipline,Severity.INFO,seq++,
                "Disiplin verisi otomatik tanınamadı",
                "Görünür katman ve çizim metinlerinden "+discipline.label+" öğesi güvenilir biçimde sınıflandırılamadı.",
                "Katman adları, lejant ve disiplin paftalarının görünür olduğundan emin olun; gerekirse Gandalf derin analizini kullanın.",
                Collections.emptyList()));
        }else{
            LinkedHashSet<Integer> markerIds=new LinkedHashSet<>();addIds(markerIds,reviewMarkers);
            if(!reviewMarkers.isEmpty()){
                findings.add(finding(discipline,Severity.HIGH,seq++,
                    "Revizyon / eksik notu bulundu",
                    reviewMarkers.size()+" adet TODO, TBD, EKSİK, REVİZE, DÜZELT veya benzeri kontrol notu disiplin verisiyle ilişkili görünüyor.",
                    "İlgili pafta ve imalatları proje müellifi/koordinasyon ekibiyle doğrulayın.",
                    markerIds));
                ids.addAll(markerIds);
            }
            seq=appendDisciplineChecks(index,discipline,matched,findings,ids,seq);
        }

        for(Finding f:findings)severityCount.put(f.severity,severityCount.getOrDefault(f.severity,0)+1);
        StringBuilder out=new StringBuilder();
        out.append("MUSACAD AI • ").append(discipline.label.toUpperCase(new Locale("tr","TR"))).append(" PROJE KONTROLÜ");
        out.append("\n• Tanınan ilgili nesne: ").append(matched.size());
        out.append("\n• Bulgu / inceleme adayı: ").append(findings.size());
        for(Severity s:Severity.values())if(severityCount.getOrDefault(s,0)>0)out.append(" • ").append(severityLabel(s)).append(": ").append(severityCount.get(s));
        for(Finding f:findings){
            out.append("\n\n[").append(f.id).append("] ").append(severityLabel(f.severity)).append(" • ").append(f.title);
            out.append("\n").append(f.detail);
            if(!f.suggestion.isEmpty())out.append("\nÖneri: ").append(f.suggestion);
        }
        out.append("\n\nNot: Sonuç çizimde görünür veri üzerinden otomatik ön kontroldür.");
        if(discipline==MusaAiDiscipline.STRUCTURAL)
            out.append(" Statik güvenlik kararı için hesap modeli/raporu, malzeme sınıfları, yükler ve mühendis onayı ayrıca gereklidir.");
        return new Result(true,out.toString(),discipline,findings,ids);
    }

    private static int appendDisciplineChecks(MusaAiDrawingIndex index,MusaAiDiscipline d,List<MusaAiDrawingIndex.Item>items,List<Finding>out,Set<Integer>ids,int seq){
        switch(d){
            case STRUCTURAL:{
                int kolon=count(items,"kolon"),kiris=count(items,"kiris"),perde=count(items,"perde"),doseme=count(items,"doseme"),temel=count(items,"temel","radye","kazik");
                if(kolon+kiris+perde+doseme+temel==0)
                    out.add(finding(d,Severity.MEDIUM,seq++,"Taşıyıcı eleman sınıfları ayrıştırılamadı","Statik olarak sınıflanan nesnelerde kolon/kiriş/perde/döşeme/temel isimleri okunamadı.","Statik pafta lejantı ve katman isimlerini kontrol edin.",ids(items)));
                List<MusaAiDrawingIndex.Item> holes=filter(items,"delik","rezervasyon","bosluk","gecis");
                if(!holes.isEmpty()){Set<Integer>x=ids(holes);ids.addAll(x);out.add(finding(d,Severity.HIGH,seq++,"Rezervasyon / geçiş koordinasyonu","Taşıyıcı sistemle ilişkili "+holes.size()+" adet delik, boşluk, rezervasyon veya geçiş ifadesi bulundu.","Mekanik/elektrik geçişlerini statik müellifle doğrulayın; sahada onaysız taşıyıcı eleman delinmemelidir.",x));}

                MusaAiStructuralAdvanced.Result advanced=MusaAiStructuralAdvanced.analyze(index,null);
                if(advanced.matched)for(MusaAiStructuralAdvanced.Finding af:advanced.findings){
                    Severity severity=advancedSeverity(af.status);
                    ids.addAll(af.sourceIds);
                    out.add(new Finding(af.id,d,severity,af.title,af.detail,af.suggestion,af.sourceIds));
                }
                break;
            }
            case ELECTRICAL:{
                List<MusaAiDrawingIndex.Item> mechFeed=filter(items,"mekanik besleme","pompa besleme","fan besleme","vrf besleme","asansor besleme");
                if(!mechFeed.isEmpty()){Set<Integer>x=ids(mechFeed);ids.addAll(x);out.add(finding(d,Severity.MEDIUM,seq++,"Disiplinler arası besleme koordinasyonu",mechFeed.size()+" adet mekanik/asansör ekipman besleme ifadesi bulundu.","Güç, kumanda, kablo güzergâhı ve pano yüklerini ilgili disiplinlerle karşılaştırın.",x));}
                break;
            }
            case MECHANICAL:{
                List<MusaAiDrawingIndex.Item> slope=filter(items,"egim","slope");
                List<MusaAiDrawingIndex.Item> shaft=filter(items,"saft","shaft","gecis");
                if(!slope.isEmpty()||!shaft.isEmpty()){Set<Integer>x=ids(slope);x.addAll(ids(shaft));ids.addAll(x);out.add(finding(d,Severity.MEDIUM,seq++,"Güzergâh / kot koordinasyon adayları",(slope.size()+shaft.size())+" adet eğim, şaft veya geçiş ifadesi bulundu.","Mimari ve statik kot/şaft bilgileriyle karşılaştırın.",x));}
                break;
            }
            case ARCHITECTURAL:{
                List<MusaAiDrawingIndex.Item> access=filter(items,"engelli","erisilebilir","rampa","kacis","yangin kapisi");
                if(!access.isEmpty()){Set<Integer>x=ids(access);ids.addAll(x);out.add(finding(d,Severity.INFO,seq++,"Erişilebilirlik / kaçış kontrol alanı",access.size()+" adet erişilebilirlik, rampa, kaçış veya yangın kapısı ifadesi bulundu.","Ölçü, net geçiş ve ilgili disiplinlerle sürekliliği kontrol edin.",x));}
                break;
            }
            case LANDSCAPE:{
                List<MusaAiDrawingIndex.Item> conflict=filter(items,"drenaj","altyapi","hat","rogar","sulama");
                if(!conflict.isEmpty()){Set<Integer>x=ids(conflict);ids.addAll(x);out.add(finding(d,Severity.MEDIUM,seq++,"Peyzaj–altyapı koordinasyonu",conflict.size()+" adet sulama/drenaj/altyapı ilişkili öğe bulundu.","Ağaç kök bölgesi, sulama ve altyapı hatlarının kot/güzergâh uyumunu kontrol edin.",x));}
                break;
            }
            case INFRASTRUCTURE:{
                List<MusaAiDrawingIndex.Item> levels=filter(items,"kot","egim","baglanti","rogar");
                if(!levels.isEmpty()){Set<Integer>x=ids(levels);ids.addAll(x);out.add(finding(d,Severity.MEDIUM,seq++,"Kot / bağlantı kontrolü",levels.size()+" adet kot, eğim, rögar veya bağlantı ifadesi bulundu.","Hat başlangıç-bitiş kotlarını, eğimleri ve kurum bağlantı noktalarını karşılaştırın.",x));}
                break;
            }
            case ELEVATOR:{
                List<MusaAiDrawingIndex.Item> dims=filter(items,"kuyu","kabin","kap i","kapi","makine dairesi","kuyu dibi","ust bosluk");
                if(!dims.isEmpty()){Set<Integer>x=ids(dims);ids.addAll(x);out.add(finding(d,Severity.MEDIUM,seq++,"Asansör kuyu/kapı koordinasyonu",dims.size()+" adet kuyu, kabin, kapı veya makine alanı ifadesi bulundu.","Mimari/statik ölçüler, üst boşluk, kuyu dibi, kapı net açıklığı ve elektrik beslemesini birlikte doğrulayın.",x));}
                break;
            }
            case FIRE_SAFETY:{
                List<MusaAiDrawingIndex.Item> life=filter(items,"sprinkler","hidrant","yangin","duman","basinclandirma","kacis","itfaiye");
                if(!life.isEmpty()){Set<Integer>x=ids(life);ids.addAll(x);out.add(finding(d,Severity.HIGH,seq++,"Can güvenliği koordinasyon alanı",life.size()+" adet yangın/can güvenliği öğesi bulundu.","Mekanik, elektrik ve mimari yangın senaryosunu birlikte kontrol edin; otomatik tarama mevzuat onayı değildir.",x));}
                break;
            }
            default:break;
        }
        return seq;
    }

    private static boolean crossBelongs(MusaAiDiscipline d,String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(d==MusaAiDiscipline.FIRE_SAFETY)return contains(q,"yangin","sprink","hidrant","duman","itfaiye");
        if(d==MusaAiDiscipline.ELECTRICAL)return contains(q,"besleme","pano","kablo","tava","priz","aydinlatma","topraklama","jenerator","ups","data","cctv");
        if(d==MusaAiDiscipline.STRUCTURAL)return contains(q,"rezervasyon","tasiyici","kolon","kiris","perde","doseme","temel");
        if(d==MusaAiDiscipline.INFRASTRUCTURE)return contains(q,"rogar","baglanti","kanalizasyon","altyapi");
        return false;
    }
    private static int count(List<MusaAiDrawingIndex.Item>items,String...terms){return filter(items,terms).size();}
    private static List<MusaAiDrawingIndex.Item> filter(List<MusaAiDrawingIndex.Item>items,String...terms){
        ArrayList<MusaAiDrawingIndex.Item>out=new ArrayList<>();
        for(MusaAiDrawingIndex.Item i:items){
            String q=MusaAiDrawingIndex.normalize(i.layer+" "+i.text);
            if(contains(q,terms))out.add(i);
        }
        return out;
    }
    private static boolean contains(String q,String...terms){for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;}
    private static boolean hasReviewMarker(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et","uygunsuz");
    }
    private static Set<Integer>ids(Collection<MusaAiDrawingIndex.Item>items){LinkedHashSet<Integer>x=new LinkedHashSet<>();addIds(x,items);return x;}
    private static void addIds(Set<Integer>out,Collection<MusaAiDrawingIndex.Item>items){for(MusaAiDrawingIndex.Item i:items)if(i!=null&&i.sourceId>=0)out.add(i.sourceId);}
    private static Finding finding(MusaAiDiscipline d,Severity s,int seq,String title,String detail,String suggestion,Collection<Integer>ids){
        return new Finding(d.code+"-"+String.format(Locale.ROOT,"%03d",seq),d,s,title,detail,suggestion,ids);
    }
    private static String summaryLine(Result r){
        int critical=0,high=0,medium=0,info=0;
        for(Finding f:r.findings){switch(f.severity){case CRITICAL:critical++;break;case HIGH:high++;break;case MEDIUM:medium++;break;default:info++;}}
        return "• Bulgu: "+r.findings.size()+" • Kritik: "+critical+" • Yüksek: "+high+" • Orta: "+medium+" • Bilgi: "+info;
    }
    private static Severity advancedSeverity(MusaAiStructuralAdvanced.Status status){
        if(status==null)return Severity.INFO;
        switch(status){
            case UYUMSUZLUK:return Severity.HIGH;
            case INCELEME_GEREKLI:return Severity.MEDIUM;
            case DOGRULANAMADI:return Severity.INFO;
            default:return Severity.INFO;
        }
    }
    private static String severityLabel(Severity s){switch(s){case CRITICAL:return "KRİTİK";case HIGH:return "YÜKSEK";case MEDIUM:return "ORTA";default:return "BİLGİ";}}
    private static String blank(String s,String fallback){return s==null||s.trim().isEmpty()?fallback:s.trim();}
    private MusaAiDisciplineAnalyzer(){}
}
