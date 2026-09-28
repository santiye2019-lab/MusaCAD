package com.musa.cad;

import java.util.*;

/**
 * Cross-discipline deterministic project review over the active MusaCAD drawing.
 *
 * This module detects discipline evidence and review candidates from visible
 * CAD layers, text and lightweight geometry. It does not certify engineering,
 * code compliance, constructability or statutory approval.
 */
public final class MusaAiDisciplineControl {
    public enum Discipline {
        ARCHITECTURE("Mimari"),
        STRUCTURAL("Statik / taşıyıcı sistem"),
        MECHANICAL("Mekanik"),
        ELECTRICAL("Elektrik"),
        FIRE("Yangın / can güvenliği"),
        INFRASTRUCTURE("Altyapı"),
        LANDSCAPE("Peyzaj"),
        ELEVATOR("Asansör");

        public final String label;
        Discipline(String label){this.label=label;}
    }

    public static final class Result {
        public final boolean matched;
        public final String text;
        public final int findingCount;
        public final Set<Discipline> detected;
        public final List<Integer> sourceIds;
        private Result(boolean matched,String text,int findingCount,Collection<Discipline>detected,Collection<Integer>sourceIds){
            this.matched=matched;this.text=text==null?"":text;this.findingCount=Math.max(0,findingCount);
            this.detected=Collections.unmodifiableSet(detected==null?EnumSet.noneOf(Discipline.class):EnumSet.copyOf(detected));
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds==null?Collections.emptyList():sourceIds));
        }
        public static Result none(){return new Result(false,"",0,EnumSet.noneOf(Discipline.class),Collections.emptyList());}
    }

    private static final class Stats {
        int items,linear,areas,texts,degenerate,reviewNotes;
        boolean dimensions,levels,axis,room,doorWindow,sectionDetail;
        boolean columnBeamSlab,foundation,rebar,concreteGrade,steelGrade;
        boolean panel,circuit,cable,lighting,outlet,grounding,weakCurrent;
        boolean pipe,duct,equipment,diameter,flowCapacity;
        boolean detector,alarm,sprinkler,hydrant,exit,fireDoor;
        boolean manhole,siteLine,invertLevel,roadSite;
        boolean plant,tree,grass,irrigation,hardscape,schedule;
        boolean liftShaft,car,door,pit,machineRoom,capacity,speed;
        final LinkedHashSet<Integer> issueIds=new LinkedHashSet<>();
        final LinkedHashSet<Integer> ids=new LinkedHashSet<>();
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        EnumSet<Discipline>selected=selected(q);
        if(selected==null)return Result.none();

        EnumMap<Discipline,Stats>stats=new EnumMap<>(Discipline.class);
        for(Discipline d:Discipline.values())stats.put(d,new Stats());

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            EnumSet<Discipline>ds=classify(hay);
            if(ds.isEmpty())continue;
            for(Discipline d:ds)add(stats.get(d),item,hay);
        }

        EnumSet<Discipline>detected=EnumSet.noneOf(Discipline.class);
        for(Discipline d:Discipline.values())if(stats.get(d).items>0)detected.add(d);

        StringBuilder out=new StringBuilder();
        out.append("MusaCAD AI • ÇOK DİSİPLİNLİ PROJE KONTROLÜ");
        out.append("\nLayout: ").append(index.layout.isEmpty()?"aktif layout":index.layout);
        out.append("\nKapsam: ");
        if(selected.size()==Discipline.values().length)out.append("Tüm disiplinler");
        else{
            ArrayList<String>names=new ArrayList<>();
            for(Discipline d:selected)names.add(d.label);
            out.append(String.join(", ",names));
        }

        out.append("\n\nTespit edilen disiplin verisi");
        boolean any=false;
        for(Discipline d:selected){
            Stats s=stats.get(d);if(s.items<=0)continue;any=true;
            out.append("\n• ").append(d.label).append(": ").append(s.items).append(" nesne");
            if(s.linear>0)out.append(" • ").append(s.linear).append(" çizgisel");
            if(s.texts>0)out.append(" • ").append(s.texts).append(" metinli");
        }
        if(!any)out.append("\n• Seçilen disiplin için görünür katman/metin eşleşmesi bulunamadı.");

        int findings=0;
        LinkedHashSet<Integer>highlights=new LinkedHashSet<>();
        out.append("\n\nİnceleme adayları");
        for(Discipline d:selected){
            Stats s=stats.get(d);if(s.items<=0)continue;
            int before=findings;
            if(s.degenerate>0){
                out.append("\n• ").append(d.label).append(": sıfır uzunluk/dejenere geometri ").append(s.degenerate);
                findings+=s.degenerate;highlights.addAll(s.issueIds);
            }
            if(s.reviewNotes>0){
                out.append("\n• ").append(d.label).append(": EKSİK / REVİZE / TODO benzeri kontrol notu ").append(s.reviewNotes);
                findings+=s.reviewNotes;highlights.addAll(s.issueIds);
            }
            findings+=disciplineChecks(out,d,s);
            if(findings==before&&s.degenerate==0&&s.reviewNotes==0){
                // No line is added here; the global zero-state below remains concise.
            }
        }
        if(findings==0)out.append("\n• Görünür CAD verisinden belirgin otomatik inceleme adayı oluşmadı.");

        out.append("\n\nDisiplinler arası koordinasyon adayları");
        int coordination=0;
        if(selected.size()>1||selected.size()==Discipline.values().length){
            if(detected.contains(Discipline.ARCHITECTURE)&&detected.contains(Discipline.STRUCTURAL)){
                out.append("\n• Mimari ↔ Statik: aks, kolon/perde, merdiven, şaft ve açıklıkların paftalar arası koordinasyonu kontrol edilmeli.");
                coordination++;
            }
            if(detected.contains(Discipline.ARCHITECTURE)&&detected.contains(Discipline.MECHANICAL)){
                out.append("\n• Mimari ↔ Mekanik: şaft, asma tavan, cihaz/kanal/boru geçişleri ve mahal kullanım çakışmaları kontrol edilmeli.");
                coordination++;
            }
            if(detected.contains(Discipline.ARCHITECTURE)&&detected.contains(Discipline.ELECTRICAL)){
                out.append("\n• Mimari ↔ Elektrik: pano, armatür, priz, zayıf akım ve mahal yerleşimleri kontrol edilmeli.");
                coordination++;
            }
            if(detected.contains(Discipline.STRUCTURAL)&&(detected.contains(Discipline.MECHANICAL)||detected.contains(Discipline.ELECTRICAL))){
                out.append("\n• Statik ↔ MEP: döşeme/kiriş/perde geçişleri ve rezervasyonlar taşıyıcı sisteme göre doğrulanmalı.");
                coordination++;
            }
            if(detected.contains(Discipline.ELEVATOR)&&(detected.contains(Discipline.ARCHITECTURE)||detected.contains(Discipline.STRUCTURAL))){
                out.append("\n• Asansör ↔ Mimari/Statik: kuyu, pit, üst boşluk, kapılar ve taşıyıcı boşluklar koordine edilmeli.");
                coordination++;
            }
            if(detected.contains(Discipline.INFRASTRUCTURE)&&(detected.contains(Discipline.LANDSCAPE)||detected.contains(Discipline.ARCHITECTURE))){
                out.append("\n• Altyapı ↔ Peyzaj/Mimari: saha kotları, rögarlar, hat güzergâhları, sert zemin ve bina girişleri koordine edilmeli.");
                coordination++;
            }
        }
        if(coordination==0)out.append("\n• Otomatik koordinasyon başlığı oluşturmak için en az iki ilgili disiplinin görünür verisi gerekir.");

        out.append("\n\nNot: Bu tarama katman adları, görünür metinler ve sınırlı geometri metadata'sına dayanır. ")
           .append("Statik hesap, elektrik yük/kısa devre/selektivite, hidrolik/ısı yükü, yangın senaryosu, erişilebilirlik, ")
           .append("asansör hesapları, zemin/altyapı hesapları ve yürürlükteki mevzuat ayrıca yetkili proje müelliflerince doğrulanmalıdır.");

        return new Result(true,out.toString(),findings,detected,limit(highlights,350));
    }

    public static String help(){
        return "Çok disiplinli AI kontrol komutları"+
            "\n• AI_DISIPLIN_KONTROL — tüm disiplinler"+
            "\n• AI_MIMARI_KONTROL"+
            "\n• AI_STATIK_KONTROL"+
            "\n• AI_ELEKTRIK_KONTROL"+
            "\n• AI_YANGIN_KONTROL"+
            "\n• AI_ALTYAPI_KONTROL"+
            "\n• AI_PEYZAJ_KONTROL"+
            "\n• AI_ASANSOR_KONTROL"+
            "\n\nDoğal dil de kullanılabilir: “Statik projeyi kontrol et”, “Tüm disiplinleri kontrol et” gibi.";
    }

    /** Exposed for BOQ/report grouping without duplicating discipline keywords elsewhere. */
    public static Set<Discipline> classifyText(String raw){
        return Collections.unmodifiableSet(classify(MusaAiDrawingIndex.normalize(raw)));
    }

    private static EnumSet<Discipline>selected(String q){
        if(q==null||q.isEmpty())return null;
        if(q.equals("ai disiplin kontrol")||q.equals("ai tum disiplin kontrol")||
           q.equals("tum disiplinleri kontrol et")||q.equals("butun disiplinleri kontrol et")||
           q.equals("projeyi tum disiplinlerde kontrol et")||q.equals("cok disiplinli kontrol")||
           q.equals("multidisiplin kontrol")||q.equals("multi disiplin kontrol"))
            return EnumSet.allOf(Discipline.class);

        if(!asksControl(q))return null;
        if(has(q,"mimari","architectural","architecture"))return EnumSet.of(Discipline.ARCHITECTURE);
        if(has(q,"statik","tasiyici","structural","betonarme","celik proje"))return EnumSet.of(Discipline.STRUCTURAL);
        if(has(q,"elektrik","electrical","kuvvetli akim","zayif akim"))return EnumSet.of(Discipline.ELECTRICAL);
        if(has(q,"yangin","fire","can guvenligi"))return EnumSet.of(Discipline.FIRE);
        if(has(q,"altyapi","infrastructure","saha altyapi"))return EnumSet.of(Discipline.INFRASTRUCTURE);
        if(has(q,"peyzaj","landscape"))return EnumSet.of(Discipline.LANDSCAPE);
        if(has(q,"asansor","elevator","lift"))return EnumSet.of(Discipline.ELEVATOR);
        // Mechanical-specific requests are intentionally left to the richer
        // MusaAiMechanicalControl / MEKAI engines.
        return null;
    }

    private static EnumSet<Discipline>classify(String hay){
        EnumSet<Discipline>out=EnumSet.noneOf(Discipline.class);
        if(has(hay,"mimari","architect","mahal","room","kapi","door","pencere","window","duvar","wall",
            "merdiven","stair","rampa","ramp","kesit","section","gorunus","elevation","mobilya"))
            out.add(Discipline.ARCHITECTURE);
        if(has(hay,"statik","structural","kolon","column","kiris","beam","doseme","slab","perde","shear wall",
            "temel","foundation","radye","rebar","donati","betonarme","concrete","nervur","filiz","ankraj"))
            out.add(Discipline.STRUCTURAL);
        if(has(hay,"mekanik","mechanical","pis su","atik su","temiz su","sicak su","soguk su","vrf","vrv",
            "chiller","fancoil","fan coil","havalandirma","ventilasyon","duct","menfez","pompa","hidrofor",
            "dogalgaz","sprinkler","hidrant","kazan","boyler"))
            out.add(Discipline.MECHANICAL);
        if(has(hay,"elektrik","electrical","pano","panel","kablo","cable","priz","socket","armat","lighting",
            "aydinlatma","topraklama","ground","busbar","buat","sigorta","mcb","rcd","ups","jenerator",
            "trafo","zayif akim","data","telefon","cctv","kamera","yangin ihbar"))
            out.add(Discipline.ELECTRICAL);
        if(has(hay,"yangin","fire","sprinkler","hidrant","yangin dolabi","yangin ihbar","detektor","detector",
            "alarm","acil cikis","emergency exit","yangin kapisi","fire door","duman","smoke"))
            out.add(Discipline.FIRE);
        if(has(hay,"altyapi","infrastructure","rogar","manhole","kanalizasyon","sewer","yagmur suyu hatti",
            "storm line","parsel bacasi","saha drenaj","site drainage","saha su","site water","yol kot",
            "invert","taban kotu","rögar"))
            out.add(Discipline.INFRASTRUCTURE);
        if(has(hay,"peyzaj","landscape","agac","tree","bitki","plant","cim","grass","sulama","irrigation",
            "sert zemin","hardscape","bordur","curb","bank","oturma"))
            out.add(Discipline.LANDSCAPE);
        if(has(hay,"asansor","elevator","lift","kuyu","shaft","kabin","car","pit","makine dairesi",
            "machine room","asansor kapisi","lift door","durak","kat kapisi"))
            out.add(Discipline.ELEVATOR);
        return out;
    }

    private static void add(Stats s,MusaAiDrawingIndex.Item item,String hay){
        s.items++;if(item.hasLength()){s.linear++;if(item.length<=1e-9d)s.degenerate++;}
        if(item.hasArea())s.areas++;if(!item.text.isEmpty())s.texts++;
        if(item.sourceId>=0)s.ids.add(item.sourceId);
        boolean issue=false;
        if(item.hasLength()&&item.length<=1e-9d)issue=true;
        if(hasReviewMarker(item.text)){s.reviewNotes++;issue=true;}
        if(issue&&item.sourceId>=0)s.issueIds.add(item.sourceId);

        s.dimensions|="DIMENSION".equals(item.type)||has(hay,"olcu","dimension","dim ");
        s.levels|=has(hay,"kot","elevation","level","ffl","ssl");
        s.axis|=has(hay,"aks","axis","grid");
        s.room|=has(hay,"mahal","room","salon","ofis","wc","hol","koridor","oda");
        s.doorWindow|=has(hay,"kapi","door","pencere","window");
        s.sectionDetail|=has(hay,"kesit","section","detay","detail");

        s.columnBeamSlab|=has(hay,"kolon","column","kiris","beam","doseme","slab","perde","shear wall");
        s.foundation|=has(hay,"temel","foundation","radye","raft","kazik","pile");
        s.rebar|=has(hay,"donati","rebar","nervur","filiz","etriye","stirrup","hasir","mesh");
        s.concreteGrade|=rawPattern(item.text,"C\\s*\\d{2,3}\\s*[/\\-]\\s*\\d{2,3}")||has(hay,"beton sinifi","concrete grade");
        s.steelGrade|=rawPattern(item.text,"B\\s*420|B\\s*500|S\\s*420|S\\s*500")||has(hay,"celik sinifi","steel grade");

        s.panel|=has(hay,"pano","panel","mdb","db","tablo");
        s.circuit|=has(hay,"devre","circuit","linye","sorti","feeder");
        s.cable|=has(hay,"kablo","cable","nyy","n2xh","halogen");
        s.lighting|=has(hay,"aydinlatma","lighting","armat","luminaire");
        s.outlet|=has(hay,"priz","socket","outlet");
        s.grounding|=has(hay,"topraklama","grounding","earth","espotansiyel","equipotential");
        s.weakCurrent|=has(hay,"zayif akim","data","telefon","cctv","kamera","access","yangin ihbar");

        s.pipe|=has(hay,"boru","pipe","pissu","pis su","temiz su","sprinkler","hidrant","dogalgaz");
        s.duct|=has(hay,"kanal","duct","havalandirma","ventilasyon");
        s.equipment|=has(hay,"pompa","pump","fan","ahu","santral","chiller","kazan","boiler","vrf","fancoil");
        s.diameter|=hasDiameter(item.layer)||hasDiameter(item.text);
        s.flowCapacity|=hasCapacity(item.text)||hasCapacity(item.layer);

        s.detector|=has(hay,"detektor","detector","smoke","duman","heat detector");
        s.alarm|=has(hay,"alarm","yangin ihbar","fire alarm","siren","flaşor","flasor");
        s.sprinkler|=has(hay,"sprinkler");
        s.hydrant|=has(hay,"hidrant","hydrant","yangin dolabi","fire cabinet");
        s.exit|=has(hay,"acil cikis","emergency exit","exit","kacis");
        s.fireDoor|=has(hay,"yangin kapisi","fire door");

        s.manhole|=has(hay,"rogar","rögar","manhole","parsel bacasi");
        s.siteLine|=has(hay,"altyapi","kanalizasyon","sewer","storm","saha drenaj","site water","saha su");
        s.invertLevel|=has(hay,"taban kotu","invert","giris kotu","cikis kotu","boru kotu");
        s.roadSite|=has(hay,"yol","road","parsel","site","vaziyet");

        s.plant|=has(hay,"bitki","plant","cali","shrub");
        s.tree|=has(hay,"agac","tree");
        s.grass|=has(hay,"cim","grass","turf");
        s.irrigation|=has(hay,"sulama","irrigation","damlama","drip");
        s.hardscape|=has(hay,"sert zemin","hardscape","bordur","curb","tas","paving");
        s.schedule|=has(hay,"liste","schedule","legend","lejant","bitki listesi");

        s.liftShaft|=has(hay,"asansor kuyusu","lift shaft","elevator shaft","kuyu","shaft");
        s.car|=has(hay,"kabin","car","car size");
        s.door|=has(hay,"kat kapisi","asansor kapisi","lift door","elevator door");
        s.pit|=has(hay,"pit","kuyu dibi","dip");
        s.machineRoom|=has(hay,"makine dairesi","machine room","mrl");
        s.capacity|=has(hay,"kapasite","capacity","kg","kisi","person");
        s.speed|=has(hay,"hiz","speed","m s");
    }

    private static int disciplineChecks(StringBuilder out,Discipline d,Stats s){
        int n=0;
        switch(d){
            case ARCHITECTURE:
                if(!s.room){out.append("\n• Mimari: görünür mahal/oda adlandırması tespit edilmedi; mahal isimleri ve kullanım kararlarını kontrol edin.");n++;}
                if(!s.dimensions){out.append("\n• Mimari: görünür ölçülendirme verisi tespit edilmedi; ölçü paftalarını kontrol edin.");n++;}
                if(!s.doorWindow){out.append("\n• Mimari: kapı/pencere referansı görünmüyor; doğrama/kapı koordinasyonunu kontrol edin.");n++;}
                if(!s.levels&&!s.sectionDetail){out.append("\n• Mimari: kot/kesit/detay referansı görünmüyor; düşey koordinasyonu kontrol edin.");n++;}
                break;
            case STRUCTURAL:
                if(!s.axis){out.append("\n• Statik: aks/grid referansı görünmüyor; taşıyıcı sistem aks koordinasyonunu kontrol edin.");n++;}
                if(!s.columnBeamSlab){out.append("\n• Statik: kolon/kiriş/döşeme/perde referansı görünmüyor; pafta sınıflandırmasını kontrol edin.");n++;}
                if(!s.foundation){out.append("\n• Statik: temel/radye/kazık referansı görünmüyor; temel paftasının ayrı olup olmadığını kontrol edin.");n++;}
                if(!s.rebar){out.append("\n• Statik: donatı/rebar referansı görünmüyor; donatı paftalarını kontrol edin.");n++;}
                if(!s.concreteGrade&&!s.steelGrade){out.append("\n• Statik: görünür beton/çelik malzeme sınıfı tespit edilmedi; proje notları ve hesap raporuyla doğrulayın.");n++;}
                break;
            case MECHANICAL:
                if((s.pipe||s.duct)&&!s.diameter&&!s.flowCapacity){out.append("\n• Mekanik: hat/kanal verisi var ancak görünür çap/debi/kapasite etiketi tespit edilmedi.");n++;}
                if(!s.equipment){out.append("\n• Mekanik: görünür ekipman referansı sınırlı; ekipman listesi ve kapasite paftalarını kontrol edin.");n++;}
                break;
            case ELECTRICAL:
                if(!s.panel){out.append("\n• Elektrik: pano/tablo referansı görünmüyor; tek hat ve pano yerleşim paftalarını kontrol edin.");n++;}
                if(!s.circuit&&!s.cable){out.append("\n• Elektrik: devre/linye/kablo etiketi görünmüyor; devreleme ve kablo kesitlerini kontrol edin.");n++;}
                if(!s.lighting&&!s.outlet){out.append("\n• Elektrik: aydınlatma/priz referansı görünmüyor; ilgili kuvvetli akım paftalarını kontrol edin.");n++;}
                if(!s.grounding){out.append("\n• Elektrik: topraklama/eşpotansiyel referansı görünmüyor; topraklama projesini kontrol edin.");n++;}
                break;
            case FIRE:
                if(!s.detector&&!s.alarm){out.append("\n• Yangın: algılama/alarm referansı görünmüyor; yangın ihbar disiplinini kontrol edin.");n++;}
                if(!s.sprinkler&&!s.hydrant){out.append("\n• Yangın: sprinkler/hidrant/dolap referansı görünmüyor; sulu söndürme paftalarını kontrol edin.");n++;}
                if(!s.exit&&!s.fireDoor){out.append("\n• Yangın: kaçış/acil çıkış/yangın kapısı referansı görünmüyor; mimari can güvenliği koordinasyonunu kontrol edin.");n++;}
                break;
            case INFRASTRUCTURE:
                if(!s.manhole){out.append("\n• Altyapı: rögar/manhole/parsel bacası referansı görünmüyor; hat düğüm noktalarını kontrol edin.");n++;}
                if(!s.invertLevel){out.append("\n• Altyapı: taban/boru giriş-çıkış kotu referansı görünmüyor; cazibeli hat kotlarını kontrol edin.");n++;}
                if(!s.roadSite){out.append("\n• Altyapı: yol/parsel/vaziyet referansı görünmüyor; saha koordinasyon paftasını kontrol edin.");n++;}
                break;
            case LANDSCAPE:
                if(!s.plant&&!s.tree&&!s.grass){out.append("\n• Peyzaj: bitki/ağaç/çim referansı görünmüyor; bitkisel peyzaj paftasını kontrol edin.");n++;}
                if(!s.irrigation){out.append("\n• Peyzaj: sulama/irrigation referansı görünmüyor; sulama projesinin ayrı olup olmadığını kontrol edin.");n++;}
                if(!s.schedule){out.append("\n• Peyzaj: bitki listesi/lejant/schedule referansı görünmüyor; tür ve adet listesini kontrol edin.");n++;}
                break;
            case ELEVATOR:
                if(!s.liftShaft){out.append("\n• Asansör: kuyu/shaft referansı görünmüyor; mimari/statik kuyu koordinasyonunu kontrol edin.");n++;}
                if(!s.car&&!s.door){out.append("\n• Asansör: kabin/kat kapısı referansı görünmüyor; yerleşim paftasını kontrol edin.");n++;}
                if(!s.pit){out.append("\n• Asansör: pit/kuyu dibi referansı görünmüyor; kuyu kesitini kontrol edin.");n++;}
                if(!s.capacity&&!s.speed){out.append("\n• Asansör: kapasite/hız referansı görünmüyor; trafik/kapasite ve teknik hesapları kontrol edin.");n++;}
                break;
        }
        return n;
    }

    private static boolean asksControl(String q){
        return has(q,"kontrol","incele","tara","hata","eksik","uygunsuz","denetle","check","rapor");
    }
    private static boolean hasReviewMarker(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return has(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et");
    }
    private static boolean hasDiameter(String raw){
        if(raw==null||raw.trim().isEmpty())return false;
        String upper=raw.toUpperCase(Locale.ROOT).replace('Ø','D');
        return upper.matches(".*\\bDN\\s*[-:]?\\s*\\d+.*")||
               upper.matches(".*\\bD\\s*[-:]?\\s*\\d+.*")||
               upper.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*MM\\b.*")||
               has(MusaAiDrawingIndex.normalize(raw),"cap","diameter");
    }
    private static boolean hasCapacity(String raw){
        if(raw==null)return false;
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*(kw|w|hp|pa|kpa|bar|m3 h|cfm|kcal h|btu h)\\b.*")||
               has(q,"debi","flow","kapasite","capacity","basinc","pressure","guc","power");
    }
    private static boolean rawPattern(String raw,String regex){
        return raw!=null&&raw.toUpperCase(Locale.ROOT).matches(".*"+regex+".*");
    }
    private static boolean has(String q,String...terms){
        if(q==null)return false;
        for(String term:terms)if(q.contains(MusaAiDrawingIndex.normalize(term)))return true;
        return false;
    }
    private static Collection<Integer>limit(Collection<Integer>ids,int max){
        ArrayList<Integer>out=new ArrayList<>();if(ids==null)return out;
        for(Integer id:ids){if(id!=null&&id>=0)out.add(id);if(out.size()>=max)break;}
        return out;
    }
    private MusaAiDisciplineControl(){}
}
