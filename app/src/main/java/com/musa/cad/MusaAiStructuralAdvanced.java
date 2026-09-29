package com.musa.cad;

import java.util.*;
import java.util.regex.*;

/**
 * Advanced conservative structural coordination checks.
 *
 * This engine reports explicit drawing/model inconsistencies and review candidates.
 * It never invents structural capacity, reinforcement adequacy, seismic performance
 * or code-compliance results that are not present in the supplied project data.
 */
public final class MusaAiStructuralAdvanced {
    public enum Status { UYUMSUZLUK, INCELEME_GEREKLI, DOGRULANAMADI, BILGI }

    public static final class Finding {
        public final String id,title,detail,suggestion;
        public final Status status;
        public final List<Integer> sourceIds;
        Finding(String id,Status status,String title,String detail,String suggestion,Collection<Integer>sourceIds){
            this.id=id;this.status=status;this.title=title;this.detail=detail;this.suggestion=suggestion;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
    }

    public static final class Result {
        public final boolean matched;
        public final String text;
        public final List<Finding> findings;
        public final List<Integer> sourceIds;
        Result(boolean matched,String text,Collection<Finding>findings,Collection<Integer>sourceIds){
            this.matched=matched;this.text=text==null?"":text;
            this.findings=Collections.unmodifiableList(new ArrayList<>(findings));
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Result none(){return new Result(false,"",Collections.emptyList(),Collections.emptyList());}
    }

    private enum Kind { COLUMN,WALL,BEAM,SLAB,FOUNDATION,ELEVATOR,STAIR,OTHER }

    private static final Pattern TAG=Pattern.compile("(?iu)\\b(?:KOL|COL|S|C|PER|WALL|P|W|KIR|BEAM|K|B|D|SLAB|T|FOOT)[-_ ]?\\d{1,4}[A-Z]?\\b");
    private static final Pattern SECTION=Pattern.compile("(?i)(?<!\\d)(\\d{2,4})\\s*[x×/]\\s*(\\d{2,4})(?!\\d)");
    private static final Pattern AXIS=Pattern.compile("(?iu)\\b(?:AKS|AXIS|GRID)\\s*[:=]?\\s*([A-ZÇĞİÖŞÜ0-9]{1,4})(?:\\s*[-/]\\s*([A-ZÇĞİÖŞÜ0-9]{1,4}))?");
    private static final Pattern FLOOR_A=Pattern.compile("(?iu)\\b(?:KAT|FLOOR|STOREY)\\s*[:=]?\\s*([+-]?\\d{1,2}|ZEM[İI]N|GROUND|BODRUM\\s*\\d{0,2}|BASEMENT\\s*\\d{0,2})\\b");
    private static final Pattern FLOOR_B=Pattern.compile("(?iu)\\b([+-]?\\d{1,2})\\s*\\.?\\s*(?:KAT|FLOOR|STOREY)\\b");
    private static final Pattern FLOOR_G=Pattern.compile("(?iu)\\b(ZEM[İI]N|GROUND)\\s*(?:KAT|FLOOR)?\\b");
    private static final Pattern FLOOR_BSM=Pattern.compile("(?iu)\\b(BODRUM|BASEMENT)\\s*(\\d{0,2})\\s*(?:KAT|FLOOR)?\\b");
    private static final Pattern SIZE=Pattern.compile("(?iu)(\\d{2,5}(?:[\\.,]\\d+)?)\\s*[x×/]\\s*(\\d{2,5}(?:[\\.,]\\d+)?)\\s*(MM|CM|M)?");

    private static final class Ref {
        final MusaAiDrawingIndex.Item item;
        final String raw,q,tag,floor,axis,section;
        final Kind kind;
        Ref(MusaAiDrawingIndex.Item item){
            this.item=item;
            raw=(item.layer+" "+item.text).trim();
            q=MusaAiDrawingIndex.normalize(raw);
            tag=tag(raw);floor=floor(raw);axis=axis(raw);section=section(raw);
            kind=kind(q,tag);
        }
    }

    public static Result analyze(MusaAiDrawingIndex index,MusaAiStructuralCalc.Model structuralCalc){
        if(index==null)return Result.none();

        ArrayList<Ref> refs=new ArrayList<>();
        LinkedHashSet<Integer> allIds=new LinkedHashSet<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            Ref r=new Ref(item);
            if(isRelevant(r)){
                refs.add(r);
                if(item.sourceId>=0)allIds.add(item.sourceId);
            }
        }
        if(refs.isEmpty())return Result.none();

        ArrayList<Finding> findings=new ArrayList<>();
        continuityChecks(refs,findings);
        sectionAndAxisChecks(refs,findings);
        openingChecks(refs,findings);
        foundationChecks(refs,findings);
        punchingChecks(refs,structuralCalc,findings);
        elevatorChecks(refs,findings);
        stairChecks(refs,findings);
        dilatationChecks(refs,findings);
        reportChecks(structuralCalc,findings);

        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        for(Finding f:findings)ids.addAll(f.sourceIds);

        StringBuilder out=new StringBuilder("İLERİ STATİK KOORDİNASYON / SÜREKLİLİK KONTROLÜ");
        int mismatch=0,review=0,unverified=0,info=0;
        for(Finding f:findings){
            switch(f.status){
                case UYUMSUZLUK:mismatch++;break;
                case INCELEME_GEREKLI:review++;break;
                case DOGRULANAMADI:unverified++;break;
                default:info++;
            }
        }
        out.append("\n• UYUMSUZLUK: ").append(mismatch);
        out.append(" • İNCELEME GEREKLİ: ").append(review);
        out.append(" • DOĞRULANAMADI: ").append(unverified);
        out.append(" • BİLGİ: ").append(info);

        int n=0;
        for(Finding f:findings){
            if(n++>=120){out.append("\n• … kalan bulgular rapor ekine bırakıldı.");break;}
            out.append("\n\n[").append(f.id).append("] ").append(label(f.status)).append(" • ").append(f.title);
            out.append("\n").append(f.detail);
            if(!f.suggestion.isEmpty())out.append("\nÖneri: ").append(f.suggestion);
        }
        out.append("\n\nNot: Bu modül açık proje/hesap verisini çapraz kontrol eder; hesap sonucu olmayan yerde taşıma gücü, zımbalama güvenliği, deprem performansı veya donatı yeterliliği uydurmaz.");
        return new Result(true,out.toString(),findings,ids);
    }

    private static void continuityChecks(List<Ref>refs,List<Finding>out){
        LinkedHashSet<Integer> projectFloors=new LinkedHashSet<>();
        for(Ref r:refs){Integer f=floorOrder(r.floor);if(f!=null)projectFloors.add(f);}
        if(projectFloors.size()<3)return;

        LinkedHashMap<String,ArrayList<Ref>> byTag=new LinkedHashMap<>();
        for(Ref r:refs){
            if(r.tag.isEmpty()||r.floor.isEmpty()||(r.kind!=Kind.COLUMN&&r.kind!=Kind.WALL))continue;
            byTag.computeIfAbsent(r.tag,k->new ArrayList<>()).add(r);
        }

        for(Map.Entry<String,ArrayList<Ref>> e:byTag.entrySet()){
            TreeMap<Integer,Ref> floors=new TreeMap<>();
            for(Ref r:e.getValue()){Integer f=floorOrder(r.floor);if(f!=null)floors.putIfAbsent(f,r);}
            if(floors.size()<2)continue;
            int min=floors.firstKey(),max=floors.lastKey();
            for(Integer projectFloor:projectFloors){
                if(projectFloor<=min||projectFloor>=max||floors.containsKey(projectFloor))continue;
                Kind k=e.getValue().get(0).kind;
                LinkedHashSet<Integer>ids=new LinkedHashSet<>();for(Ref r:e.getValue())addId(ids,r);
                out.add(new Finding(k==Kind.WALL?"ST-03":"ST-02",Status.INCELEME_GEREKLI,
                    k==Kind.WALL?"Katlar arası perde sürekliliği":"Katlar arası kolon sürekliliği",
                    e.getKey()+" elemanı "+floorLabel(projectFloor)+" seviyesinde eşleşmeden üst ve alt katlarda görülüyor.",
                    "Transfer elemanı, isim değişikliği veya gerçek süreksizlik olup olmadığını statik model ve kat kalıp planlarıyla doğrulayın.",ids));
                break;
            }
        }
    }

    private static void sectionAndAxisChecks(List<Ref>refs,List<Finding>out){
        LinkedHashMap<String,ArrayList<Ref>> byTag=new LinkedHashMap<>();
        for(Ref r:refs)if(!r.tag.isEmpty()&&(r.kind==Kind.COLUMN||r.kind==Kind.WALL||r.kind==Kind.BEAM))
            byTag.computeIfAbsent(r.tag,k->new ArrayList<>()).add(r);

        for(Map.Entry<String,ArrayList<Ref>> e:byTag.entrySet()){
            LinkedHashSet<String>axes=new LinkedHashSet<>(),sections=new LinkedHashSet<>(),floors=new LinkedHashSet<>();
            LinkedHashSet<Integer>ids=new LinkedHashSet<>();
            for(Ref r:e.getValue()){
                if(!r.axis.isEmpty())axes.add(r.axis);
                if(!r.section.isEmpty())sections.add(r.section);
                if(!r.floor.isEmpty())floors.add(r.floor);
                addId(ids,r);
            }
            if(floors.size()>1&&axes.size()>1)
                out.add(new Finding("ST-04",Status.INCELEME_GEREKLI,"Taşıyıcı aks değişimi",
                    e.getKey()+" farklı katlarda birden fazla aks tanımıyla okunuyor: "+join(axes,8)+".",
                    "Aks kaçıklığı, transfer çözümü veya etiketleme farkını katlar arası kalıp planında doğrulayın.",ids));
            if(floors.size()>1&&sections.size()>1)
                out.add(new Finding("ST-05",Status.INCELEME_GEREKLI,"Katlar arası kesit değişimi",
                    e.getKey()+" için farklı kat/kayıtlarda şu kesitler okundu: "+join(sections,8)+".",
                    "Kesit değişiminin hesap modelinde ve donatı detayında aynı revizyonda tanımlı olduğunu doğrulayın.",ids));
        }
    }

    private static void openingChecks(List<Ref>refs,List<Finding>out){
        for(Ref r:refs){
            if(!has(r.q,"rezervasyon","bosluk","delik","gecis","opening","sleeve","saft","shaft"))continue;
            LinkedHashSet<Integer>ids=idSet(r);
            if(r.kind==Kind.BEAM)
                out.add(new Finding("ST-08",Status.INCELEME_GEREKLI,"Kiriş rezervasyon / delik kontrolü",
                    location(r)+" üzerinde kirişle ilişkili boşluk, delik veya rezervasyon ifadesi bulundu.",
                    "Geçiş yerini ve ölçüsünü statik detay/hesapla doğrulayın; onaysız sonradan delme yapılmamalıdır.",ids));
            else if(r.kind==Kind.WALL)
                out.add(new Finding("ST-09",Status.INCELEME_GEREKLI,"Perde açıklığı / model koordinasyonu",
                    location(r)+" üzerinde perdeyle ilişkili açıklık veya rezervasyon ifadesi bulundu.",
                    "Perde açıklığının hesap modelinde ve donatı detayında aynı geometrinin parçası olduğunu doğrulayın.",ids));
            else if(r.kind==Kind.SLAB)
                out.add(new Finding("ST-07",Status.INCELEME_GEREKLI,"Döşeme boşluğu / rezervasyon kontrolü",
                    location(r)+" üzerinde döşemeyle ilişkili açıklık veya rezervasyon ifadesi bulundu.",
                    "Boşluk çevresi donatısını, kiriş/kolon yakınlığını ve mimari-mekanik ölçü koordinasyonunu doğrulayın.",ids));
        }
    }

    private static void foundationChecks(List<Ref>refs,List<Finding>out){
        int foundation=0,vertical=0;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        boolean foundationAxis=false,verticalAxis=false;
        for(Ref r:refs){
            if(r.kind==Kind.FOUNDATION){foundation++;foundationAxis|=!r.axis.isEmpty();addId(ids,r);}
            if(r.kind==Kind.COLUMN||r.kind==Kind.WALL){vertical++;verticalAxis|=!r.axis.isEmpty();}
        }
        if(foundation>0&&vertical>0&&(!foundationAxis||!verticalAxis))
            out.add(new Finding("ST-06",Status.DOGRULANAMADI,"Temel / üst yapı aks eşleşmesi kısmi",
                "Temel ve düşey taşıyıcı verisi mevcut ancak taraflardan en az birinde açık aks bilgisi okunamadı.",
                "Radye/kazık başlığı ile kolon-perde akslarını temel kalıp planı ve üst yapı kalıp planında karşılaştırın.",ids));
    }

    private static void punchingChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        boolean flat=false,punchingDrawing=false;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs){
            if(has(r.q,"kirissiz doseme","mantar doseme","flat slab","flat plate"))flat=true;
            if(has(r.q,"zimbalama","punching")){punchingDrawing=true;addId(ids,r);}
        }
        boolean punchingReport=false;
        if(calc!=null)for(MusaAiStructuralCalc.Element e:calc.elements)if(e!=null&&!e.punchingDetails.isEmpty()){punchingReport=true;break;}

        if(flat&&!punchingDrawing&&!punchingReport)
            out.add(new Finding("ST-14",Status.DOGRULANAMADI,"Zımbalama doğrulaması gerekli",
                "Kirişsiz/mantar döşeme ifadesi bulundu ancak görünür projede veya yüklenen hesap verisinde açık zımbalama sonucu/detayı eşleştirilemedi.",
                "Kolon-döşeme birleşimleri için zımbalama hesabını ve varsa zımbalama donatısı detaylarını yükleyip karşılaştırın.",ids));
        else if(punchingDrawing&&calc!=null&&!punchingReport)
            out.add(new Finding("ST-14",Status.DOGRULANAMADI,"Zımbalama proje–rapor eşleşmesi",
                "DWG tarafında zımbalama ifadesi bulundu; yüklenen hesap raporunda eşleşen açık zımbalama detayı çıkarılamadı.",
                "Hesap raporundaki kolon/döşeme etiketleri ile pafta etiketlerini eşleştirin.",ids));
    }

    private static void elevatorChecks(List<Ref>refs,List<Finding>out){
        ArrayList<Ref> elevators=new ArrayList<>();
        LinkedHashSet<String> sizes=new LinkedHashSet<>();
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs)if(r.kind==Kind.ELEVATOR||has(r.q,"asansor","elevator","lift","kuyu")){
            elevators.add(r);addId(ids,r);String s=size(r.raw);if(!s.isEmpty())sizes.add(s);
        }
        if(elevators.isEmpty())return;
        if(sizes.size()>1)
            out.add(new Finding("ST-10",Status.INCELEME_GEREKLI,"Asansör kuyusu ölçü koordinasyonu",
                "Asansör/kuyu ile ilişkili birden fazla ölçü ifadesi okundu: "+join(sizes,8)+".",
                "Mimari, statik ve asansör uygulama projesindeki net kuyu/kapı/pit ölçülerini aynı aks ve katta karşılaştırın.",ids));
        else if(sizes.isEmpty())
            out.add(new Finding("ST-10",Status.DOGRULANAMADI,"Asansör kuyusu ölçüsü doğrulanamadı",
                "Asansör/kuyu ifadesi bulundu ancak güvenilir kuyu ölçüsü otomatik çıkarılamadı.",
                "Kuyu iç ölçüsü, perde kalınlığı, kapı açıklığı, pit ve üst boşluk değerlerini görünür hale getirip tekrar kontrol edin.",ids));
    }

    private static void stairChecks(List<Ref>refs,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();boolean stair=false,opening=false;
        for(Ref r:refs){
            if(r.kind==Kind.STAIR||has(r.q,"merdiven","stair")){stair=true;addId(ids,r);}
            if(has(r.q,"merdiven boslugu","stair opening","sahanlik","landing"))opening=true;
        }
        if(stair&&!opening)
            out.add(new Finding("ST-11",Status.DOGRULANAMADI,"Merdiven boşluğu / mesnet koordinasyonu",
                "Merdiven verisi bulundu ancak görünür indeks içinde açık merdiven boşluğu/sahanlık mesnet tanımı eşleştirilemedi.",
                "Mimari merdiven boşluğu ile statik döşeme boşluğu, sahanlık ve mesnet kirişlerini karşılaştırın.",ids));
    }

    private static void dilatationChecks(List<Ref>refs,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();int n=0;
        for(Ref r:refs)if(has(r.q,"dilatasyon","dilatation","expansion joint","deprem derzi","derz")){n++;addId(ids,r);}
        if(n>0)
            out.add(new Finding("ST-12",Status.INCELEME_GEREKLI,"Dilatasyon / blok sürekliliği kontrolü",
                n+" adet dilatasyon/derz ifadesi bulundu. Çizim indeksinden tek başına taşıyıcı elemanların derzi geçip geçmediği kesinleştirilemez.",
                "Kiriş, döşeme, perde ve temel sürekliliğini blok ayrımı boyunca geometrik olarak doğrulayın; ortak temel varsa hesap modelindeki kabulü ayrıca kontrol edin.",ids));
    }

    private static void reportChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        if(calc.elements.isEmpty())
            out.add(new Finding("ST-15",Status.DOGRULANAMADI,"Hesap raporu eleman eşleşmesi sınırlı",
                "Yüklenen hesap raporundan kat/aks/eleman bazında yeterli taşıyıcı kayıt ayrııştırılamadı.",
                "Eleman etiketleri, kat adları ve aks bilgileri içeren hesap çıktısını veya model dışa aktarımını kullanın.",Collections.emptyList()));
    }

    private static boolean isRelevant(Ref r){
        if(r==null)return false;
        return r.kind!=Kind.OTHER||has(r.q,"statik","betonarme","tasiyici","donati","rezervasyon","bosluk","dilatasyon","zimbalama","kazik","radye");
    }

    private static Kind kind(String q,String tag){
        if(has(q,"asansor","elevator","lift","kuyu"))return Kind.ELEVATOR;
        if(has(q,"merdiven","stair","sahanlik","landing"))return Kind.STAIR;
        if(has(q,"temel","radye","kazik","foundation","footing","pile"))return Kind.FOUNDATION;
        if(has(q,"kolon","column"))return Kind.COLUMN;
        if(has(q,"perde","shear wall"))return Kind.WALL;
        if(has(q,"kiris","beam"))return Kind.BEAM;
        if(has(q,"doseme","slab"))return Kind.SLAB;
        String t=tag==null?"":tag;
        if(t.matches("(?:KOL|COL|S|C)\\d{1,4}[A-Z]?"))return Kind.COLUMN;
        if(t.matches("(?:PER|WALL|P|W)\\d{1,4}[A-Z]?"))return Kind.WALL;
        if(t.matches("(?:KIR|BEAM|K|B)\\d{1,4}[A-Z]?"))return Kind.BEAM;
        if(t.matches("(?:D|SLAB)\\d{1,4}[A-Z]?"))return Kind.SLAB;
        if(t.matches("(?:T|FOOT)\\d{1,4}[A-Z]?"))return Kind.FOUNDATION;
        return Kind.OTHER;
    }

    private static String tag(String raw){
        Matcher m=TAG.matcher(raw==null?"":raw);if(!m.find())return "";
        return m.group().toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");
    }
    private static String section(String raw){
        Matcher m=SECTION.matcher(raw==null?"":raw);if(!m.find())return "";
        return Integer.parseInt(m.group(1))+"x"+Integer.parseInt(m.group(2));
    }
    private static String axis(String raw){
        Matcher m=AXIS.matcher(raw==null?"":raw);if(!m.find())return "";
        String a=cleanToken(m.group(1)),b=cleanToken(m.group(2));
        return b.isEmpty()?a:a+"-"+b;
    }
    private static String floor(String raw){
        if(raw==null)return "";
        Matcher m=FLOOR_BSM.matcher(raw);if(m.find())return canonicalFloor(m.group(1)+(m.group(2)==null?"":m.group(2)));
        String q=MusaAiDrawingIndex.normalize(raw);
        m=FLOOR_G.matcher(raw);if(m.find()&&!q.contains("zemin sinifi"))return "ZEMIN";
        m=FLOOR_A.matcher(raw);if(m.find())return canonicalFloor(m.group(1));
        m=FLOOR_B.matcher(raw);if(m.find())return canonicalFloor(m.group(1));
        return "";
    }
    private static Integer floorOrder(String floor){
        if(floor==null||floor.isEmpty())return null;
        String q=MusaAiDrawingIndex.normalize(floor).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.contains("ZEMIN")||q.contains("GROUND"))return 0;
        if(q.startsWith("BODRUM")){
            String n=q.substring("BODRUM".length()).replaceAll("[^0-9]","");
            return n.isEmpty()?-1:-Math.max(1,Integer.parseInt(n));
        }
        String n=q.replaceAll("[^0-9+-]","");
        try{return Integer.parseInt(n);}catch(Exception e){return null;}
    }
    private static String floorLabel(int n){return n==0?"ZEMİN":(n<0?"BODRUM "+Math.abs(n):n+". KAT");}
    private static String canonicalFloor(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.contains("GROUND")||q.contains("ZEMIN"))return "ZEMIN";
        if(q.startsWith("BASEMENT"))q="BODRUM"+q.substring("BASEMENT".length());
        if(q.startsWith("BODRUM"))return q;
        if(q.matches("[+-]?\\d{1,2}"))return q+".KAT";
        return q;
    }
    private static String size(String raw){
        Matcher m=SIZE.matcher(raw==null?"":raw);if(!m.find())return "";
        String unit=m.group(3)==null?"":m.group(3).toUpperCase(Locale.ROOT);
        return m.group(1).replace(',','.')+"x"+m.group(2).replace(',','.')+(unit.isEmpty()?"":" "+unit);
    }
    private static String location(Ref r){
        StringBuilder s=new StringBuilder();
        if(!r.tag.isEmpty())s.append(r.tag);
        if(!r.floor.isEmpty()){if(s.length()>0)s.append(" • ");s.append(r.floor);}
        if(!r.axis.isEmpty()){if(s.length()>0)s.append(" • ");s.append("AKS ").append(r.axis);}
        return s.length()==0?"İlgili taşıyıcı kayıt":s.toString();
    }
    private static LinkedHashSet<Integer>idSet(Ref r){LinkedHashSet<Integer>x=new LinkedHashSet<>();addId(x,r);return x;}
    private static void addId(Set<Integer>out,Ref r){if(r!=null&&r.item!=null&&r.item.sourceId>=0)out.add(r.item.sourceId);}
    private static String cleanToken(String s){return s==null?"":s.toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");}
    private static boolean has(String q,String...terms){for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;}
    private static String join(Collection<String>values,int max){
        StringBuilder out=new StringBuilder();int n=0;
        for(String v:values){if(n++>=max){out.append(", …");break;}if(out.length()>0)out.append(", ");out.append(v);}
        return out.length()==0?"—":out.toString();
    }
    private static String label(Status s){
        switch(s){
            case UYUMSUZLUK:return "UYUMSUZLUK";
            case INCELEME_GEREKLI:return "İNCELEME GEREKLİ";
            case DOGRULANAMADI:return "DOĞRULANAMADI";
            default:return "BİLGİ";
        }
    }

    private MusaAiStructuralAdvanced(){}
}
