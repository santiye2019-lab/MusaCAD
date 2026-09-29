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
    private static final Pattern GEO_SOIL_CLASS=Pattern.compile("(?iu)\\b(?:ZEM[İI]N\\s*SINIFI|GROUND\\s*TYPE)\\s*[:=]?\\s*(Z[A-F])\\b");
    private static final Pattern GEO_BEARING=Pattern.compile("(?iu)\\b(?:ZEM[İI]N\\s+(?:EMN[İI]YET\\s+GER[İI]LMES[İI]|TAŞIMA\\s+GÜCÜ|TASIMA\\s+GUCU)|ALLOWABLE\\s+BEARING\\s+(?:CAPACITY|PRESSURE)|BEARING\\s+CAPACITY)\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?\\s*(?:KPA|KN\\s*/\\s*M(?:2|²)|T\\s*/\\s*M(?:2|²)|KG\\s*/\\s*CM(?:2|²)))");
    private static final Pattern GEO_SUBGRADE=Pattern.compile("(?iu)\\b(?:YATAK\\s+KATSAYISI|ZEM[İI]N\\s+YATAK\\s+KATSAYISI|SUBGRADE\\s+MODULUS|MODULUS\\s+OF\\s+SUBGRADE\\s+REACTION|K[Ss])\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?\\s*(?:KN\\s*/\\s*M(?:3|³)|MN\\s*/\\s*M(?:3|³)|T\\s*/\\s*M(?:3|³)))");
    private static final Pattern GEO_GROUNDWATER=Pattern.compile("(?iu)\\b(?:YERALTI\\s+SUYU(?:\\s+SEV[İI]YES[İI])?|YER\\s+ALTI\\s+SUYU(?:\\s+SEV[İI]YES[İI])?|GROUNDWATER(?:\\s+LEVEL)?)\\s*[:=]?\\s*([+-]?[0-9]+(?:[\\.,][0-9]+)?\\s*M)");
    private static final Pattern GEO_FOUNDATION_LEVEL=Pattern.compile("(?iu)\\b(?:TEMEL\\s+ALT\\s+KOTU|FOUNDATION\\s+(?:BOTTOM|BASE)\\s+LEVEL)\\s*[:=]?\\s*([+-]?[0-9]+(?:[\\.,][0-9]+)?\\s*M)");

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
        cantileverChecks(refs,findings);
        transferChecks(refs,findings);
        slabServiceabilityChecks(refs,structuralCalc,findings);
        seismicParameterChecks(structuralCalc,findings);
        beamColumnJointChecks(refs,structuralCalc,findings);
        strongColumnWeakBeamChecks(refs,structuralCalc,findings);
        confinementChecks(refs,structuralCalc,findings);
        irregularityChecks(structuralCalc,findings);
        storyDriftChecks(structuralCalc,findings);
        torsionChecks(structuralCalc,findings);
        softWeakStoryChecks(structuralCalc,findings);
        modalChecks(structuralCalc,findings);
        shortColumnChecks(refs,structuralCalc,findings);
        couplingBeamChecks(refs,structuralCalc,findings);
        diaphragmChecks(refs,structuralCalc,findings);
        basementWallChecks(refs,findings);
        geotechnicalChecks(refs,structuralCalc,findings);
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

    public static boolean isFocusedQuery(String raw){
        return !focusIds(raw).isEmpty();
    }

    public static boolean focusedQueryNeedsReport(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return has(q,
            "modal","periyot","period","kutle katilim","mass participation",
            "kat otelen","story drift","burul","torsion","yumusak kat","soft story","zayif kat","weak story",
            "guclu kolon","strong column","zayif kiris","weak beam","kolon kiris birlesim","beam column joint",
            "zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu");
    }

    public static Result analyzeFocused(MusaAiDrawingIndex index,MusaAiStructuralCalc.Model calc,String raw){
        LinkedHashSet<String>wanted=focusIds(raw);
        if(wanted.isEmpty())return Result.none();
        Result all=analyze(index,calc);
        if(!all.matched)return all;

        ArrayList<Finding>filtered=new ArrayList<>();
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Finding f:all.findings)if(wanted.contains(f.id)){
            filtered.add(f);ids.addAll(f.sourceIds);
        }

        StringBuilder out=new StringBuilder("ODAKLI STATİK KONTROL • ").append(focusTitle(raw));
        if(filtered.isEmpty()){
            out.append("\n• Bu başlık için görünür proje/hesap verisinden eşleştirilebilir bulgu üretilemedi.");
            if(focusedQueryNeedsReport(raw)&&calc==null)
                out.append("\n• Bu kontrol için statik hesap raporu/model çıktısı gerekir.");
            out.append("\n• Sonuç yokluğu uygunluk onayı anlamına gelmez.");
            return new Result(true,out.toString(),Collections.emptyList(),Collections.emptyList());
        }

        int mismatch=0,review=0,unverified=0,info=0;
        for(Finding f:filtered){
            switch(f.status){
                case UYUMSUZLUK:mismatch++;break;
                case INCELEME_GEREKLI:review++;break;
                case DOGRULANAMADI:unverified++;break;
                default:info++;
            }
        }
        out.append("\n• UYUMSUZLUK: ").append(mismatch)
           .append(" • İNCELEME GEREKLİ: ").append(review)
           .append(" • DOĞRULANAMADI: ").append(unverified)
           .append(" • BİLGİ: ").append(info);
        for(Finding f:filtered){
            out.append("\n\n[").append(f.id).append("] ").append(label(f.status)).append(" • ").append(f.title);
            out.append("\n").append(f.detail);
            if(!f.suggestion.isEmpty())out.append("\nÖneri: ").append(f.suggestion);
        }
        out.append("\n\nNot: Odaklı kontrol açık proje/hesap verisini filtreler; eksik hesap sonucu veya güvenlik değeri uydurmaz.");
        return new Result(true,out.toString(),filtered,ids);
    }

    private static LinkedHashSet<String>focusIds(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        LinkedHashSet<String>ids=new LinkedHashSet<>();
        if(q.isEmpty())return ids;
        if(has(q,"zimbala","punching"))ids.add("ST-14");
        if(has(q,"modal","mod anal","response spectrum")){ids.add("ST-29");ids.add("ST-30");ids.add("ST-31");}
        if(has(q,"kutle katilim","mass participation","etkin modal kutle")){ids.add("ST-30");ids.add("ST-29");}
        if(has(q,"periyot","period"))ids.add("ST-31");
        if(has(q,"kat otelen","story drift","interstory drift"))ids.add("ST-26");
        if(has(q,"burul","torsion"))ids.add("ST-27");
        if(has(q,"yumusak kat","soft story","zayif kat","weak story"))ids.add("ST-28");
        if(has(q,"guclu kolon","strong column","zayif kiris","weak beam")){ids.add("ST-23");ids.add("ST-22");}
        if(has(q,"kolon kiris birlesim","kiris kolon birlesim","beam column joint"))ids.add("ST-22");
        if(has(q,"sarilma","siklastirma","confinement"))ids.add("ST-24");
        if(has(q,"transfer","aktarma")){ids.add("ST-17");ids.add("ST-02");ids.add("ST-03");ids.add("ST-04");}
        if(has(q,"konsol","cantilever"))ids.add("ST-16");
        if(has(q,"dilatasyon","deprem derzi","expansion joint"))ids.add("ST-12");
        if(has(q,"asansor kuyu","elevator shaft"))ids.add("ST-10");
        if(has(q,"merdiven","stair"))ids.add("ST-11");
        if(has(q,"rezervasyon","delik","opening","sleeve")){ids.add("ST-07");ids.add("ST-08");ids.add("ST-09");}
        if(has(q,"kisa kolon","short column"))ids.add("ST-32");
        if(has(q,"perde bag kirisi","coupling beam"))ids.add("ST-33");
        if(has(q,"rijit diyafram","rigid diaphragm","semi rigid","doseme sureksiz","slab discontinuity"))ids.add("ST-34");
        if(has(q,"bodrum perde","bodrum perdesi","basement wall","cevre perdesi")){ids.add("ST-35");ids.add("ST-03");}
        if(has(q,"zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu","zemin temel parametre")){ids.add("ST-36");ids.add("ST-37");}
        return ids;
    }

    private static String focusTitle(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(has(q,"zimbala","punching"))return "ZIMBALAMA";
        if(has(q,"modal","mod anal","response spectrum"))return "MODAL ANALİZ";
        if(has(q,"kutle katilim","mass participation"))return "MODAL KÜTLE KATILIMI";
        if(has(q,"periyot","period"))return "PERİYOT";
        if(has(q,"kat otelen","story drift"))return "KAT ÖTELENMESİ";
        if(has(q,"burul","torsion"))return "BURULMA";
        if(has(q,"yumusak kat","soft story","zayif kat","weak story"))return "YUMUŞAK / ZAYIF KAT";
        if(has(q,"guclu kolon","strong column","zayif kiris","weak beam"))return "GÜÇLÜ KOLON – ZAYIF KİRİŞ";
        if(has(q,"kolon kiris birlesim","beam column joint"))return "KİRİŞ–KOLON BİRLEŞİMİ";
        if(has(q,"sarilma","siklastirma","confinement"))return "SARILMA / SIKLAŞTIRMA";
        if(has(q,"transfer","aktarma"))return "TRANSFER / AKTARMA SİSTEMİ";
        if(has(q,"konsol","cantilever"))return "KONSOL";
        if(has(q,"dilatasyon","deprem derzi","expansion joint"))return "DİLATASYON";
        if(has(q,"asansor kuyu","elevator shaft"))return "ASANSÖR KUYUSU";
        if(has(q,"merdiven","stair"))return "MERDİVEN";
        if(has(q,"rezervasyon","delik","opening","sleeve"))return "REZERVASYON / BOŞLUK";
        if(has(q,"kisa kolon","short column"))return "KISA KOLON";
        if(has(q,"perde bag kirisi","coupling beam"))return "PERDE BAĞ KİRİŞİ";
        if(has(q,"rijit diyafram","rigid diaphragm","semi rigid","doseme sureksiz","slab discontinuity"))return "DİYAFRAM / DÖŞEME SÜREKLİLİĞİ";
        if(has(q,"bodrum perde","bodrum perdesi","basement wall","cevre perdesi"))return "BODRUM / PERDE SÜREKLİLİĞİ";
        if(has(q,"zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu","zemin temel parametre"))return "ZEMİN / TEMEL PARAMETRELERİ";
        return "STATİK";
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

    private static void cantileverChecks(List<Ref>refs,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();boolean cantilever=false,detail=false;
        for(Ref r:refs){
            if(has(r.q,"konsol","cantilever")){cantilever=true;addId(ids,r);}
            if(has(r.q,"ankraj","kenetlenme","ust donati","mesnet donati","anchorage","development length"))detail=true;
        }
        if(cantilever&&!detail)
            out.add(new Finding("ST-16",Status.DOGRULANAMADI,"Konsol ankraj / üst donatı doğrulaması",
                "Konsol eleman ifadesi bulundu ancak görünür indeks içinde açık ankraj, kenetlenme veya üst/mesnet donatısı detayı eşleştirilemedi.",
                "Konsol kök bölgesi üst donatısı, ankraj/kenetlenme boyu ve mesnet detayını statik pafta ve hesap çıktısıyla doğrulayın.",ids));
    }

    private static void transferChecks(List<Ref>refs,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();int n=0;
        for(Ref r:refs)if(has(r.q,"transfer kat","transfer doseme","transfer kiris","aktarma kiri","aktarma dose","transfer floor","transfer beam","transfer slab")){
            n++;addId(ids,r);
        }
        if(n>0)
            out.add(new Finding("ST-17",Status.INCELEME_GEREKLI,"Transfer katı / aktarma sistemi",
                n+" adet transfer/aktarma sistemi ifadesi bulundu.",
                "Üst kat kolon/perde yük aktarımını, transfer elemanlarının etiketlerini ve hesap modelindeki sürekliliği birlikte doğrulayın.",ids));
    }

    private static void slabServiceabilityChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        boolean slab=false,thickness=false,deflectionDrawing=false;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs){
            if(r.kind==Kind.SLAB){slab=true;addId(ids,r);}
            if(has(r.q,"doseme kalinligi","slab thickness","h doseme","h="))thickness=true;
            if(has(r.q,"sehim","deflection"))deflectionDrawing=true;
        }
        boolean deflectionReport=false;
        if(calc!=null){
            for(String cue:calc.loadCues)if(has(MusaAiDrawingIndex.normalize(cue),"sehim","deflection"))deflectionReport=true;
            for(String value:calc.seismicCues)if(has(MusaAiDrawingIndex.normalize(value),"sehim","deflection"))deflectionReport=true;
        }
        if(slab&&!thickness)
            out.add(new Finding("ST-18",Status.DOGRULANAMADI,"Döşeme kalınlığı otomatik doğrulanamadı",
                "Döşeme öğeleri tanındı ancak görünür indeks içinde açık döşeme kalınlığı ifadesi çıkarılamadı.",
                "Döşeme kalınlıklarını kalıp planı/lejandından görünür hale getirip rapor/model verisiyle karşılaştırın.",ids));
        if(deflectionDrawing&&!deflectionReport&&calc!=null)
            out.add(new Finding("ST-19",Status.DOGRULANAMADI,"Sehim sonucu rapor eşleşmesi",
                "Çizim/proje tarafında sehim ifadesi bulundu ancak yüklenen hesap verisinden açık sehim sonucu eşleştirilemedi.",
                "Servisabilite/sehim sonuçlarını içeren hesap raporu bölümünü veya model çıktısını yükleyin.",ids));
    }

    private static void seismicParameterChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        ArrayList<String>missing=new ArrayList<>();
        for(String key:Arrays.asList("SDS","SD1","DTS","BYS"))if(!calc.designParameters.containsKey(key))missing.add(key);
        if(!calc.designParameters.containsKey("Zemin Sınıfı"))missing.add("Zemin Sınıfı");
        if(!missing.isEmpty())
            out.add(new Finding("ST-20",Status.DOGRULANAMADI,"Deprem tasarım parametreleri eksik/okunamadı",
                "Yüklenen hesap verisinde şu parametreler açık biçimde ayrıştırılamadı: "+join(missing,10)+".",
                "SDS, SD1, DTS, BYS ve zemin sınıfını hesap raporunun proje bilgileri bölümünden doğrulayın.",Collections.emptyList()));
        if(calc.loadCues.isEmpty())
            out.add(new Finding("ST-21",Status.DOGRULANAMADI,"Yük kabulleri otomatik doğrulanamadı",
                "Hesap raporundan sabit/hareketli/kar/rüzgâr yüklerine ilişkin güvenilir metin ipucu çıkarılamadı.",
                "Yük kabulleri ve kombinasyon özetini içeren hesap bölümlerini rapora dahil edin.",Collections.emptyList()));
    }

    private static void beamColumnJointChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        boolean jointDrawing=false;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs)if(has(r.q,"kolon kiris birlesim","kiris kolon birlesim","birlesim bolgesi","beam column joint","joint region")){
            jointDrawing=true;addId(ids,r);
        }
        List<String>report=reportCues(calc,"kolon kiris birlesim","beam column joint","joint region");
        if(jointDrawing&&report.isEmpty()&&calc!=null)
            out.add(new Finding("ST-22",Status.DOGRULANAMADI,"Kiriş–kolon birleşim bölgesi rapor eşleşmesi",
                "DWG/proje tarafında birleşim bölgesi ifadesi bulundu ancak yüklenen hesap raporundan eşleşen birleşim kontrol satırı çıkarılamadı.",
                "Birleşim kesme güvenliği ve donatı detayına ilişkin hesap raporu bölümünü/etiketlerini proje ile eşleştirin.",ids));
        else if(!report.isEmpty())
            out.add(new Finding("ST-22",reportedStatus(report),"Kiriş–kolon birleşim kontrolü",
                "Hesap raporundan birleşim bölgesi kontrol verisi okundu: "+cueSummary(report,3)+".",
                "MusaCAD rapor beyanını aktarır; birleşim kapasitesini bağımsız olarak yeniden hesaplamaz. İlgili kolon/kiriş detayıyla çapraz kontrol edin.",ids));
    }

    private static void strongColumnWeakBeamChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>report=reportCues(calc,"guclu kolon","zayif kiris","strong column","weak beam");
        boolean frame=false;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs)if(r.kind==Kind.COLUMN||r.kind==Kind.BEAM){frame=true;addId(ids,r);}
        if(!report.isEmpty())
            out.add(new Finding("ST-23",reportedStatus(report),"Güçlü kolon–zayıf kiriş rapor kontrolü",
                "Hesap raporundan güçlü kolon–zayıf kiriş kontrolüne ilişkin satır okundu: "+cueSummary(report,4)+".",
                "Sonucun ilgili kat/aks birleşimleriyle eşleştiğini doğrulayın; MusaCAD burada oran hesabı uydurmaz.",ids));
        else if(frame)
            out.add(new Finding("ST-23",Status.DOGRULANAMADI,"Güçlü kolon–zayıf kiriş sonucu okunamadı",
                "Kolon ve kiriş verisi mevcut ancak yüklenen hesap raporundan açık güçlü kolon–zayıf kiriş kontrol satırı ayrıştırılamadı.",
                "Birleşim bazlı güçlü kolon–zayıf kiriş kontrol tablosunu içeren hesap raporu bölümünü yükleyin.",ids));
    }

    private static void confinementChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        boolean drawing=false,report=false,member=false;LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs){
            if(r.kind==Kind.COLUMN||r.kind==Kind.BEAM||r.kind==Kind.WALL){member=true;addId(ids,r);}
            if(has(r.q,"sarilma bolgesi","siklastirma bolgesi","ozel deprem etriyesi","confinement zone","special seismic hoop"))drawing=true;
        }
        if(calc!=null){
            for(MusaAiStructuralCalc.Element e:calc.elements)if(e!=null&&!e.confinement.isEmpty()){report=true;break;}
            if(!report&&!reportCues(calc,"sarilma bolgesi","siklastirma bolgesi","confinement zone","ozel deprem etriyesi").isEmpty())report=true;
        }
        if(member&&!drawing&&!report)
            out.add(new Finding("ST-24",Status.DOGRULANAMADI,"Sarılma / sıklaştırma bölgesi doğrulanamadı",
                "Taşıyıcı kolon-kiriş-perde verisi bulundu ancak çizim veya hesap verisinde açık sarılma/sıklaştırma bölgesi eşleştirilemedi.",
                "Özel deprem etriyeleri, sarılma boyları ve birleşim bölgesi detaylarını görünür pafta/hesap verisiyle doğrulayın.",ids));
        else if(drawing&&calc!=null&&!report)
            out.add(new Finding("ST-24",Status.DOGRULANAMADI,"Sarılma detayı proje–rapor eşleşmesi",
                "Çizimde sarılma/sıklaştırma ifadesi bulundu ancak hesap raporunda eşleşen açık detay verisi çıkarılamadı.",
                "Kat/eleman etiketleri üzerinden sarılma bölgesini hesap raporuyla eşleştirin.",ids));
    }

    private static void irregularityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"duzensizlik","irregularity","a1","a2","a3","b1","b2","b3");
        if(cues.isEmpty())
            out.add(new Finding("ST-25",Status.DOGRULANAMADI,"Düzensizlik kontrol özeti okunamadı",
                "Yüklenen hesap raporundan A/B tipi düzensizliklere ilişkin açık kontrol satırı ayrıştırılamadı.",
                "Düzensizlikler/deprem kontrol özeti bölümünü rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-25",reportedStatus(cues),"Düzensizlik rapor özeti",
                "Hesap raporundan düzensizlik kontrolüne ilişkin veri okundu: "+cueSummary(cues,5)+".",
                "Her düzensizlik türünü ilgili kat, geometrik veri ve analiz sonucu ile ayrı doğrulayın.",Collections.emptyList()));
    }

    private static void storyDriftChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kat otelemesi","goreli kat otelemesi","story drift","interstory drift");
        if(cues.isEmpty())
            out.add(new Finding("ST-26",Status.DOGRULANAMADI,"Göreli kat ötelenmesi sonucu okunamadı",
                "Hesap raporunda açık kat/göreli kat ötelenmesi satırı ayrıştırılamadı.",
                "Kat bazlı ötelenme tablosunu veya analiz programı özetini rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-26",reportedStatus(cues),"Göreli kat ötelenmesi rapor kontrolü",
                "Hesap raporundan kat ötelenmesi verisi okundu: "+cueSummary(cues,4)+".",
                "MusaCAD rapordaki sonucu aktarır; sınır değer hesabını veri olmadan yeniden üretmez.",Collections.emptyList()));
    }

    private static void torsionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"burulma","torsional irregularity","eta bi","etabi");
        if(cues.isEmpty())
            out.add(new Finding("ST-27",Status.DOGRULANAMADI,"Burulma düzensizliği sonucu okunamadı",
                "Hesap raporundan burulma düzensizliği/katsayısına ilişkin açık satır ayrıştırılamadı.",
                "Kat bazlı burulma düzensizliği kontrol tablosunu rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-27",reportedStatus(cues),"Burulma düzensizliği rapor kontrolü",
                "Hesap raporundan burulma kontrol verisi okundu: "+cueSummary(cues,4)+".",
                "Kritik katları plan geometrisi, rijitlik dağılımı ve hesap modeliyle çapraz kontrol edin.",Collections.emptyList()));
    }

    private static void softWeakStoryChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"yumusak kat","soft story","zayif kat","weak story");
        if(cues.isEmpty())
            out.add(new Finding("ST-28",Status.DOGRULANAMADI,"Yumuşak / zayıf kat sonucu okunamadı",
                "Yüklenen hesap raporundan yumuşak veya zayıf kat kontrolüne ilişkin açık satır ayrıştırılamadı.",
                "Kat rijitlik/dayanım karşılaştırması bölümünü rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-28",reportedStatus(cues),"Yumuşak / zayıf kat rapor kontrolü",
                "Hesap raporundan ilgili kontrol verisi okundu: "+cueSummary(cues,4)+".",
                "Rapor sonucunu kat bazlı taşıyıcı sistem ve model rijitlikleriyle doğrulayın.",Collections.emptyList()));
    }

    private static void modalChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>modal=reportCues(calc,"modal","mod sekli","mode shape","mod birlestirme","response spectrum","modal combination");
        List<String>mass=reportCues(calc,"kutle katilim","mass participation","etkin modal kutle","effective modal mass");
        List<String>period=reportCues(calc,"periyot","period");
        if(modal.isEmpty())
            out.add(new Finding("ST-29",Status.DOGRULANAMADI,"Modal analiz özeti okunamadı",
                "Hesap raporundan modal analiz/mod birleştirme yöntemine ilişkin açık satır ayrıştırılamadı.",
                "Modal analiz ve mod birleştirme özetini rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-29",reportedStatus(modal),"Modal analiz rapor özeti",
                "Modal analiz verisi okundu: "+cueSummary(modal,4)+".",
                "Mod sayısı ve kullanılan analiz yöntemini hesap modeliyle doğrulayın.",Collections.emptyList()));

        if(mass.isEmpty())
            out.add(new Finding("ST-30",Status.DOGRULANAMADI,"Modal kütle katılımı okunamadı",
                "Hesap raporundan etkin/modal kütle katılımına ilişkin açık satır ayrıştırılamadı.",
                "X/Y doğrultuları için kümülatif kütle katılım tablosunu rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-30",reportedStatus(mass),"Modal kütle katılım raporu",
                "Hesap raporundan kütle katılım verisi okundu: "+cueSummary(mass,4)+".",
                "Kümülatif katılımın kullanılan mod sayısıyla birlikte rapor/model üzerinde doğrulanması gerekir.",Collections.emptyList()));

        boolean t1=calc.designParameters.containsKey("T1X")||calc.designParameters.containsKey("T1Y");
        if(period.isEmpty()&&!t1)
            out.add(new Finding("ST-31",Status.DOGRULANAMADI,"Hakim periyot verisi okunamadı",
                "Hesap raporundan T1/periyot değerleri açık biçimde ayrıştırılamadı.",
                "X/Y doğrultusu hakim periyotları ve modal periyot tablosunu rapora dahil edin.",Collections.emptyList()));
        else{
            String values="";
            if(calc.designParameters.containsKey("T1X"))values+="T1X="+calc.designParameters.get("T1X");
            if(calc.designParameters.containsKey("T1Y"))values+=(values.isEmpty()?"":" • ")+"T1Y="+calc.designParameters.get("T1Y");
            if(values.isEmpty())values=cueSummary(period,3);
            out.add(new Finding("ST-31",Status.BILGI,"Hakim periyot / modal periyot verisi",
                "Hesap raporundan periyot verisi okundu: "+values+".",
                "Değerlerin doğru model, yön ve son revizyon analizine ait olduğunu doğrulayın.",Collections.emptyList()));
        }
    }

    private static List<String> reportCues(MusaAiStructuralCalc.Model calc,String...terms){
        ArrayList<String>out=new ArrayList<>();
        if(calc==null)return out;
        for(String cue:calc.seismicCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,terms)&&!out.contains(cue))out.add(cue);
        }
        return out;
    }

    private static Status reportedStatus(Collection<String>cues){
        boolean negative=false,positive=false;
        for(String cue:cues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,"saglamiyor","saglanmiyor","uygunsuz","yetersiz","basarisiz","fail","failed","not satisfied","not comply","does not comply"))negative=true;
            if(has(q,"sagliyor","saglanmistir","uygun","yeterli","basarili","satisfied","pass","passed","complies"))positive=true;
        }
        if(negative)return Status.UYUMSUZLUK;
        if(positive)return Status.BILGI;
        return Status.INCELEME_GEREKLI;
    }

    private static String cueSummary(Collection<String>cues,int max){
        if(cues==null||cues.isEmpty())return "—";
        ArrayList<String>shortened=new ArrayList<>();int n=0;
        for(String cue:cues){
            if(n++>=max)break;
            String x=cue==null?"":cue.trim().replaceAll("\\s+"," ");
            if(x.length()>150)x=x.substring(0,150)+"…";
            if(!x.isEmpty())shortened.add(x);
        }
        return join(shortened,max);
    }

    private static void shortColumnChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        ArrayList<String>drawing=new ArrayList<>();
        for(Ref r:refs)if(has(r.q,"kisa kolon","short column")){
            drawing.add(location(r));addId(ids,r);
        }
        List<String>report=reportCues(calc,"kisa kolon","short column");
        if(drawing.isEmpty()&&report.isEmpty())return;
        Status status=!report.isEmpty()?reportedStatus(report):(calc==null?Status.INCELEME_GEREKLI:Status.DOGRULANAMADI);
        String detail=!report.isEmpty()
            ?"Hesap raporundan kısa kolon ile ilgili veri okundu: "+cueSummary(report,4)+"."
            :"Çizimde kısa kolon ifadesi bulunan kayıtlar: "+join(drawing,6)+".";
        out.add(new Finding("ST-32",status,"Kısa kolon kontrol adayı",detail,
            "Serbest kolon yüksekliği, dolgu/parapet etkisi, kesme talebi ve özel sarılma detayını hesap modeli ile paftada birlikte doğrulayın; MusaCAD kısa kolon dayanımı hesaplamaz.",ids));
    }

    private static void couplingBeamChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        ArrayList<String>drawing=new ArrayList<>();
        boolean specialDetail=false;
        for(Ref r:refs){
            boolean coupling=has(r.q,"perde bag kirisi","perde baglant i kirisi","coupling beam")||
                (has(r.q,"bag kirisi")&&has(r.q,"perde"));
            if(coupling){drawing.add(location(r));addId(ids,r);}
            if(coupling&&has(r.q,"capraz donati","diagonal donati","diagonal reinforcement","ozel etriye","special hoop"))specialDetail=true;
        }
        List<String>report=reportCues(calc,"perde bag kirisi","coupling beam");
        if(drawing.isEmpty()&&report.isEmpty())return;
        Status status=!report.isEmpty()?reportedStatus(report):(calc==null?Status.INCELEME_GEREKLI:Status.DOGRULANAMADI);
        String detail=!report.isEmpty()
            ?"Hesap raporundan perde bağ kirişi verisi okundu: "+cueSummary(report,4)+"."
            :"Çizimde perde bağ kirişi olarak tanınan kayıtlar: "+join(drawing,6)+".";
        if(specialDetail)detail+=" Çizimde özel/çapraz donatı ifadesi de görüldü.";
        out.add(new Finding("ST-33",status,"Perde bağ kirişi / coupling beam kontrolü",detail,
            "Bağ kirişinin perde açıklığı geometrisi, kesit ve özel donatı çözümünü aynı kat/aks için hesap raporu ve detay paftasında eşleştirin.",ids));
    }

    private static void diaphragmChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        ArrayList<String>drawing=new ArrayList<>();
        int openingCount=0;
        for(Ref r:refs){
            if(has(r.q,"rijit diyafram","rigid diaphragm","semi rigid diaphragm","yar i rijit diyafram","yari rijit diyafram","doseme sureksiz","slab discontinuity")){
                drawing.add(location(r));addId(ids,r);
            }
            if(r.kind==Kind.SLAB&&has(r.q,"bosluk","rezervasyon","saft","shaft","opening")){openingCount++;addId(ids,r);}
        }
        List<String>report=reportCues(calc,"rijit diyafram","rigid diaphragm","semi rigid diaphragm","doseme sureksiz","slab discontinuity");
        if(drawing.isEmpty()&&report.isEmpty())return;
        Status status=!report.isEmpty()?reportedStatus(report):(calc==null?Status.INCELEME_GEREKLI:Status.DOGRULANAMADI);
        String detail=!report.isEmpty()
            ?"Hesap raporundan diyafram/döşeme sürekliliği verisi okundu: "+cueSummary(report,4)+"."
            :"Çizimde diyafram/döşeme sürekliliği ifadesi bulundu: "+join(drawing,6)+".";
        if(openingCount>0)detail+=" Aynı görünür statik veride "+openingCount+" adet döşeme boşluğu/rezervasyon kaydı da bulunuyor.";
        out.add(new Finding("ST-34",status,"Rijit diyafram / döşeme sürekliliği",detail,
            "Diyafram kabulünü büyük boşluklar, şaftlar, dilatasyonlar ve döşeme süreksizlikleriyle birlikte hesap modelinde doğrulayın; otomatik tarama diyafram rijitliği hesabı yapmaz.",ids));
    }

    private static void basementWallChecks(List<Ref>refs,List<Finding>out){
        LinkedHashMap<String,ArrayList<Ref>>byTag=new LinkedHashMap<>();
        for(Ref r:refs)if(r.kind==Kind.WALL&&!r.tag.isEmpty()&&!r.floor.isEmpty())
            byTag.computeIfAbsent(r.tag,k->new ArrayList<>()).add(r);

        for(Map.Entry<String,ArrayList<Ref>>e:byTag.entrySet()){
            ArrayList<Ref>basement=new ArrayList<>(),upper=new ArrayList<>();
            for(Ref r:e.getValue()){
                Integer fo=floorOrder(r.floor);if(fo==null)continue;
                if(fo<0)basement.add(r);else upper.add(r);
            }
            if(basement.isEmpty()||upper.isEmpty())continue;
            LinkedHashSet<String>bAxes=new LinkedHashSet<>(),uAxes=new LinkedHashSet<>(),bSections=new LinkedHashSet<>(),uSections=new LinkedHashSet<>();
            LinkedHashSet<Integer>ids=new LinkedHashSet<>();
            for(Ref r:basement){if(!r.axis.isEmpty())bAxes.add(r.axis);if(!r.section.isEmpty())bSections.add(r.section);addId(ids,r);}
            for(Ref r:upper){if(!r.axis.isEmpty())uAxes.add(r.axis);if(!r.section.isEmpty())uSections.add(r.section);addId(ids,r);}
            boolean axisMismatch=!bAxes.isEmpty()&&!uAxes.isEmpty()&&Collections.disjoint(bAxes,uAxes);
            boolean sectionChange=!bSections.isEmpty()&&!uSections.isEmpty()&&!bSections.equals(uSections);
            if(axisMismatch||sectionChange){
                StringBuilder detail=new StringBuilder(e.getKey()+" perdesi bodrumdan zemin/üst katlara devam ediyor.");
                if(axisMismatch)detail.append(" Bodrum aksları ").append(join(bAxes,6)).append(", üst yapı aksları ").append(join(uAxes,6)).append(".");
                if(sectionChange)detail.append(" Bodrum kesitleri ").append(join(bSections,6)).append(", üst yapı kesitleri ").append(join(uSections,6)).append(".");
                out.add(new Finding("ST-35",Status.INCELEME_GEREKLI,"Bodrum–üst yapı perde geçişi",detail.toString(),
                    "Bodrum çevre/perde sisteminin üst yapı perdesiyle aynı taşıyıcı hat üzerinde devam edip etmediğini ve geçiş detayının hesap modelinde karşılığını doğrulayın.",ids));
            }
        }
    }

    private static void geotechnicalChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        LinkedHashMap<String,String>drawing=projectGeoParameters(refs);
        LinkedHashMap<String,String>report=new LinkedHashMap<>();
        for(String key:Arrays.asList("Zemin Sınıfı","Zemin Taşıma Gücü","Yatak Katsayısı","Yeraltı Suyu","Temel Alt Kotu"))
            if(calc.designParameters.containsKey(key))report.put(key,normalizeGeoValue(calc.designParameters.get(key)));

        boolean foundationContext=!calc.foundationTypes.isEmpty()||!drawing.isEmpty()||
            !reportCues(calc,"zemin tasima gucu","zemin emniyet gerilmesi","allowable bearing","bearing capacity","yatak katsayisi","subgrade modulus","temel alt kotu","groundwater","yeralti suyu").isEmpty();
        if(!foundationContext)return;

        ArrayList<String>missing=new ArrayList<>();
        if(!report.containsKey("Zemin Sınıfı"))missing.add("Zemin Sınıfı");
        boolean pileOnly=calc.foundationTypes.size()==1&&calc.foundationTypes.contains("KAZIK");
        if(!pileOnly&&!report.containsKey("Zemin Taşıma Gücü")&&!report.containsKey("Yatak Katsayısı"))
            missing.add("Zemin Taşıma Gücü / Yatak Katsayısı");
        if(!missing.isEmpty())
            out.add(new Finding("ST-36",Status.DOGRULANAMADI,"Zemin / temel tasarım parametreleri eksik veya okunamadı",
                "Yüklenen statik hesap verisinde şu zemin/temel parametreleri açık biçimde ayrıştırılamadı: "+join(missing,8)+".",
                "Zemin ve temel tasarımına esas değerleri geoteknik raporun ilgili sayfası ve statik hesap kabulüyle doğrulayın; MusaCAD eksik değeri yönetmelikten türetmez.",Collections.emptyList()));
        else
            out.add(new Finding("ST-36",Status.BILGI,"Zemin / temel parametreleri okundu",
                "Statik hesap verisinden okunan parametreler: "+geoSummary(report)+".",
                "Bu değerlerin güncel geoteknik rapor ve temel projesiyle aynı revizyona ait olduğunu doğrulayın.",Collections.emptyList()));

        ArrayList<String>same=new ArrayList<>(),different=new ArrayList<>();
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Map.Entry<String,String>e:drawing.entrySet()){
            String rv=report.get(e.getKey());if(rv==null)continue;
            if(sameGeoValue(e.getValue(),rv))same.add(e.getKey()+"="+e.getValue());
            else different.add(e.getKey()+": proje "+e.getValue()+" • hesap "+rv);
        }
        for(Ref r:refs)if(has(r.q,"zemin tasima gucu","zemin emniyet gerilmesi","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu","zemin sinifi"))addId(ids,r);
        if(!different.isEmpty())
            out.add(new Finding("ST-37",Status.UYUMSUZLUK,"Zemin / temel parametre proje–hesap farkı",
                "Açıkça okunabilen ortak parametrelerde fark bulundu: "+join(different,8)+".",
                "Geoteknik rapor, temel paftası ve hesap modelindeki kabul değerlerini aynı revizyon üzerinden karşılaştırın.",ids));
        else if(!same.isEmpty())
            out.add(new Finding("ST-37",Status.BILGI,"Zemin / temel parametre proje–hesap eşleşmesi",
                "Açıkça okunabilen ortak parametreler eşleşiyor: "+join(same,8)+".",
                "Eşleşme yalnız görünür metin değerleri içindir; temel taşıma gücü veya oturma hesabı yapılmamıştır.",ids));
    }

    private static LinkedHashMap<String,String>projectGeoParameters(List<Ref>refs){
        LinkedHashMap<String,String>out=new LinkedHashMap<>();
        for(Ref r:refs){
            putGeo(out,"Zemin Sınıfı",GEO_SOIL_CLASS,r.raw);
            putGeo(out,"Zemin Taşıma Gücü",GEO_BEARING,r.raw);
            putGeo(out,"Yatak Katsayısı",GEO_SUBGRADE,r.raw);
            putGeo(out,"Yeraltı Suyu",GEO_GROUNDWATER,r.raw);
            putGeo(out,"Temel Alt Kotu",GEO_FOUNDATION_LEVEL,r.raw);
        }
        return out;
    }

    private static void putGeo(Map<String,String>out,String key,Pattern pattern,String raw){
        Matcher m=pattern.matcher(raw==null?"":raw);
        if(m.find())out.putIfAbsent(key,normalizeGeoValue(m.group(1)));
    }

    private static String normalizeGeoValue(String raw){
        if(raw==null)return "";
        String value=raw.toUpperCase(new Locale("tr","TR")).replace(',','.').replace("²","2").replace("³","3").replaceAll("\\s+","");
        if(value.endsWith("KN/M2"))value=value.substring(0,value.length()-5)+"KPA";
        if(value.endsWith("MN/M3")){
            String n=value.substring(0,value.length()-5);
            try{return canonicalNumber(Double.parseDouble(n)*1000d)+"KN/M3";}catch(Exception ignored){}
        }
        return value;
    }

    private static boolean sameGeoValue(String a,String b){return normalizeGeoValue(a).equals(normalizeGeoValue(b));}

    private static String canonicalNumber(double value){
        if(Math.abs(value-Math.rint(value))<1e-9)return Long.toString(Math.round(value));
        String s=Double.toString(value);
        while(s.endsWith("0"))s=s.substring(0,s.length()-1);
        return s.endsWith(".")?s.substring(0,s.length()-1):s;
    }

    private static String geoSummary(Map<String,String>values){
        ArrayList<String>items=new ArrayList<>();
        for(Map.Entry<String,String>e:values.entrySet())items.add(e.getKey()+"="+e.getValue());
        return join(items,10);
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
        return r.kind!=Kind.OTHER||has(r.q,"statik","betonarme","tasiyici","donati","rezervasyon","bosluk","dilatasyon","zimbalama","kazik","radye",
            "kisa kolon","short column","perde bag kirisi","coupling beam","rijit diyafram","rigid diaphragm","semi rigid",
            "doseme sureksiz","slab discontinuity","bodrum perdesi","basement wall","cevre perdesi",
            "zemin tasima gucu","zemin emniyet gerilmesi","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu");
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
