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
        foundationBehaviorChecks(structuralCalc,findings);
        pileCapacityChecks(structuralCalc,findings);
        upliftChecks(structuralCalc,findings);
        secondOrderChecks(structuralCalc,findings);
        baseShearChecks(structuralCalc,findings);
        soilStructureInteractionChecks(structuralCalc,findings);
        massSourceChecks(structuralCalc,findings);
        centerEccentricityChecks(structuralCalc,findings);
        accidentalEccentricityChecks(structuralCalc,findings);
        diaphragmForcePathChecks(refs,structuralCalc,findings);
        verticalSeismicChecks(structuralCalc,findings);
        loadCombinationChecks(structuralCalc,findings);
        seismicLoadCaseChecks(structuralCalc,findings);
        rdiSystemChecks(structuralCalc,findings);
        effectiveStiffnessChecks(structuralCalc,findings);
        releaseRigidZoneChecks(structuralCalc,findings);
        columnWallInteractionChecks(structuralCalc,findings);
        beamCapacityChecks(structuralCalc,findings);
        wallShearCapacityChecks(structuralCalc,findings);
        explicitCapacityFailureChecks(structuralCalc,findings);
        utilizationRankingChecks(structuralCalc,findings);
        deflectionServiceabilityChecks(structuralCalc,findings);
        crackWidthChecks(structuralCalc,findings);
        vibrationServiceabilityChecks(structuralCalc,findings);
        longTermEffectChecks(structuralCalc,findings);
        explicitServiceabilityFailureChecks(structuralCalc,findings);
        modelInstabilityChecks(structuralCalc,findings);
        disconnectedModelChecks(structuralCalc,findings);
        meshQualityChecks(structuralCalc,findings);
        convergenceChecks(structuralCalc,findings);
        localAxisChecks(structuralCalc,findings);
        revisionConsistencyChecks(refs,structuralCalc,findings);
        materialAssignmentChecks(structuralCalc,findings);
        sectionAssignmentIntegrityChecks(structuralCalc,findings);
        storyElementMetadataChecks(structuralCalc,findings);
        duplicateElementIdentityChecks(structuralCalc,findings);
        degenerateGeometryChecks(structuralCalc,findings);
        supportBoundaryAssignmentChecks(structuralCalc,findings);
        diaphragmConstraintAssignmentChecks(structuralCalc,findings);
        loadAssignmentIntegrityChecks(structuralCalc,findings);
        selfWeightGravityChecks(structuralCalc,findings);
        unitSystemChecks(structuralCalc,findings);
        designCodeVersionChecks(structuralCalc,findings);
        storyElevationCoordinateChecks(structuralCalc,findings);
        analysisCaseRunStatusChecks(structuralCalc,findings);
        loadCaseReferenceIntegrityChecks(structuralCalc,findings);
        objectPropertyCompatibilityChecks(structuralCalc,findings);
        shellThicknessAssignmentChecks(structuralCalc,findings);
        pierSpandrelAssignmentChecks(structuralCalc,findings);
        designProcedureStatusChecks(structuralCalc,findings);
        autoMeshAssignmentChecks(structuralCalc,findings);
        loadPatternTypeChecks(structuralCalc,findings);
        responseSpectrumAssignmentChecks(structuralCalc,findings);
        modalCaseSetupChecks(structuralCalc,findings);
        dampingDefinitionChecks(structuralCalc,findings);
        dynamicCombinationMethodChecks(structuralCalc,findings);
        timeHistoryFunctionChecks(structuralCalc,findings);
        timeHistoryStepChecks(structuralCalc,findings);
        nonlinearHingeAssignmentChecks(structuralCalc,findings);
        nonlinearCaseControlChecks(structuralCalc,findings);
        stagedConstructionChecks(structuralCalc,findings);
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
            "zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu",
            "zemin basinci","temel basinci","soil pressure","oturma","settlement","kazik kapasitesi","pile capacity","pile load",
            "uplift","yuzme","hidrostatik","p-delta","p delta","ikinci mertebe","second order",
            "taban kesme","base shear","spektrum olcekle","spectrum scale","zemin yapi etkilesimi","soil structure interaction","yay katsayisi","spring stiffness",
            "kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass",
            "kutle merkezi","rijitlik merkezi","center of mass","center of rigidity","eksantrisite","eccentricity",
            "tesadufi eksantrisite","accidental eccentricity","collector","drag strut","diaphragm chord",
            "dusey deprem","dikey deprem","vertical earthquake","vertical seismic",
            "yuk kombinasyonu","load combination","load combo","kombinasyon",
            "deprem yuk durum","seismic load case","earthquake load case","rsx","rsy",
            "r/d/i","r d i","tasiyici sistem katsay","behavior factor","overstrength","importance factor",
            "etkin rijitlik","catlamis kesit","çatlamış kesit","cracked section","effective stiffness","stiffness modifier","property modifier",
            "mafsal","hinge","release","end release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset",
            "sehim","deflection","servisabilite","serviceability",
            "catlak genisligi","çatlak genişliği","crack width",
            "titresim","titreşim","vibration","comfort frequency","floor frequency",
            "sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection",
            "servis siniri","servis sınırı","serviceability limit",
            "singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness",
            "unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint",
            "mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi",
            "nonconvergence","non-convergence","did not converge","not converged","convergence failed","yakinsamadi","yakınsamadı","yakinsama hatasi","yakınsama hatası","iteration limit","iterasyon limiti",
            "local axis","local axes","yerel eksen","orientation assignment","section orientation","major axis","minor axis","eleman yonu","eleman yönü",
            "revizyon","revision","rev no","revizyon no","model revision","model rev",
            "material assignment","material property","malzeme atama","malzeme ataması","undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material",
            "section assignment","section property","kesit atama","kesit ataması","undefined section","section not assigned","property not assigned","default section",
            "story assignment","story data","kat atama","kat bilgisi","unknown story","undefined story","floor assignment",
            "duplicate element","duplicate joint","duplicate member","mukerrer eleman","mükerrer eleman","conflicting assignment","çelişkili atama","celiskili atama",
            "zero length","zero-length","very short element","very short member","coincident joint","coincident node","sifir uzunluk","sıfır uzunluk","degenerate element",
            "support restraint","joint restraint","boundary condition","mesnet atama","mesnet tanimi","mesnet tanımı","support not assigned","missing restraint","unrestrained joint",
            "diaphragm assignment","diaphragm constraint","constraint assignment","diyafram atama","diyafram tanimi","diyafram tanımı","diaphragm not assigned",
            "load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama",
            "self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier",
            "unit system","model units","birim sistemi","birim ayari","birim ayarı","unit mismatch","birim uyumsuz",
            "design code","code version","tasarim yonetmeligi","tasarım yönetmeliği","yonetmelik surumu","yönetmelik sürümü","tbdy","ts500",
            "story elevation","floor elevation","kat kotu","kat kotlari","kat kotları","coordinate system","koordinat sistemi","elevation mismatch",
            "analysis case","run status","analysis status","not run","case failed","analysis outdated","analiz durumu","analiz case","case durumu","analiz case durumu","calistirilmamis","çalıştırılmamış",
            "duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","load case referans","load case referansi","load case referansı","yuk durumu referansi","yük durumu referansı",
            "object property compatibility","property type mismatch","wrong property type","property tip","property uyumluluk","eleman property uyumsuz","nesne property uyumsuz","frame property area","area property frame",
            "shell thickness","area thickness","slab thickness property","wall thickness property","thickness not assigned","default thickness","kabuk kalinligi","kabuk kalınlığı","alan kalinligi","alan kalınlığı",
            "pier label","spandrel label","pier assignment","spandrel assignment","unassigned pier","unassigned spandrel","perde pier","spandrel atama",
            "design procedure","design status","not designed","design excluded","no design","check only","design group","tasarim durumu","tasarım durumu","tasarim disi","tasarım dışı",
            "auto mesh","automatic mesh","area mesh assignment","mesh assignment","mesh not assigned","unmeshed area","otomatik mesh","mesh atama",
            "load pattern type","load pattern category","pattern type mismatch","dead pattern","live pattern","wind pattern","snow pattern","quake pattern","yuk pattern tipi","yük pattern tipi",
            "response spectrum function","spectrum function","spectrum direction","ux uy uz","rs direction","spektrum fonksiyonu","spektrum yonu","spektrum yönü",
            "modal case method","eigen method","ritz vector","ritz vectors","modal source","modal setup","modal yontem","modal yöntem","ritz vektoru","ritz vektörü",
            "damping ratio","modal damping","response spectrum damping","rayleigh damping","sonum orani","sönüm oranı","modal sonum","modal sönüm",
            "modal combination","cqc","srss","directional combination","direction combination","modal birlestirme","modal birleştirme","yon birlestirme","yön birleştirme",
            "time history function","time history case","ground motion function","record function","zaman tanim alani","zaman tanım alanı","zaman gecmisi","zaman geçmişi",
            "time step","time increment","number of output steps","output time step","duration","zaman adimi","zaman adımı","analiz suresi","analiz süresi",
            "nonlinear hinge assignment","plastic hinge assignment","hinge property","hinge assignment","nonlinear hinge","plastik mafsal atama","plastik mafsal",
            "nonlinear case parameters","nonlinear solution control","maximum iterations","iteration tolerance","event stepping","nonlinear control","dogrusal olmayan analiz ayari","doğrusal olmayan analiz ayarı",
            "staged construction","construction stage","stage definition","stage sequence","staged nonlinear","asama tanimi","aşama tanımı","yapim asamasi","yapım aşaması");
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
        boolean dynamicSetupQuery=has(q,
            "load pattern type","load pattern category","pattern type mismatch","yuk pattern tipi","yük pattern tipi",
            "response spectrum function","spectrum function","spectrum direction","rs direction","spektrum fonksiyonu","spektrum yonu","spektrum yönü",
            "modal case method","eigen method","ritz vector","ritz vectors","modal source","modal setup","modal yontem","modal yöntem","ritz vektoru","ritz vektörü",
            "damping ratio","modal damping","response spectrum damping","rayleigh damping","sonum orani","sönüm oranı","modal sonum","modal sönüm",
            "modal combination","cqc","srss","directional combination","direction combination","yon birlestirme","yön birleştirme");
        if(has(q,"zimbala","punching"))ids.add("ST-14");
        if(!dynamicSetupQuery&&has(q,"modal","mod anal","response spectrum")){ids.add("ST-29");ids.add("ST-30");ids.add("ST-31");}
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
        if(has(q,"zemin basinci","temel basinci","soil pressure","contact pressure","oturma","settlement"))ids.add("ST-38");
        if(has(q,"kazik kapasitesi","kazik tasima","pile capacity","pile load","kazik yuk"))ids.add("ST-39");
        if(has(q,"uplift","yuzme","hidrostatik","hydrostatic","buoyancy"))ids.add("ST-40");
        if(has(q,"p-delta","p delta","ikinci mertebe","second order","second-order"))ids.add("ST-41");
        if(has(q,"taban kesme","base shear","spektrum olcekle","spectrum scale","scaling"))ids.add("ST-42");
        if(has(q,"zemin yapi etkilesimi","soil structure interaction","soil-structure interaction","yay katsayisi","spring stiffness","area spring"))ids.add("ST-43");
        if(has(q,"kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass"))ids.add("ST-44");
        boolean accidental=has(q,"tesadufi eksantrisite","accidental eccentricity","additional eccentricity");
        if(!accidental&&has(q,"kutle merkezi","rijitlik merkezi","center of mass","centre of mass","center of rigidity","centre of rigidity","eksantrisite","eccentricity"))ids.add("ST-45");
        if(accidental)ids.add("ST-46");
        if(has(q,"collector","drag strut","diyafram kiri","diaphragm chord","chord force"))ids.add("ST-47");
        if(has(q,"dusey deprem","dikey deprem","vertical earthquake","vertical seismic"))ids.add("ST-48");
        if(has(q,"yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon"))ids.add("ST-49");
        if(has(q,"deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","rsx","rsy"))ids.add("ST-50");
        if(has(q,"r/d/i","r d i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor"))ids.add("ST-51");
        if(has(q,"etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier","property modifier","rijitlik carpani","rijitlik çarpanı"))ids.add("ST-52");
        if(has(q,"mafsal","hinge","release","end release","moment release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset"))ids.add("ST-53");
        if(has(q,"pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı"))ids.add("ST-54");
        if(has(q,"kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment","flexural ratio","moment ratio"))ids.add("ST-55");
        if(has(q,"perde kesme","wall shear","shear wall shear","kesme kapasitesi","shear capacity"))ids.add("ST-56");
        if(has(q,"kapasite asimi","kapasite aşımı","capacity failure","failed capacity","yetersiz eleman","uygunsuz eleman"))ids.add("ST-57");
        if(has(q,"kapasite orani","kapasite oranı","capacity ratio","utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio","kritik eleman"))ids.add("ST-58");
        if(has(q,"sehim","deflection","servisabilite","serviceability"))ids.add("ST-59");
        if(has(q,"catlak genisligi","çatlak genişliği","crack width"))ids.add("ST-60");
        if(has(q,"titresim","titreşim","vibration","comfort frequency","floor frequency"))ids.add("ST-61");
        if(has(q,"sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection"))ids.add("ST-62");
        if(has(q,"servis siniri","servis sınırı","serviceability limit","sehim asimi","sehim aşımı","catlak asimi","çatlak aşımı","titresim limiti","titreşim limiti"))ids.add("ST-63");
        if(has(q,"singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness"))ids.add("ST-64");
        if(has(q,"unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint"))ids.add("ST-65");
        if(has(q,"mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi"))ids.add("ST-66");
        if(has(q,"nonconvergence","non-convergence","did not converge","not converged","convergence failed","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti"))ids.add("ST-67");
        if(has(q,"local axis","local axes","yerel eksen","orientation assignment","section orientation","major axis","minor axis","eleman yonu","eleman yönü"))ids.add("ST-68");
        if(has(q,"revizyon","revision","rev no","revizyon no","model revision","model rev"))ids.add("ST-69");
        if(has(q,"material assignment","material property","malzeme atama","malzeme ataması","undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material"))ids.add("ST-70");
        if(has(q,"section assignment","section property","kesit atama","kesit ataması","undefined section","section not assigned","property not assigned","default section"))ids.add("ST-71");
        if(has(q,"story assignment","story data","kat atama","kat bilgisi","unknown story","undefined story","floor assignment","kat aks eleman","kat-aks-eleman"))ids.add("ST-72");
        if(has(q,"duplicate element","duplicate joint","duplicate member","mukerrer eleman","mükerrer eleman","conflicting assignment","çelişkili atama","celiskili atama","kimlik cakismasi","kimlik çakışması"))ids.add("ST-73");
        if(has(q,"zero length","zero-length","very short element","very short member","coincident joint","coincident node","sifir uzunluk","sıfır uzunluk","degenerate element"))ids.add("ST-74");
        if(has(q,"support restraint","joint restraint","boundary condition","mesnet atama","mesnet tanimi","mesnet tanımı","support not assigned","missing restraint","unrestrained joint"))ids.add("ST-75");
        if(has(q,"diaphragm assignment","diaphragm constraint","constraint assignment","diyafram atama","diyafram tanimi","diyafram tanımı","diaphragm not assigned"))ids.add("ST-76");
        if(has(q,"load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama"))ids.add("ST-77");
        if(has(q,"self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier"))ids.add("ST-78");
        if(has(q,"unit system","model units","birim sistemi","birim ayari","birim ayarı","unit mismatch","birim uyumsuz"))ids.add("ST-79");
        if(has(q,"design code","code version","tasarim yonetmeligi","tasarım yönetmeliği","yonetmelik surumu","yönetmelik sürümü","tbdy","ts500"))ids.add("ST-80");
        if(has(q,"story elevation","floor elevation","kat kotu","kat kotlari","kat kotları","coordinate system","koordinat sistemi","elevation mismatch"))ids.add("ST-81");
        if(has(q,"analysis case","run status","analysis status","not run","case failed","analysis outdated","analiz durumu","analiz case","case durumu","analiz case durumu","calistirilmamis","çalıştırılmamış"))ids.add("ST-82");
        if(has(q,"duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","load case referans","load case referansi","load case referansı","yuk durumu referansi","yük durumu referansı"))ids.add("ST-83");
        if(has(q,"object property compatibility","property type mismatch","wrong property type","property tip","property uyumluluk","eleman property uyumsuz","nesne property uyumsuz","frame property area","area property frame"))ids.add("ST-84");
        if(has(q,"shell thickness","area thickness","slab thickness property","wall thickness property","thickness not assigned","default thickness","kabuk kalinligi","kabuk kalınlığı","alan kalinligi","alan kalınlığı"))ids.add("ST-85");
        if(has(q,"pier label","spandrel label","pier assignment","spandrel assignment","unassigned pier","unassigned spandrel","perde pier","spandrel atama"))ids.add("ST-86");
        if(has(q,"design procedure","design status","not designed","design excluded","no design","check only","design group","tasarim durumu","tasarım durumu","tasarim disi","tasarım dışı"))ids.add("ST-87");
        if(has(q,"auto mesh","automatic mesh","area mesh assignment","mesh assignment","mesh not assigned","unmeshed area","otomatik mesh","mesh atama"))ids.add("ST-88");
        if(has(q,"load pattern type","load pattern category","pattern type mismatch","dead pattern","live pattern","wind pattern","snow pattern","quake pattern","yuk pattern tipi","yük pattern tipi"))ids.add("ST-89");
        if(has(q,"response spectrum function","spectrum function","spectrum direction","ux uy uz","rs direction","spektrum fonksiyonu","spektrum yonu","spektrum yönü"))ids.add("ST-90");
        if(has(q,"modal case method","eigen method","ritz vector","ritz vectors","modal source","modal setup","modal yontem","modal yöntem","ritz vektoru","ritz vektörü"))ids.add("ST-91");
        if(has(q,"damping ratio","modal damping","response spectrum damping","rayleigh damping","sonum orani","sönüm oranı","modal sonum","modal sönüm"))ids.add("ST-92");
        if(has(q,"modal combination","cqc","srss","directional combination","direction combination","modal birlestirme","modal birleştirme","yon birlestirme","yön birleştirme"))ids.add("ST-93");
        if(has(q,"time history function","time history case","ground motion function","record function","zaman tanim alani","zaman tanım alanı","zaman gecmisi","zaman geçmişi"))ids.add("ST-94");
        if(has(q,"time step","time increment","number of output steps","output time step","duration","zaman adimi","zaman adımı","analiz suresi","analiz süresi"))ids.add("ST-95");
        if(has(q,"nonlinear hinge assignment","plastic hinge assignment","hinge property","hinge assignment","nonlinear hinge","plastik mafsal atama","plastik mafsal"))ids.add("ST-96");
        if(has(q,"nonlinear case parameters","nonlinear solution control","maximum iterations","iteration tolerance","event stepping","nonlinear control","dogrusal olmayan analiz ayari","doğrusal olmayan analiz ayarı"))ids.add("ST-97");
        if(has(q,"staged construction","construction stage","stage definition","stage sequence","staged nonlinear","asama tanimi","aşama tanımı","yapim asamasi","yapım aşaması"))ids.add("ST-98");
        return ids;
    }

    private static String focusTitle(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(has(q,"zimbala","punching"))return "ZIMBALAMA";
        if(has(q,"load pattern type","load pattern category","pattern type mismatch","dead pattern","live pattern","wind pattern","snow pattern","quake pattern","yuk pattern tipi","yük pattern tipi"))return "LOAD PATTERN TÜR / KATEGORİ";
        if(has(q,"response spectrum function","spectrum function","spectrum direction","ux uy uz","rs direction","spektrum fonksiyonu","spektrum yonu","spektrum yönü"))return "RESPONSE SPECTRUM FONKSİYON / YÖN";
        if(has(q,"modal case method","eigen method","ritz vector","ritz vectors","modal source","modal setup","modal yontem","modal yöntem","ritz vektoru","ritz vektörü"))return "MODAL CASE YÖNTEMİ";
        if(has(q,"damping ratio","modal damping","response spectrum damping","rayleigh damping","sonum orani","sönüm oranı","modal sonum","modal sönüm"))return "SÖNÜM TANIMLARI";
        if(has(q,"modal combination","cqc","srss","directional combination","direction combination","modal birlestirme","modal birleştirme","yon birlestirme","yön birleştirme"))return "DİNAMİK KOMBİNASYON YÖNTEMİ";
        if(has(q,"time history function","time history case","ground motion function","record function","zaman tanim alani","zaman tanım alanı","zaman gecmisi","zaman geçmişi"))return "ZAMAN TANIM ALANI / KAYIT FONKSİYONU";
        if(has(q,"time step","time increment","number of output steps","output time step","duration","zaman adimi","zaman adımı","analiz suresi","analiz süresi"))return "TIME-STEP / ANALİZ SÜRESİ";
        if(has(q,"nonlinear hinge assignment","plastic hinge assignment","hinge property","hinge assignment","nonlinear hinge","plastik mafsal atama","plastik mafsal"))return "DOĞRUSAL OLMAYAN MAFSAL ATAMASI";
        if(has(q,"nonlinear case parameters","nonlinear solution control","maximum iterations","iteration tolerance","event stepping","nonlinear control","dogrusal olmayan analiz ayari","doğrusal olmayan analiz ayarı"))return "DOĞRUSAL OLMAYAN ANALİZ KONTROLLERİ";
        if(has(q,"staged construction","construction stage","stage definition","stage sequence","staged nonlinear","asama tanimi","aşama tanımı","yapim asamasi","yapım aşaması"))return "YAPIM AŞAMASI / STAGED CONSTRUCTION";
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
        if(has(q,"zemin basinci","temel basinci","soil pressure","contact pressure","oturma","settlement"))return "TEMEL BASINCI / OTURMA";
        if(has(q,"kazik kapasitesi","kazik tasima","pile capacity","pile load","kazik yuk"))return "KAZIK YÜK / KAPASİTE";
        if(has(q,"uplift","yuzme","hidrostatik","hydrostatic","buoyancy"))return "YÜZME / UPLIFT";
        if(has(q,"p-delta","p delta","ikinci mertebe","second order","second-order"))return "P-DELTA / İKİNCİ MERTEBE";
        if(has(q,"taban kesme","base shear","spektrum olcekle","spectrum scale","scaling"))return "TABAN KESMESİ / SPEKTRUM ÖLÇEKLEME";
        if(has(q,"zemin yapi etkilesimi","soil structure interaction","soil-structure interaction","yay katsayisi","spring stiffness","area spring"))return "ZEMİN–YAPI ETKİLEŞİMİ";
        if(has(q,"kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass"))return "KÜTLE KAYNAĞI / DEPREM KÜTLESİ";
        if(has(q,"tesadufi eksantrisite","accidental eccentricity","additional eccentricity"))return "TESADÜFİ EKSANTRİSİTE";
        if(has(q,"kutle merkezi","rijitlik merkezi","center of mass","centre of mass","center of rigidity","centre of rigidity","eksantrisite","eccentricity"))return "KÜTLE–RİJİTLİK MERKEZİ / EKSANTRİSİTE";
        if(has(q,"collector","drag strut","diyafram kiri","diaphragm chord","chord force"))return "DİYAFRAM KUVVET AKTARIMI";
        if(has(q,"dusey deprem","dikey deprem","vertical earthquake","vertical seismic"))return "DÜŞEY DEPREM ETKİSİ";
        if(has(q,"yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon"))return "YÜK KOMBİNASYONLARI";
        if(has(q,"deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","rsx","rsy"))return "DEPREM YÜK DURUMLARI";
        if(has(q,"r/d/i","r d i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor"))return "R / D / I TAŞIYICI SİSTEM KABULLERİ";
        if(has(q,"etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier","property modifier","rijitlik carpani","rijitlik çarpanı"))return "ETKİN / ÇATLAMIŞ KESİT RİJİTLİKLERİ";
        if(has(q,"mafsal","hinge","release","end release","moment release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset"))return "MAFSAL / RELEASE / RİJİT BÖLGE KABULLERİ";
        if(has(q,"pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı"))return "KOLON / PERDE PMM ETKİLEŞİMİ";
        if(has(q,"kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment","flexural ratio","moment ratio"))return "KİRİŞ KESME / EĞİLME SONUÇLARI";
        if(has(q,"perde kesme","wall shear","shear wall shear","kesme kapasitesi","shear capacity"))return "PERDE KESME SONUÇLARI";
        if(has(q,"kapasite asimi","kapasite aşımı","capacity failure","failed capacity","yetersiz eleman","uygunsuz eleman"))return "KAPASİTE AŞIMI / BAŞARISIZ ELEMANLAR";
        if(has(q,"kapasite orani","kapasite oranı","capacity ratio","utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio","kritik eleman"))return "ELEMAN KULLANIM / KAPASİTE ORANLARI";
        if(has(q,"sehim","deflection","servisabilite","serviceability"))return "SEHİM / SERVİS VERİLEBİLİRLİK";
        if(has(q,"catlak genisligi","çatlak genişliği","crack width"))return "ÇATLAK GENİŞLİĞİ";
        if(has(q,"titresim","titreşim","vibration","comfort frequency","floor frequency"))return "TİTREŞİM / KONFOR";
        if(has(q,"sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection"))return "UZUN SÜRELİ ETKİLER";
        if(has(q,"servis siniri","servis sınırı","serviceability limit","sehim asimi","sehim aşımı","catlak asimi","çatlak aşımı","titresim limiti","titreşim limiti"))return "SERVİS SINIRI AŞIMLARI";
        if(has(q,"singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness"))return "MODEL TEKİLLİĞİ / KARARSIZLIK";
        if(has(q,"unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint"))return "MODEL BAĞLANTI BÜTÜNLÜĞÜ";
        if(has(q,"mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi"))return "SONLU ELEMAN / MESH KALİTESİ";
        if(has(q,"nonconvergence","non-convergence","did not converge","not converged","convergence failed","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti"))return "ANALİZ YAKINSAMASI";
        if(has(q,"local axis","local axes","yerel eksen","orientation assignment","section orientation","major axis","minor axis","eleman yonu","eleman yönü"))return "YEREL EKSEN / ORYANTASYON";
        if(has(q,"revizyon","revision","rev no","revizyon no","model revision","model rev"))return "MODEL / PROJE REVİZYON EŞLEŞMESİ";
        if(has(q,"material assignment","material property","malzeme atama","malzeme ataması","undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material"))return "MALZEME ATAMA BÜTÜNLÜĞÜ";
        if(has(q,"section assignment","section property","kesit atama","kesit ataması","undefined section","section not assigned","property not assigned","default section"))return "KESİT / PROPERTY ATAMA BÜTÜNLÜĞÜ";
        if(has(q,"story assignment","story data","kat atama","kat bilgisi","unknown story","undefined story","floor assignment","kat aks eleman","kat-aks-eleman"))return "KAT / AKS / ELEMAN VERİ BÜTÜNLÜĞÜ";
        if(has(q,"duplicate element","duplicate joint","duplicate member","mukerrer eleman","mükerrer eleman","conflicting assignment","çelişkili atama","celiskili atama","kimlik cakismasi","kimlik çakışması"))return "MÜKERRER / ÇELİŞKİLİ ELEMAN KİMLİĞİ";
        if(has(q,"zero length","zero-length","very short element","very short member","coincident joint","coincident node","sifir uzunluk","sıfır uzunluk","degenerate element"))return "SIFIR / BOZUK GEOMETRİLİ ELEMANLAR";
        if(has(q,"support restraint","joint restraint","boundary condition","mesnet atama","mesnet tanimi","mesnet tanımı","support not assigned","missing restraint","unrestrained joint"))return "MESNET / SINIR ŞARTI ATAMALARI";
        if(has(q,"diaphragm assignment","diaphragm constraint","constraint assignment","diyafram atama","diyafram tanimi","diyafram tanımı","diaphragm not assigned"))return "DİYAFRAM / CONSTRAINT ATAMALARI";
        if(has(q,"load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama"))return "YÜK ATAMA BÜTÜNLÜĞÜ";
        if(has(q,"self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier"))return "ÖZ AĞIRLIK / GRAVITY TANIMLARI";
        if(has(q,"unit system","model units","birim sistemi","birim ayari","birim ayarı","unit mismatch","birim uyumsuz"))return "BİRİM SİSTEMİ";
        if(has(q,"design code","code version","tasarim yonetmeligi","tasarım yönetmeliği","yonetmelik surumu","yönetmelik sürümü","tbdy","ts500"))return "YÖNETMELİK / TASARIM KODU";
        if(has(q,"story elevation","floor elevation","kat kotu","kat kotlari","kat kotları","coordinate system","koordinat sistemi","elevation mismatch"))return "KAT KOTU / KOORDİNAT";
        if(has(q,"analysis case","run status","analysis status","not run","case failed","analysis outdated","analiz durumu","analiz case","case durumu","analiz case durumu","calistirilmamis","çalıştırılmamış"))return "ANALİZ CASE DURUMU";
        if(has(q,"duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","load case referans","load case referansi","load case referansı","yuk durumu referansi","yük durumu referansı"))return "LOAD CASE / KOMBİNASYON REFERANSI";
        if(has(q,"object property compatibility","property type mismatch","wrong property type","property tip","property uyumluluk","eleman property uyumsuz","nesne property uyumsuz","frame property area","area property frame"))return "ELEMAN / PROPERTY TİP UYUMLULUĞU";
        if(has(q,"shell thickness","area thickness","slab thickness property","wall thickness property","thickness not assigned","default thickness","kabuk kalinligi","kabuk kalınlığı","alan kalinligi","alan kalınlığı"))return "SHELL / ALAN KALINLIK ATAMASI";
        if(has(q,"pier label","spandrel label","pier assignment","spandrel assignment","unassigned pier","unassigned spandrel","perde pier","spandrel atama"))return "PIER / SPANDREL ATAMALARI";
        if(has(q,"design procedure","design status","not designed","design excluded","no design","check only","design group","tasarim durumu","tasarım durumu","tasarim disi","tasarım dışı"))return "TASARIM PROSEDÜRÜ / KAPSAMI";
        if(has(q,"auto mesh","automatic mesh","area mesh assignment","mesh assignment","mesh not assigned","unmeshed area","otomatik mesh","mesh atama"))return "AUTO-MESH ATAMA KAPSAMI";
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
        List<String>basicLoads=loadCues(calc,
            "hareketli yuk","sabit yuk","kar yuku","ruzgar yuku","duvar yuku",
            "live load","dead load","snow load","wind load");
        if(basicLoads.isEmpty())
            out.add(new Finding("ST-21",Status.DOGRULANAMADI,"Yük kabulleri otomatik doğrulanamadı",
                "Hesap raporundan sabit/hareketli/kar/rüzgâr yüklerine ilişkin güvenilir metin ipucu çıkarılamadı.",
                "Temel yük kabullerini içeren hesap bölümünü rapora dahil edin; yük kombinasyonları ST-49 kapsamında ayrıca kontrol edilir.",Collections.emptyList()));
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
            if(has(q,"saglamiyor","saglanmiyor","uygunsuz","yetersiz","basarisiz","asiyor","aşıyor","asildi","aşıldı","exceed","exceeded","fail","failed","not satisfied","not comply","does not comply"))negative=true;
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

    private static void foundationBehaviorChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"zemin basinci","temel basinci","soil pressure","contact pressure","oturma","settlement");
        boolean shallow=calc.foundationTypes.contains("RADYE")||calc.foundationTypes.contains("TEKİL")||
            calc.foundationTypes.contains("SÜREKLİ")||calc.foundationTypes.contains("BİRLEŞİK");
        if(!shallow&&cues.isEmpty())return;
        if(cues.isEmpty())
            out.add(new Finding("ST-38",Status.DOGRULANAMADI,"Temel zemin basıncı / oturma sonucu okunamadı",
                "Sığ temel sistemi tanındı ancak hesap raporundan açık zemin temas basıncı veya oturma sonucu ayrıştırılamadı.",
                "Temel temas basıncı, taşıma gücü karşılaştırması ve varsa oturma sonuçlarını içeren hesap/geoteknik rapor bölümünü doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-38",reportedStatus(cues),"Temel zemin basıncı / oturma rapor kontrolü",
                "Hesap raporundan temel davranışına ilişkin veri okundu: "+cueSummary(cues,5)+".",
                "MusaCAD rapor satırını aktarır; zemin basıncı veya oturma hesabını bağımsız olarak yeniden üretmez.",Collections.emptyList()));
    }

    private static void pileCapacityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kazik kapasitesi","kazik tasima","pile capacity","pile load","kazik yuk");
        boolean pile=calc.foundationTypes.contains("KAZIK");
        if(!pile&&cues.isEmpty())return;
        if(cues.isEmpty())
            out.add(new Finding("ST-39",Status.DOGRULANAMADI,"Kazık yük / kapasite sonucu okunamadı",
                "Kazıklı temel sistemi tanındı ancak hesap raporundan açık kazık yükü veya taşıma kapasitesi sonucu ayrıştırılamadı.",
                "Kazık başına düşey/yatay yükler ile geoteknik/structural kazık kapasitesi tablosunu aynı kazık tipi için doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-39",reportedStatus(cues),"Kazık yük / kapasite rapor kontrolü",
                "Hesap raporundan kazık yük/kapasite verisi okundu: "+cueSummary(cues,5)+".",
                "Değerlerin aynı kazık çapı, boyu ve geoteknik tasarım kabulüne ait olduğunu doğrulayın; MusaCAD kazık kapasitesi hesaplamaz.",Collections.emptyList()));
    }

    private static void upliftChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"uplift","yuzme","hidrostatik","hydrostatic","buoyancy");
        boolean groundwater=calc.designParameters.containsKey("Yeraltı Suyu");
        if(!groundwater&&cues.isEmpty())return;
        if(cues.isEmpty())
            out.add(new Finding("ST-40",Status.DOGRULANAMADI,"Yeraltı suyu / yüzme kontrolü okunamadı",
                "Hesap verisinde yeraltı suyu seviyesi "+calc.designParameters.get("Yeraltı Suyu")+" olarak okundu ancak açık uplift/yüzme/hidrostatik kontrol sonucu bulunamadı.",
                "Temel alt kotu, yeraltı suyu seviyesi, yapı öz-ağırlığı ve hidrostatik kaldırma kontrolünü ilgili hesap bölümünde doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-40",reportedStatus(cues),"Yüzme / uplift rapor kontrolü",
                "Hesap raporundan hidrostatik/yüzme kontrol verisi okundu: "+cueSummary(cues,5)+".",
                "Rapor sonucunu güncel yeraltı suyu seviyesi ve temel kotuyla çapraz kontrol edin; otomatik tarama kaldırma güvenliği hesabı yapmaz.",Collections.emptyList()));
    }

    private static void secondOrderChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"p-delta","p delta","ikinci mertebe","second order","second-order");
        boolean seismicContext=calc.designParameters.containsKey("SDS")||calc.designParameters.containsKey("DTS")||calc.designParameters.containsKey("BYS");
        if(cues.isEmpty()&&!seismicContext)return;
        if(cues.isEmpty())
            out.add(new Finding("ST-41",Status.DOGRULANAMADI,"P-Delta / ikinci mertebe kontrolü okunamadı",
                "Deprem tasarım parametreleri mevcut ancak yüklenen rapordan açık P-Delta/ikinci mertebe analiz satırı ayrıştırılamadı.",
                "İkinci mertebe etkilerinin hesap modelinde dikkate alınıp alınmadığını analiz ayarları ve rapor özeti üzerinden doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-41",reportedStatus(cues),"P-Delta / ikinci mertebe rapor kontrolü",
                "Hesap raporundan ikinci mertebe verisi okundu: "+cueSummary(cues,4)+".",
                "MusaCAD rapor beyanını aktarır; stabilite katsayısını veya ikinci mertebe etkilerini yeniden hesaplamaz.",Collections.emptyList()));
    }

    private static void baseShearChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"taban kesme","base shear","spektrum olcekle","spectrum scale","scaling");
        boolean seismicContext=calc.designParameters.containsKey("SDS")||calc.designParameters.containsKey("SD1");
        if(cues.isEmpty()&&!seismicContext)return;
        if(cues.isEmpty())
            out.add(new Finding("ST-42",Status.DOGRULANAMADI,"Taban kesmesi / spektrum ölçekleme sonucu okunamadı",
                "Deprem spektrum parametreleri mevcut ancak rapordan açık taban kesmesi veya spektrum ölçekleme bilgisi ayrıştırılamadı.",
                "Eşdeğer deprem yükü ile modal spektrum sonuçlarının ölçekleme/karşılaştırma özetini hesap raporundan doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-42",reportedStatus(cues),"Taban kesmesi / spektrum ölçekleme rapor kontrolü",
                "Hesap raporundan taban kesmesi/ölçekleme verisi okundu: "+cueSummary(cues,5)+".",
                "X/Y doğrultularını, yükleme kombinasyonunu ve son ölçek katsayılarını analiz modeliyle doğrulayın.",Collections.emptyList()));
    }

    private static void soilStructureInteractionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"zemin yapi etkilesimi","soil structure interaction","soil-structure interaction","yay katsayisi","spring stiffness","area spring");
        boolean subgrade=calc.designParameters.containsKey("Yatak Katsayısı");
        if(!subgrade&&cues.isEmpty())return;
        if(cues.isEmpty())
            out.add(new Finding("ST-43",Status.DOGRULANAMADI,"Zemin–yapı etkileşimi / yay modeli okunamadı",
                "Yatak katsayısı "+calc.designParameters.get("Yatak Katsayısı")+" olarak okundu ancak hesap raporunda açık zemin yayları/zemin–yapı etkileşimi tanımı ayrıştırılamadı.",
                "Temel modelindeki yay/alan yayı tanımlarının kullanılan yatak katsayısı ve birim sistemiyle uyumunu doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-43",reportedStatus(cues),"Zemin–yapı etkileşimi / yay modeli",
                "Hesap raporundan zemin-yapı etkileşimi verisi okundu: "+cueSummary(cues,5)+".",
                "Yay katsayısı, birimler, sıkıştırma-only kabulü ve temel elemanlarına atanma kapsamını modelde doğrulayın.",Collections.emptyList()));
    }

    private static void massSourceChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass");
        if(cues.isEmpty())
            out.add(new Finding("ST-44",Status.DOGRULANAMADI,"Kütle kaynağı / deprem kütlesi okunamadı",
                "Yüklenen hesap raporundan analiz kütle kaynağı veya deprem kütlesi tanımı ayrıştırılamadı.",
                "Sabit yük, hareketli yük katılım oranı ve varsa ilave kütlelerin kütle kaynağında nasıl tanımlandığını rapor/model çıktısıyla doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-44",reportedStatus(cues),"Kütle kaynağı / deprem kütlesi rapor kontrolü",
                "Hesap raporundan kütle kaynağına ilişkin veri okundu: "+cueSummary(cues,5)+".",
                "Kütle kaynağının yük kombinasyonları ve proje kullanım sınıfıyla uyumunu model üzerinde ayrıca doğrulayın.",Collections.emptyList()));
    }

    private static void centerEccentricityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kutle merkezi","rijitlik merkezi","center of mass","centre of mass","center of rigidity","centre of rigidity","eksantrisite","eccentricity");
        cues.removeIf(cue->has(MusaAiDrawingIndex.normalize(cue),"tesadufi eksantrisite","accidental eccentricity","additional eccentricity"));
        if(cues.isEmpty())
            out.add(new Finding("ST-45",Status.DOGRULANAMADI,"Kütle–rijitlik merkezi / eksantrisite verisi okunamadı",
                "Hesap raporundan kat bazlı kütle merkezi, rijitlik merkezi veya doğal eksantrisite verisi ayrıştırılamadı.",
                "Kat bazlı CM/CR koordinatları veya eksantrisite tablosunu rapora dahil edin; MusaCAD merkezleri çizimden tahmin etmez.",Collections.emptyList()));
        else
            out.add(new Finding("ST-45",reportedStatus(cues),"Kütle–rijitlik merkezi / eksantrisite rapor kontrolü",
                "Hesap raporundan merkez/eksantrisite verisi okundu: "+cueSummary(cues,5)+".",
                "Kritik katlarda plan geometrisi ve rijitlik dağılımıyla birlikte kontrol edin.",Collections.emptyList()));
    }

    private static void accidentalEccentricityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"tesadufi eksantrisite","accidental eccentricity","additional eccentricity");
        if(cues.isEmpty())
            out.add(new Finding("ST-46",Status.DOGRULANAMADI,"Tesadüfi eksantrisite tanımı okunamadı",
                "Yüklenen hesap raporundan tesadüfi/ilave eksantrisite uygulamasına ilişkin açık satır ayrıştırılamadı.",
                "Deprem yük durumlarında tesadüfi eksantrisite tanımını ve yön kombinasyonlarını analiz modelinden doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-46",reportedStatus(cues),"Tesadüfi eksantrisite rapor kontrolü",
                "Hesap raporundan tesadüfi eksantrisite verisi okundu: "+cueSummary(cues,4)+".",
                "Tanımın ilgili tüm deprem yük durumlarına ve yönlere uygulandığını model üzerinde doğrulayın.",Collections.emptyList()));
    }

    private static void diaphragmForcePathChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();ArrayList<String>drawing=new ArrayList<>();
        for(Ref r:refs)if(has(r.q,"collector","drag strut","diyafram kirisi","diyafram kiri","diaphragm chord","chord force","toplayici eleman")){
            drawing.add(location(r));addId(ids,r);
        }
        List<String>cues=reportCues(calc,"collector","drag strut","diyafram kirisi","diyafram kiri","diaphragm chord","chord force","toplayici eleman");
        if(drawing.isEmpty()&&cues.isEmpty())return;
        Status status=!cues.isEmpty()?reportedStatus(cues):(calc==null?Status.INCELEME_GEREKLI:Status.DOGRULANAMADI);
        String detail=!cues.isEmpty()
            ?"Hesap raporundan diyafram kuvvet aktarım elemanlarına ilişkin veri okundu: "+cueSummary(cues,4)+"."
            :"Çizimde collector/chord/toplayıcı eleman ifadesi bulundu: "+join(drawing,6)+".";
        out.add(new Finding("ST-47",status,"Diyafram collector / chord kuvvet aktarımı",detail,
            "Büyük açıklık ve düzensizlik bölgelerinde diyafram kuvvet yolunu, collector/chord elemanlarını ve bağlantı detaylarını hesap modeliyle eşleştirin.",ids));
    }

    private static void verticalSeismicChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"dusey deprem","dikey deprem","vertical earthquake","vertical seismic","vertical response spectrum");
        if(cues.isEmpty())
            out.add(new Finding("ST-48",Status.DOGRULANAMADI,"Düşey deprem etkisi tanımı okunamadı",
                "Yüklenen hesap raporundan düşey deprem etkisi veya düşey spektrum tanımı ayrıştırılamadı.",
                "Düşey deprem etkisinin gerekli olduğu eleman/koşullar için kullanılan yük durumu ve kombinasyonları model çıktısından doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-48",reportedStatus(cues),"Düşey deprem etkisi rapor kontrolü",
                "Hesap raporundan düşey deprem etkisine ilişkin veri okundu: "+cueSummary(cues,4)+".",
                "İlgili yük durumunun hangi eleman ve kombinasyonlarda kullanıldığını ayrıca doğrulayın.",Collections.emptyList()));
    }

    private static void loadCombinationChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=new ArrayList<>();
        cues.addAll(loadCues(calc,"yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon","combination"));
        if(cues.isEmpty())
            out.add(new Finding("ST-49",Status.DOGRULANAMADI,"Yük kombinasyonları okunamadı",
                "Yüklenen hesap raporundan tasarım/servis yük kombinasyonlarına ilişkin açık satır ayrıştırılamadı.",
                "Sabit, hareketli, deprem, rüzgâr ve diğer yüklerin hangi tasarım/servis kombinasyonlarında kullanıldığını rapor/model çıktısıyla doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-49",reportedStatus(cues),"Yük kombinasyonları rapor kontrolü",
                "Hesap raporundan yük kombinasyonu verisi okundu: "+cueSummary(cues,6)+".",
                "Kombinasyon listesinin son model revizyonuna ait olduğunu ve beklenen yük durumlarını kapsadığını doğrulayın; MusaCAD eksik kombinasyonu yönetmelikten türetmez.",Collections.emptyList()));
    }

    private static void seismicLoadCaseChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","rsx","rsy","response spectrum x","response spectrum y");
        if(cues.isEmpty())
            out.add(new Finding("ST-50",Status.DOGRULANAMADI,"Deprem yük durumları okunamadı",
                "Hesap raporundan X/Y deprem yük durumları, spektrum yük durumları veya eşdeğer deprem yük durumları açık biçimde ayrıştırılamadı.",
                "Deprem yük durumlarının yön, eksantrisite ve spektrum tanımlarını analiz modelinden doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-50",reportedStatus(cues),"Deprem yük durumları rapor kontrolü",
                "Hesap raporundan deprem yük durumlarına ilişkin veri okundu: "+cueSummary(cues,6)+".",
                "Yük durumlarının ilgili spektrum, yön ve eksantrisite tanımlarıyla eşleştiğini model üzerinde doğrulayın.",Collections.emptyList()));
    }

    private static void rdiSystemChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        ArrayList<String>values=new ArrayList<>();
        for(String key:Arrays.asList("R","D","I"))if(calc.designParameters.containsKey(key))
            values.add(key+"="+calc.designParameters.get(key));
        List<String>cues=reportCues(calc,"r/d/i","r d i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor","dayanim fazlaligi","dayanım fazlalığı","bina onem","bina önem");
        if(values.size()<3&&cues.isEmpty())
            out.add(new Finding("ST-51",Status.DOGRULANAMADI,"R / D / I taşıyıcı sistem kabulleri eksik",
                "Hesap raporundan R, D ve I parametrelerinin tamamı açık biçimde ayrıştırılamadı.",
                "Taşıyıcı sistem türü, davranış katsayısı R, dayanım fazlalığı D ve bina önem katsayısı I değerlerini proje bilgi sayfası/model çıktısıyla doğrulayın.",Collections.emptyList()));
        else{
            String detail="Okunan parametreler: "+(values.isEmpty()?"—":join(values,6))+".";
            if(!cues.isEmpty())detail+=" İlgili rapor satırları: "+cueSummary(cues,4)+".";
            out.add(new Finding("ST-51",Status.BILGI,"R / D / I taşıyıcı sistem kabulleri",detail,
                "MusaCAD yalnız okunan değerleri raporlar; taşıyıcı sistem sınıfına göre uygun R/D/I seçimini bağımsız olarak hükme bağlamaz.",Collections.emptyList()));
        }
    }

    private static void effectiveStiffnessChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier","property modifier","rijitlik carpani","rijitlik çarpanı");
        if(cues.isEmpty())
            out.add(new Finding("ST-52",Status.DOGRULANAMADI,"Etkin / çatlamış kesit rijitlikleri okunamadı",
                "Yüklenen hesap raporundan kolon, kiriş, perde veya döşeme için etkin/çatlamış kesit rijitlik kabulleri ayrıştırılamadı.",
                "Eleman bazlı stiffness/property modifier değerlerini ve hangi analizlerde uygulandığını model çıktısından doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-52",reportedStatus(cues),"Etkin / çatlamış kesit rijitlikleri",
                "Hesap raporundan rijitlik kabullerine ilişkin veri okundu: "+cueSummary(cues,6)+".",
                "Modifier değerlerinin eleman türü ve analiz amacıyla uyumunu model üzerinde doğrulayın; MusaCAD katsayıları eksikse tahmin etmez.",Collections.emptyList()));
    }

    private static void releaseRigidZoneChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"mafsal","hinge","release","end release","moment release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset");
        if(cues.isEmpty())
            out.add(new Finding("ST-53",Status.DOGRULANAMADI,"Mafsal / release / rijit bölge kabulleri okunamadı",
                "Hesap raporundan eleman uç release/mafsal, rijit bölge veya end-offset tanımları ayrıştırılamadı.",
                "Özellikle çelik/kompozit elemanlar, bağ kirişleri, konsollar ve kiriş-kolon birleşimlerinde uç kabullerini analiz modelinden doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-53",reportedStatus(cues),"Mafsal / release / rijit bölge kabulleri",
                "Hesap raporundan eleman uç/rijit bölge verisi okundu: "+cueSummary(cues,6)+".",
                "Tanımların gerçek birleşim davranışı ve detay paftalarıyla eşleştiğini doğrulayın.",Collections.emptyList()));
    }

    private static void columnWallInteractionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı");
        if(cues.isEmpty())
            out.add(new Finding("ST-54",Status.DOGRULANAMADI,"Kolon / perde PMM etkileşim sonuçları okunamadı",
                "Yüklenen hesap raporundan kolon/perde eksenel kuvvet–iki eksenli eğilme etkileşimine ilişkin açık sonuç satırı ayrıştırılamadı.",
                "Eleman bazlı PMM/interaction ratio sonuçlarını içeren tasarım özetini rapora dahil edin; MusaCAD eksik etkileşim hesabını kendisi üretmez.",Collections.emptyList()));
        else
            out.add(new Finding("ST-54",reportedStatus(cues),"Kolon / perde PMM etkileşim raporu",
                "Hesap raporundan PMM/interaction verisi okundu: "+cueSummary(cues,8)+".",
                "Değerleri ilgili kat/eleman etiketi ve yük kombinasyonu ile eşleştirin; MusaCAD yalnız raporlanan sonucu aktarır.",Collections.emptyList()));
    }

    private static void beamCapacityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment","flexural ratio","moment ratio");
        if(cues.isEmpty())
            out.add(new Finding("ST-55",Status.DOGRULANAMADI,"Kiriş kesme / eğilme kullanım sonuçları okunamadı",
                "Hesap raporundan kiriş kesme veya eğilme kullanım/kapasite sonuçları ayrıştırılamadı.",
                "Kiriş tasarım özetindeki kesme, moment ve varsa kullanım oranı sonuçlarını rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-55",reportedStatus(cues),"Kiriş kesme / eğilme rapor kontrolü",
                "Hesap raporundan kiriş kapasite verisi okundu: "+cueSummary(cues,8)+".",
                "Kritik satırları kiriş etiketi, kat ve belirleyici kombinasyonla eşleştirin; MusaCAD donatı kapasitesini yeniden hesaplamaz.",Collections.emptyList()));
    }

    private static void wallShearCapacityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"perde kesme","wall shear","shear wall shear","kesme kapasitesi","shear capacity");
        if(cues.isEmpty())
            out.add(new Finding("ST-56",Status.DOGRULANAMADI,"Perde kesme kapasite sonucu okunamadı",
                "Yüklenen hesap raporundan perde kesme talep/kapasite sonuçları ayrıştırılamadı.",
                "Perde tasarım özetindeki kesme talebi, kapasite ve belirleyici kombinasyon satırlarını rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-56",reportedStatus(cues),"Perde kesme rapor kontrolü",
                "Hesap raporundan perde kesme verisi okundu: "+cueSummary(cues,8)+".",
                "Sonucu perde etiketi, kat, kesit ve donatı detayıyla çapraz kontrol edin.",Collections.emptyList()));
    }

    private static void explicitCapacityFailureChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        ArrayList<String>bad=new ArrayList<>();
        for(String cue:calc.seismicCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,"yetersiz","uygunsuz","failed","fail","does not comply","not satisfied","capacity failure","kapasite asimi","kapasite aşımı")&&!bad.contains(cue))
                bad.add(cue);
        }
        if(bad.isEmpty())return;
        out.add(new Finding("ST-57",Status.UYUMSUZLUK,"Raporda açık kapasite/uygunluk başarısızlığı",
            "Hesap raporunda açık başarısız/yetersiz/uygunsuz ifadeler bulundu: "+cueSummary(bad,10)+".",
            "İlgili elemanları ve belirleyici kombinasyonları doğrudan hesap modelinde inceleyin; otomatik özet tek başına mühendislik kararı değildir.",Collections.emptyList()));
    }

    private static void utilizationRankingChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"kapasite orani","kapasite oranı","capacity ratio","utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio","interaction ratio");
        if(cues.isEmpty())return;
        ArrayList<ReportedRatio>ratios=new ArrayList<>();
        Pattern p=Pattern.compile("(?i)(?:RATIO|ORAN[İI]?|UTILI[ZS]ATION|D\\s*/?\\s*C)\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        for(String cue:cues){
            Matcher m=p.matcher(cue);
            if(m.find()){
                try{ratios.add(new ReportedRatio(Double.parseDouble(m.group(1).replace(',','.')),cue));}catch(Exception ignored){}
            }
        }
        if(ratios.isEmpty()){
            out.add(new Finding("ST-58",Status.BILGI,"Eleman kullanım / kapasite oranı satırları bulundu",
                "Rapor içinde kullanım/kapasite oranı ifadeleri bulundu ancak güvenilir sayısal oranlar ayrıştırılamadı: "+cueSummary(cues,6)+".",
                "Kritik eleman sıralaması için eleman etiketi ve sayısal oran içeren tablo/model çıktısını kullanın.",Collections.emptyList()));
            return;
        }
        ratios.sort((a,b)->Double.compare(b.value,a.value));
        ArrayList<String>top=new ArrayList<>();
        for(int i=0;i<Math.min(8,ratios.size());i++){
            ReportedRatio r=ratios.get(i);
            String cue=r.cue.trim().replaceAll("\\s+"," ");
            if(cue.length()>120)cue=cue.substring(0,120)+"…";
            top.add(String.format(Locale.ROOT,"%.3f • %s",r.value,cue));
        }
        out.add(new Finding("ST-58",Status.BILGI,"Raporlanan en yüksek kullanım / kapasite oranları",
            "Rapor içindeki ayrıştırılabilir oranlar büyükten küçüğe sıralandı: "+join(top,8)+".",
            "Bu sıralama yalnız raporda yazan oranları gösterir; eşik uygunluğu veya eleman güvenliği hakkında bağımsız hüküm vermez.",Collections.emptyList()));
    }

    private static final class ReportedRatio{
        final double value;final String cue;
        ReportedRatio(double value,String cue){this.value=value;this.cue=cue;}
    }

    private static List<String> serviceReportCues(MusaAiStructuralCalc.Model calc,String...terms){
        ArrayList<String>out=new ArrayList<>();
        if(calc==null)return out;
        for(String cue:calc.loadCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,terms)&&!out.contains(cue))out.add(cue);
        }
        for(String cue:calc.seismicCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,terms)&&!out.contains(cue))out.add(cue);
        }
        return out;
    }

    private static void deflectionServiceabilityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"sehim","deflection","servisabilite","serviceability");
        if(cues.isEmpty())
            out.add(new Finding("ST-59",Status.DOGRULANAMADI,"Sehim / servis verilebilirlik sonucu okunamadı",
                "Yüklenen hesap raporundan sehim veya servis verilebilirlik sonucuna ilişkin açık satır ayrıştırılamadı.",
                "Kiriş/döşeme servis kombinasyonu, hesaplanan sehim ve kullanılan sınır bilgisini rapora dahil edin; MusaCAD eksik sehim hesabı üretmez.",Collections.emptyList()));
        else
            out.add(new Finding("ST-59",reportedStatus(cues),"Sehim / servis verilebilirlik rapor kontrolü",
                "Hesap raporundan servis sonucu okundu: "+cueSummary(cues,8)+".",
                "Sonucu ilgili eleman/kat, açıklık, servis kombinasyonu ve raporda verilen sınırla birlikte doğrulayın.",Collections.emptyList()));
    }

    private static void crackWidthChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"catlak genisligi","çatlak genişliği","crack width");
        if(cues.isEmpty())
            out.add(new Finding("ST-60",Status.DOGRULANAMADI,"Çatlak genişliği sonucu okunamadı",
                "Yüklenen hesap raporundan çatlak genişliği / crack-width kontrol sonucu ayrıştırılamadı.",
                "Servis durumu çatlak genişliği sonuçlarını eleman etiketi ve kullanılan sınır ile birlikte rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-60",reportedStatus(cues),"Çatlak genişliği rapor kontrolü",
                "Hesap raporundan çatlak genişliği verisi okundu: "+cueSummary(cues,8)+".",
                "Raporlanan değeri eleman, donatı düzeni, çevresel/servis kabulü ve raporda belirtilen sınırla çapraz kontrol edin.",Collections.emptyList()));
    }

    private static void vibrationServiceabilityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"titresim","titreşim","vibration","comfort frequency","floor frequency");
        if(cues.isEmpty())
            out.add(new Finding("ST-61",Status.DOGRULANAMADI,"Titreşim / konfor sonucu okunamadı",
                "Yüklenen hesap raporundan döşeme titreşimi, konfor veya ilgili frekans sonucu ayrıştırılamadı.",
                "Titreşim açısından kontrol edilen alanlarda ilgili frekans/ivme/konfor çıktısını rapora dahil edin; MusaCAD eksik dinamik hesabı üretmez.",Collections.emptyList()));
        else
            out.add(new Finding("ST-61",reportedStatus(cues),"Titreşim / konfor rapor kontrolü",
                "Hesap raporundan titreşim/konfor verisi okundu: "+cueSummary(cues,8)+".",
                "Sonucu kullanım amacı, açıklık/döşeme bölgesi ve raporda verilen kabul sınırıyla birlikte değerlendirin.",Collections.emptyList()));
    }

    private static void longTermEffectChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection");
        if(cues.isEmpty())
            out.add(new Finding("ST-62",Status.DOGRULANAMADI,"Uzun süreli sehim / sünme-rötre sonucu okunamadı",
                "Hesap raporundan uzun süreli deformasyon, sünme veya rötre etkisine ilişkin açık sonuç ayrıştırılamadı.",
                "Uzun açıklıklı veya hassas elemanlarda kullanılan sünme/rötre kabulleri ile uzun süreli sehim sonuçlarını rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-62",reportedStatus(cues),"Uzun süreli etkiler rapor kontrolü",
                "Hesap raporundan uzun süreli etki verisi okundu: "+cueSummary(cues,8)+".",
                "Kabullerin malzeme yaşı, yükleme süresi ve proje servis koşullarıyla uyumunu hesap modelinden doğrulayın.",Collections.emptyList()));
    }

    private static void explicitServiceabilityFailureChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        ArrayList<String>bad=new ArrayList<>();
        for(String cue:calc.loadCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            boolean service=has(q,"sehim","deflection","servisabilite","serviceability","catlak genisligi","çatlak genişliği","crack width","titresim","titreşim","vibration","creep","shrinkage","sunme","sünme","rotre","rötre");
            boolean failed=has(q,"asiyor","aşıyor","asildi","aşıldı","exceed","exceeded","uygunsuz","yetersiz","basarisiz","başarısız","fail","failed","not satisfied","does not comply");
            if(service&&failed&&!bad.contains(cue))bad.add(cue);
        }
        if(bad.isEmpty())return;
        out.add(new Finding("ST-63",Status.UYUMSUZLUK,"Raporda açık servis sınırı aşımı",
            "Servis verilebilirlik ile ilgili açık sınır-aşımı/başarısızlık ifadeleri bulundu: "+cueSummary(bad,10)+".",
            "İlgili eleman, servis kombinasyonu ve kullanılan sınırı doğrudan hesap modelinde inceleyin; otomatik özet tek başına uygunluk kararı değildir.",Collections.emptyList()));
    }

    private static Status analysisWarningStatus(Collection<String>cues){
        boolean positive=false,negative=false;
        for(String cue:cues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,"no singularity","no instability","no mechanism","no unconnected","no disconnected","no orphan",
                "converged successfully","convergence achieved","yakinsama saglandi","yakınsama sağlandı",
                "mesh quality suitable","mesh uygun","local axis suitable","yerel eksen uygun","orientation verified")){
                positive=true;
                continue;
            }
            if(has(q,"singular","singularity","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness",
                "unconnected","disconnected","orphan","floating node","floating joint",
                "mesh warning","distorted element","nonconvergence","non-convergence","did not converge","not converged","convergence failed",
                "yakinsamadi","yakınsamadı","yakinsama hatasi","yakınsama hatası","iteration limit","iterasyon limiti",
                "uygunsuz","yetersiz","hata","error","failed","fail"))negative=true;
            if(has(q,"uygun","suitable","verified","checked","basarili","başarılı","converged"))positive=true;
        }
        if(negative)return Status.UYUMSUZLUK;
        if(positive)return Status.BILGI;
        return Status.INCELEME_GEREKLI;
    }

    private static void modelInstabilityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness");
        if(cues.isEmpty())
            out.add(new Finding("ST-64",Status.DOGRULANAMADI,"Model tekilliği / kararsızlık uyarısı doğrulanamadı",
                "Yüklenen hesap raporundan tekil rijitlik matrisi, kararsızlık, mekanizma veya sıfır/negatif rijitlik uyarısına ilişkin açık satır ayrıştırılamadı.",
                "Analiz programının warning/error özetini ve kararsız düğüm/serbestlik kayıtlarını rapora dahil edin; MusaCAD görünmeyen model hatasını varsaymaz.",Collections.emptyList()));
        else
            out.add(new Finding("ST-64",analysisWarningStatus(cues),"Model tekilliği / kararsızlık rapor kontrolü",
                "Hesap raporundan tekillik/kararsızlık verisi okundu: "+cueSummary(cues,10)+".",
                "Uyarıyı ilgili düğüm, eleman, serbestlik derecesi ve analiz yük durumuyla doğrudan model üzerinde inceleyin.",Collections.emptyList()));
    }

    private static void disconnectedModelChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint","joint not connected","element not connected");
        if(cues.isEmpty())
            out.add(new Finding("ST-65",Status.DOGRULANAMADI,"Bağlantısız düğüm / eleman uyarısı doğrulanamadı",
                "Hesap raporundan bağlantısız/orphan düğüm veya eleman uyarısına ilişkin açık satır ayrıştırılamadı.",
                "Model connectivity/check-model çıktısını rapora dahil edin ve birleşmesi gereken düğüm/elemanların tolerans içinde gerçekten bağlı olduğunu doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-65",analysisWarningStatus(cues),"Model bağlantı bütünlüğü rapor kontrolü",
                "Hesap raporundan bağlantı bütünlüğü verisi okundu: "+cueSummary(cues,10)+".",
                "İlgili düğüm/eleman etiketlerini modelde gösterip bağlantı, merge toleransı ve uç serbestliklerini kontrol edin.",Collections.emptyList()));
    }

    private static void meshQualityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi");
        if(cues.isEmpty())
            out.add(new Finding("ST-66",Status.DOGRULANAMADI,"Sonlu eleman / mesh kalite bilgisi okunamadı",
                "Yüklenen hesap raporundan kabuk/sonlu eleman mesh boyutu veya kalite uyarısına ilişkin açık satır ayrıştırılamadı.",
                "Özellikle perde, döşeme, radye ve lokal gerilme bölgeleri için mesh boyutu/kalite özetini model çıktısına dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-66",analysisWarningStatus(cues),"Sonlu eleman / mesh kalite rapor kontrolü",
                "Hesap raporundan mesh verisi okundu: "+cueSummary(cues,10)+".",
                "Aşırı bozuk eleman, büyük aspect-ratio ve kritik bölgelerde yetersiz ağ inceliğini model üzerinde doğrulayın.",Collections.emptyList()));
    }

    private static void convergenceChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"nonconvergence","non-convergence","did not converge","not converged","convergence failed","converged","convergence achieved","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti");
        if(cues.isEmpty())
            out.add(new Finding("ST-67",Status.DOGRULANAMADI,"Analiz yakınsama sonucu okunamadı",
                "Yüklenen hesap raporundan doğrusal olmayan analiz yakınsaması veya iterasyon sınırına ilişkin açık sonuç ayrıştırılamadı.",
                "Doğrusal olmayan analiz kullanılıyorsa yakınsama/iterasyon özetini ve başarısız yük adımlarını rapora dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-67",analysisWarningStatus(cues),"Analiz yakınsama rapor kontrolü",
                "Hesap raporundan yakınsama verisi okundu: "+cueSummary(cues,10)+".",
                "Yakınsamayan yük adımı/analiz durumu varsa model, malzeme doğrusal olmayanlığı, mafsal ve çözüm ayarlarını doğrudan inceleyin.",Collections.emptyList()));
    }

    private static void localAxisChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"local axis","local axes","yerel eksen","orientation assignment","section orientation","major axis","minor axis","eleman yonu","eleman yönü");
        if(cues.isEmpty())
            out.add(new Finding("ST-68",Status.DOGRULANAMADI,"Yerel eksen / eleman oryantasyonu okunamadı",
                "Yüklenen hesap raporundan çubuk/kabuk yerel eksen veya kesit oryantasyonuna ilişkin açık kayıt ayrıştırılamadı.",
                "Kiriş, kolon, perde ve kabuk elemanlarda yerel eksen/orientasyon görünümünü model kontrol çıktısına dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-68",analysisWarningStatus(cues),"Yerel eksen / oryantasyon rapor kontrolü",
                "Hesap raporundan yerel eksen/orientasyon verisi okundu: "+cueSummary(cues,10)+".",
                "Özellikle asimetrik kesitler, kabuk yönleri ve yüklerin yerel eksene bağlı olduğu elemanlarda yönleri modelde görsel olarak doğrulayın.",Collections.emptyList()));
    }

    private static String revisionToken(String raw){
        if(raw==null||raw.trim().isEmpty())return "";
        String upper=raw.toUpperCase(new Locale("tr","TR")).replace('İ','I');
        String[]parts=upper.split("[^A-Z0-9]+");
        boolean seen=false;
        for(String part:parts){
            if(part.isEmpty())continue;
            if(part.equals("REV")||part.equals("REVISION")||part.equals("REVIZYON")){
                seen=true;
                continue;
            }
            if(!seen)continue;
            if(part.equals("NO")||part.equals("NUMBER")||part.equals("NUMARASI"))continue;
            if(part.matches("(?=.*[0-9])[A-Z0-9]{1,16}"))return part;
        }
        return "";
    }
    private static void revisionConsistencyChecks(List<Ref>refs,MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        LinkedHashSet<String>drawing=new LinkedHashSet<>(),report=new LinkedHashSet<>();
        LinkedHashSet<Integer>ids=new LinkedHashSet<>();
        for(Ref r:refs){
            String token=revisionToken(r.raw);
            if(!token.isEmpty()){drawing.add(token);addId(ids,r);}
        }
        for(String cue:reportCues(calc,"revizyon","revision","rev no","revizyon no","model revision","model rev")){
            String token=revisionToken(cue);
            if(!token.isEmpty())report.add(token);
        }
        if(drawing.isEmpty()||report.isEmpty()){
            out.add(new Finding("ST-69",Status.DOGRULANAMADI,"Model / proje revizyon eşleşmesi doğrulanamadı",
                "Pafta/model revizyon bilgisinin iki tarafta da karşılaştırılabilir biçimde okunması mümkün olmadı. Proje: "+join(drawing,6)+" • Hesap: "+join(report,6)+".",
                "Statik pafta anteti ile hesap/model çıktısında aynı revizyon kodu veya tarih bilgisini görünür biçimde bulundurun.",ids));
            return;
        }
        LinkedHashSet<String>common=new LinkedHashSet<>(drawing);common.retainAll(report);
        if(common.isEmpty())
            out.add(new Finding("ST-69",Status.UYUMSUZLUK,"Model / proje revizyonu eşleşmiyor",
                "Paftadan okunan revizyon: "+join(drawing,6)+" • hesap/model raporundan okunan revizyon: "+join(report,6)+".",
                "Hesap modelinin son onaylı statik proje revizyonuna ait olduğunu doğrulayın; eski model ile yeni pafta birlikte kullanılmamalıdır.",ids));
        else
            out.add(new Finding("ST-69",Status.BILGI,"Model / proje revizyonu eşleşiyor",
                "Karşılaştırılabilen ortak revizyon kodu bulundu: "+join(common,6)+".",
                "Bu eşleşme yalnız görünür revizyon bilgisini doğrular; dosyanın tüm içeriğinin aynı revizyona ait olduğunu tek başına kanıtlamaz.",ids));
    }

    private static void materialAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"material assignment","material property","malzeme atama","malzeme ataması","undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material");
        ArrayList<String>bad=new ArrayList<>();
        for(String cue:cues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,"undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material","uygunsuz","failed","error"))bad.add(cue);
        }
        if(!bad.isEmpty()){
            out.add(new Finding("ST-70",Status.UYUMSUZLUK,"Eksik / varsayılan malzeme ataması raporlandı",
                "Hesap/model çıktısında malzeme atama uyarıları bulundu: "+cueSummary(bad,10)+".",
                "İlgili elemanların beton/çelik malzeme property atamalarını modelde eleman bazında düzeltin.",Collections.emptyList()));
            return;
        }
        if(calc.concreteGrades.isEmpty()&&calc.rebarGrades.isEmpty()){
            out.add(new Finding("ST-70",Status.DOGRULANAMADI,"Malzeme sınıfı / ataması okunamadı",
                "Hesap raporundan güvenilir beton veya donatı çeliği sınıfı ayrıştırılamadı.",
                "Malzeme tanım ve eleman-atama özetini hesap/model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        int concreteMissing=0,rebarMissing=0;
        for(MusaAiStructuralCalc.Element e:calc.elements){
            if(e==null)continue;
            if(e.concreteGrade.isEmpty())concreteMissing++;
            if(e.rebarGrade.isEmpty())rebarMissing++;
        }
        if(!cues.isEmpty()&&reportedStatus(cues)==Status.BILGI)
            out.add(new Finding("ST-70",Status.BILGI,"Malzeme atama özeti raporda mevcut",
                "Okunan malzemeler: beton "+join(calc.concreteGrades,8)+" • donatı "+join(calc.rebarGrades,8)+". Raporlanmış atama bilgisi: "+cueSummary(cues,6)+".",
                "Kritik elemanlarda malzeme property adının ve tasarım sınıfının modelde doğru elemana atandığını örnekleme ile doğrulayın.",Collections.emptyList()));
        else
            out.add(new Finding("ST-70",Status.INCELEME_GEREKLI,"Malzeme sınıfları var; eleman bazlı atama kısmi",
                "Genel malzeme sınıfları okundu: beton "+join(calc.concreteGrades,8)+" • donatı "+join(calc.rebarGrades,8)+". Ayrıştırılan elemanlarda açık beton sınıfı olmayan "+concreteMissing+", donatı sınıfı olmayan "+rebarMissing+" kayıt var.",
                "Global malzeme tanımı ile eleman property atamasını karıştırmayın; farklı malzeme kullanılan elemanları özellikle kontrol edin.",Collections.emptyList()));
    }

    private static void sectionAssignmentIntegrityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"section assignment","section property","kesit atama","kesit ataması","undefined section","section not assigned","property not assigned","default section");
        ArrayList<String>bad=new ArrayList<>();
        for(String cue:cues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,"undefined section","section not assigned","property not assigned","default section","uygunsuz","failed","error"))bad.add(cue);
        }
        LinkedHashMap<String,LinkedHashSet<String>>byIdentity=new LinkedHashMap<>();
        int missing=0;
        for(MusaAiStructuralCalc.Element e:calc.elements){
            if(e==null)continue;
            if(e.section.isEmpty())missing++;
            if(e.tag.isEmpty()||e.section.isEmpty())continue;
            String key=e.floor+"|"+e.tag;
            byIdentity.computeIfAbsent(key,k->new LinkedHashSet<>()).add(e.section);
        }
        ArrayList<String>conflicts=new ArrayList<>();
        for(Map.Entry<String,LinkedHashSet<String>>e:byIdentity.entrySet())
            if(e.getValue().size()>1)conflicts.add(e.getKey()+"="+join(e.getValue(),5));
        if(!bad.isEmpty()||!conflicts.isEmpty())
            out.add(new Finding("ST-71",Status.UYUMSUZLUK,"Kesit / property atama çelişkisi",
                "Açık atama uyarıları: "+cueSummary(bad,6)+" • aynı kimlikte farklı kesitler: "+join(conflicts,8)+".",
                "Modelde section property atamalarını eleman etiketi ve kat bazında gözden geçirin; varsayılan/boş property bırakmayın.",Collections.emptyList()));
        else if(calc.sections.isEmpty())
            out.add(new Finding("ST-71",Status.DOGRULANAMADI,"Kesit / property ataması okunamadı",
                "Hesap raporundan güvenilir kesit listesi veya section property ataması ayrıştırılamadı.",
                "Eleman-kesit eşleştirme tablosunu model raporuna dahil edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-71",missing>0?Status.INCELEME_GEREKLI:Status.BILGI,"Kesit / property atama bütünlüğü",
                "Raporlanan kesitler: "+join(calc.sections,12)+". Ayrıştırılan elemanlarda açık kesit bilgisi olmayan kayıt sayısı: "+missing+".",
                "Kesitsiz görünen kayıtların rapor ayrıştırma eksikliği mi yoksa gerçek model property eksikliği mi olduğunu model üzerinde doğrulayın.",Collections.emptyList()));
    }

    private static void storyElementMetadataChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        if(calc.elements.isEmpty()){
            out.add(new Finding("ST-72",Status.DOGRULANAMADI,"Kat / aks / eleman metadata bütünlüğü doğrulanamadı",
                "Kat-aks-eleman bazında ayrıştırılabilir kayıt bulunmadığı için model veri bütünlüğü incelenemedi.",
                "Eleman listesi veya model tablo çıktısında kat, eleman etiketi, aks ve kesit alanlarını birlikte sağlayın.",Collections.emptyList()));
            return;
        }
        int noFloor=0,noAxis=0,unknownType=0;
        ArrayList<String>samples=new ArrayList<>();
        for(MusaAiStructuralCalc.Element e:calc.elements){
            if(e==null)continue;
            boolean frame=e.memberType==MusaAiStructuralCalc.MemberType.BEAM||e.memberType==MusaAiStructuralCalc.MemberType.COLUMN||e.memberType==MusaAiStructuralCalc.MemberType.WALL;
            if(e.floor.isEmpty()){noFloor++;if(samples.size()<8)samples.add(e.tag+" KAT?");}
            if(frame&&e.axis.isEmpty()){noAxis++;if(samples.size()<8)samples.add(e.tag+" AKS?");}
            if(e.memberType==MusaAiStructuralCalc.MemberType.UNKNOWN){unknownType++;if(samples.size()<8)samples.add(e.tag+" TİP?");}
        }
        Status status=(noFloor>0||noAxis>0||unknownType>0)?Status.INCELEME_GEREKLI:Status.BILGI;
        out.add(new Finding("ST-72",status,"Kat / aks / eleman metadata bütünlüğü",
            "Ayrıştırılan "+calc.elements.size()+" kayıtta kat bilgisi eksik: "+noFloor+" • çubuk/perde aksı eksik: "+noAxis+" • eleman tipi belirsiz: "+unknownType+". Örnekler: "+join(samples,8)+".",
            "Eksik metadata gerçek model eksikliğiyse düzeltin; yalnız rapor formatından kaynaklanıyorsa kat/aks/eleman tablosunu dışa aktarın.",Collections.emptyList()));
    }

    private static void duplicateElementIdentityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        if(calc.elements.isEmpty()){
            out.add(new Finding("ST-73",Status.DOGRULANAMADI,"Mükerrer / çelişkili eleman kimliği doğrulanamadı",
                "Eleman tablosu ayrıştırılamadığı için aynı kat ve etikete ait çakışan kayıtlar denetlenemedi.",
                "Kat + eleman etiketi + aks + kesit içeren model eleman listesini rapora dahil edin.",Collections.emptyList()));
            return;
        }
        LinkedHashMap<String,ArrayList<MusaAiStructuralCalc.Element>>groups=new LinkedHashMap<>();
        for(MusaAiStructuralCalc.Element e:calc.elements){
            if(e==null||e.tag.isEmpty())continue;
            String key=(e.floor.isEmpty()?"?":e.floor)+"|"+e.tag;
            groups.computeIfAbsent(key,k->new ArrayList<>()).add(e);
        }
        ArrayList<String>duplicates=new ArrayList<>(),conflicts=new ArrayList<>();
        for(Map.Entry<String,ArrayList<MusaAiStructuralCalc.Element>>g:groups.entrySet()){
            if(g.getValue().size()<2)continue;
            duplicates.add(g.getKey()+" x"+g.getValue().size());
            LinkedHashSet<String>axes=new LinkedHashSet<>(),sections=new LinkedHashSet<>(),types=new LinkedHashSet<>();
            for(MusaAiStructuralCalc.Element e:g.getValue()){
                if(!e.axis.isEmpty())axes.add(e.axis);
                if(!e.section.isEmpty())sections.add(e.section);
                if(e.memberType!=MusaAiStructuralCalc.MemberType.UNKNOWN)types.add(e.memberType.name());
            }
            if(axes.size()>1||sections.size()>1||types.size()>1)
                conflicts.add(g.getKey()+" [aks="+join(axes,4)+", kesit="+join(sections,4)+", tip="+join(types,4)+"]");
        }
        if(!conflicts.isEmpty())
            out.add(new Finding("ST-73",Status.UYUMSUZLUK,"Aynı eleman kimliğinde çelişkili kayıtlar",
                "Kat+etiket bazında çelişkili kayıtlar bulundu: "+join(conflicts,10)+".",
                "Mükerrer model elemanlarını, kopyalanmış property atamalarını ve kat/etiket çakışmalarını modelde temizleyin.",Collections.emptyList()));
        else if(!duplicates.isEmpty())
            out.add(new Finding("ST-73",Status.INCELEME_GEREKLI,"Mükerrer eleman kimliği kayıtları",
                "Ayrıştırılan raporda aynı kat+etiket birden çok satırda geçiyor: "+join(duplicates,10)+". Açık aks/kesit/tip çelişkisi saptanmadı.",
                "Bunların raporun farklı sonuç satırları mı yoksa gerçek mükerrer model elemanları mı olduğunu kontrol edin.",Collections.emptyList()));
        else
            out.add(new Finding("ST-73",Status.BILGI,"Ayrıştırılan kayıtlarda eleman kimliği çakışması görülmedi",
                "Kat+eleman etiketi bazında açık mükerrer/çelişkili kayıt saptanmadı.",
                "Bu kontrol yalnız rapordan ayrıştırılan elemanlarla sınırlıdır; modelin kendi duplicate/check-model aracını da çalıştırın.",Collections.emptyList()));
    }

    private static Status assignmentCueStatus(Collection<String>cues,String...negativeTerms){
        boolean positive=false,negative=false;
        for(String cue:cues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,negativeTerms))negative=true;
            if(has(q,"uygun","sagliyor","sağlıyor","assigned","defined","verified","checked","basarili","başarılı","no warning","no error"))positive=true;
        }
        if(negative)return Status.UYUMSUZLUK;
        if(positive)return Status.BILGI;
        return Status.INCELEME_GEREKLI;
    }

    private static void degenerateGeometryChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"zero length","zero-length","very short element","very short member","coincident joint","coincident node","sifir uzunluk","sıfır uzunluk","degenerate element");
        if(cues.isEmpty()){
            out.add(new Finding("ST-74",Status.DOGRULANAMADI,"Sıfır / bozuk geometrili eleman kontrolü okunamadı",
                "Hesap/model raporundan sıfır uzunluklu, aşırı kısa veya çakışık düğüm/eleman uyarısına ilişkin açık çıktı ayrıştırılamadı.",
                "Model check/geometry check çıktısında zero-length, coincident joint ve degenerate element kontrollerini görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"zero length element detected","zero-length element detected","very short element warning","very short member warning","coincident joint detected","coincident node detected","degenerate element detected","sifir uzunluklu eleman bulundu","sıfır uzunluklu eleman bulundu","error","failed");
        out.add(new Finding("ST-74",s,"Sıfır / bozuk geometrili eleman rapor kontrolü",
            "Model raporundan geometri kontrol verisi okundu: "+cueSummary(cues,10)+".",
            "Uyarı verilen düğüm ve elemanları yakınlaştırarak birleşim, eleman boyu ve üst üste binen geometri açısından inceleyin.",Collections.emptyList()));
    }

    private static void supportBoundaryAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"support restraint","joint restraint","boundary condition","mesnet atama","mesnet tanimi","mesnet tanımı","support not assigned","missing restraint","unrestrained joint");
        if(cues.isEmpty()){
            out.add(new Finding("ST-75",Status.DOGRULANAMADI,"Mesnet / sınır şartı atamaları okunamadı",
                "Hesap raporundan mesnet, joint restraint veya boundary-condition atamasına ilişkin açık kayıt ayrıştırılamadı.",
                "Temel/mesnet düğümleri için sabit, mafsallı, yaylı veya serbestlik tanımlarını model kontrol çıktısına dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"support not assigned","missing restraint","unrestrained joint","undefined support","boundary condition missing","mesnet atanmamis","mesnet atanmamış","error","failed");
        out.add(new Finding("ST-75",s,"Mesnet / sınır şartı atama kontrolü",
            "Model raporundan mesnet/restraint verisi okundu: "+cueSummary(cues,10)+".",
            "Atamaların gerçek taşıyıcı sistem ve zemin/temel modelleme kabulüyle eşleştiğini düğüm bazında doğrulayın.",Collections.emptyList()));
    }

    private static void diaphragmConstraintAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=reportCues(calc,"diaphragm assignment","diaphragm constraint","constraint assignment","diyafram atama","diyafram tanimi","diyafram tanımı","diaphragm not assigned");
        if(cues.isEmpty()){
            out.add(new Finding("ST-76",Status.DOGRULANAMADI,"Diyafram / constraint ataması okunamadı",
                "Hesap raporundan kat diyaframı veya constraint atamasına ilişkin açık model kaydı ayrıştırılamadı.",
                "Rijit/yarı rijit diyafram kabulü kullanılan katlarda assignment özetini model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"diaphragm not assigned","constraint not assigned","missing diaphragm","undefined diaphragm","diyafram atanmamis","diyafram atanmamış","error","failed");
        out.add(new Finding("ST-76",s,"Diyafram / constraint atama kontrolü",
            "Model raporundan diyafram/constraint verisi okundu: "+cueSummary(cues,10)+".",
            "Diyafram adının doğru kat düğümlerine atandığını ve rijit/yarı rijit kabulün hesap modeliyle uyumlu olduğunu doğrulayın.",Collections.emptyList()));
    }

    private static void loadAssignmentIntegrityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama");
        if(cues.isEmpty()){
            out.add(new Finding("ST-77",Status.DOGRULANAMADI,"Yük atama bütünlüğü okunamadı",
                "Hesap raporundan frame/area/shell yük ataması veya atanmamış yük uyarısına ilişkin açık kayıt ayrıştırılamadı.",
                "Sabit, hareketli, duvar, kaplama, kar ve diğer tasarım yüklerinin eleman/alan assignment özetini rapora dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"load not assigned","unassigned load","missing load","load assignment missing","yuk atanmamis","yük atanmamış","error","failed");
        out.add(new Finding("ST-77",s,"Yük atama bütünlüğü rapor kontrolü",
            "Model raporundan yük atama verisi okundu: "+cueSummary(cues,10)+".",
            "Yüklerin doğru load pattern, yön, büyüklük ve eleman/alan grubuna atandığını modelde örnekleme ile kontrol edin.",Collections.emptyList()));
    }

    private static void selfWeightGravityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier");
        if(cues.isEmpty()){
            out.add(new Finding("ST-78",Status.DOGRULANAMADI,"Öz ağırlık / gravity tanımı okunamadı",
                "Hesap raporundan self-weight multiplier veya gravity yük durumuna ilişkin açık tanım ayrıştırılamadı.",
                "Öz ağırlığın hangi load pattern içinde ve hangi çarpanla üretildiğini model raporunda görünür hale getirin; iki kez eklenmediğini doğrulayın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"self weight multiplier = 0","self-weight multiplier = 0","self weight missing","gravity load missing","oz agirlik yok","öz ağırlık yok","duplicate self weight","self weight duplicated","error","failed");
        out.add(new Finding("ST-78",s,"Öz ağırlık / gravity tanım kontrolü",
            "Model raporundan öz ağırlık/gravity verisi okundu: "+cueSummary(cues,10)+".",
            "Self-weight çarpanını, dead-load pattern ilişkisini ve öz ağırlığın başka sabit yük içinde tekrar edilip edilmediğini kontrol edin.",Collections.emptyList()));
    }

    private static void unitSystemChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"unit system","model units","birim sistemi","birim ayari","birim ayarı","unit mismatch","birim uyumsuz");
        if(cues.isEmpty()){
            out.add(new Finding("ST-79",Status.DOGRULANAMADI,"Model birim sistemi okunamadı",
                "Hesap/model raporundan kullanılan kuvvet-uzunluk birim sistemine ilişkin açık kayıt ayrıştırılamadı.",
                "Model raporunda aktif birim sistemini görünür hale getirin ve giriş/çıktı tablolarının aynı birim sisteminde olduğunu doğrulayın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"unit mismatch","birim uyumsuz","wrong units","incorrect units","inconsistent units","error","failed");
        out.add(new Finding("ST-79",s,"Model birim sistemi kontrolü",
            "Model raporundan birim bilgisi okundu: "+cueSummary(cues,8)+".",
            "Kesit, yük, kot ve malzeme değerlerinin raporda belirtilen aktif birim sistemiyle tutarlı olduğunu örnekleme ile doğrulayın.",Collections.emptyList()));
    }

    private static void designCodeVersionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"design code","code version","tasarim yonetmeligi","tasarım yönetmeliği","yonetmelik surumu","yönetmelik sürümü","tbdy","ts500");
        if(cues.isEmpty()){
            out.add(new Finding("ST-80",Status.DOGRULANAMADI,"Yönetmelik / tasarım kodu okunamadı",
                "Hesap raporundan kullanılan tasarım standardı veya sürüm bilgisi açık biçimde ayrıştırılamadı.",
                "Betonarme ve deprem tasarımında kullanılan yönetmelik adını/sürümünü model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"code mismatch","version mismatch","wrong code","obsolete code","eski yonetmelik","eski yönetmelik","uyumsuz","error","failed");
        out.add(new Finding("ST-80",s,"Yönetmelik / tasarım kodu kontrolü",
            "Hesap raporundan tasarım kodu/sürüm bilgisi okundu: "+cueSummary(cues,8)+".",
            "Proje şartnamesi ve onaylı hesap esaslarıyla aynı yönetmelik/sürümün kullanıldığını doğrulayın.",Collections.emptyList()));
    }

    private static void storyElevationCoordinateChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"story elevation","floor elevation","kat kotu","kat kotlari","kat kotları","coordinate system","koordinat sistemi","elevation mismatch");
        if(cues.isEmpty()){
            out.add(new Finding("ST-81",Status.DOGRULANAMADI,"Kat kotu / koordinat sistemi bilgisi okunamadı",
                "Hesap/model raporundan kat kotları veya model koordinat sistemine ilişkin açık kontrol satırı ayrıştırılamadı.",
                "Kat kotu tablosu ile global koordinat sistemi bilgisini rapora dahil edin; mimari/statik kotlarla aynı referansı kullandığını doğrulayın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"elevation mismatch","story elevation mismatch","floor elevation mismatch","kat kotu uyumsuz","coordinate mismatch","koordinat uyumsuz","error","failed");
        out.add(new Finding("ST-81",s,"Kat kotu / koordinat sistemi kontrolü",
            "Model raporundan kat kotu/koordinat verisi okundu: "+cueSummary(cues,10)+".",
            "Özellikle temel, zemin katı, transfer katı ve çatı kotlarını mimari ve statik paftalarla karşılaştırın.",Collections.emptyList()));
    }

    private static void analysisCaseRunStatusChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"analysis case","run status","analysis status","not run","case failed","analysis outdated","analiz durumu","analiz case","case durumu","analiz case durumu","calistirilmamis","çalıştırılmamış");
        if(cues.isEmpty()){
            out.add(new Finding("ST-82",Status.DOGRULANAMADI,"Analiz case çalışma durumu okunamadı",
                "Hesap/model raporundan analiz durumlarının çalıştırılıp çalıştırılmadığına ilişkin açık kayıt ayrıştırılamadı.",
                "Tasarımda kullanılan load case ve kombinasyonların güncel model üzerinde başarıyla çalıştırıldığını gösteren analiz durum özetini rapora dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"not run","case failed","analysis failed","analysis outdated","results outdated","calistirilmamis","çalıştırılmamış","guncel degil","güncel değil","error","failed");
        out.add(new Finding("ST-82",s,"Analiz case çalışma durumu kontrolü",
            "Model raporundan analiz durumu okundu: "+cueSummary(cues,10)+".",
            "Başarısız, çalıştırılmamış veya model değişikliğinden sonra güncelliğini yitirmiş case bulunmadığını doğrulayın.",Collections.emptyList()));
    }

    private static void loadCaseReferenceIntegrityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","load case referans","load case referansi","load case referansı","yuk durumu referansi","yük durumu referansı");
        if(cues.isEmpty()){
            out.add(new Finding("ST-83",Status.DOGRULANAMADI,"Load case / kombinasyon referans bütünlüğü okunamadı",
                "Hesap/model raporundan mükerrer, tanımsız veya eksik load case referansına ilişkin açık model-check çıktısı ayrıştırılamadı.",
                "Kombinasyonların yalnız tanımlı load case/pattern adlarına referans verdiğini ve mükerrer isim bulunmadığını model kontrol raporunda doğrulayın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"duplicate load case","duplicate combination","undefined load case","missing load case","orphan load case","invalid reference","referans hatasi","referans hatası","error","failed");
        out.add(new Finding("ST-83",s,"Load case / kombinasyon referans bütünlüğü",
            "Model raporundan load case/kombinasyon referans verisi okundu: "+cueSummary(cues,10)+".",
            "Hatalı veya mükerrer adları düzelterek kombinasyonları yeniden üretin ve analiz sonuçlarını yeniden çalıştırın.",Collections.emptyList()));
    }

    private static void objectPropertyCompatibilityChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"object property compatibility","property type mismatch","wrong property type","property tip","property uyumluluk","eleman property uyumsuz","nesne property uyumsuz","frame property area","area property frame");
        if(cues.isEmpty()){
            out.add(new Finding("ST-84",Status.DOGRULANAMADI,"Eleman / property tipi uyumluluğu okunamadı",
                "Model raporundan frame/area/shell nesnesi ile atanan property tipinin uyumuna ilişkin açık kontrol satırı ayrıştırılamadı.",
                "Model-check çıktısında nesne tipi ile section/area property tipinin uyumlu olduğunu görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"property type mismatch","wrong property type","frame property assigned to area","area property assigned to frame","eleman property uyumsuz","nesne property uyumsuz","error","failed");
        out.add(new Finding("ST-84",s,"Eleman / property tipi uyumluluk kontrolü",
            "Model raporundan nesne-property uyumluluk verisi okundu: "+cueSummary(cues,10)+".",
            "Uyarı verilen nesnelerde frame/area/shell tipini ve atanan property sınıfını doğrudan modelden doğrulayın.",Collections.emptyList()));
    }

    private static void shellThicknessAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"shell thickness","area thickness","slab thickness property","wall thickness property","thickness not assigned","default thickness","kabuk kalinligi","kabuk kalınlığı","alan kalinligi","alan kalınlığı");
        if(cues.isEmpty()){
            out.add(new Finding("ST-85",Status.DOGRULANAMADI,"Shell / alan kalınlık atamaları okunamadı",
                "Döşeme, perde veya diğer area/shell elemanların kalınlık property atamalarına ilişkin açık model raporu bulunamadı.",
                "Döşeme ve perde area property adlarını, kalınlıklarını ve atama kapsamını model kontrol çıktısına dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"thickness not assigned","default thickness","undefined thickness","zero thickness","kabuk kalinligi yok","kabuk kalınlığı yok","error","failed");
        out.add(new Finding("ST-85",s,"Shell / alan kalınlık atama kontrolü",
            "Model raporundan kalınlık/property verisi okundu: "+cueSummary(cues,10)+".",
            "Döşeme/perde kalınlıklarının onaylı pafta ve kesitlerle eşleştiğini, varsayılan property kalmadığını doğrulayın.",Collections.emptyList()));
    }

    private static void pierSpandrelAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"pier label","spandrel label","pier assignment","spandrel assignment","unassigned pier","unassigned spandrel","perde pier","spandrel atama");
        if(cues.isEmpty()){
            out.add(new Finding("ST-86",Status.DOGRULANAMADI,"Pier / spandrel atamaları okunamadı",
                "Perde ve bağ kirişi tasarımında kullanılan pier/spandrel etiketlerinin atama durumuna ilişkin açık rapor satırı ayrıştırılamadı.",
                "Perde tasarımında kullanılan pier/spandrel etiketlerini kat ve eleman bazında model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"unassigned pier","unassigned spandrel","duplicate pier label","duplicate spandrel label","missing pier","missing spandrel","pier atanmamis","pier atanmamış","error","failed");
        out.add(new Finding("ST-86",s,"Pier / spandrel atama kontrolü",
            "Model raporundan pier/spandrel verisi okundu: "+cueSummary(cues,10)+".",
            "Etiketlerin perde sürekliliğini ve bağ kirişi bölgelerini doğru temsil ettiğini katlar boyunca kontrol edin.",Collections.emptyList()));
    }

    private static void designProcedureStatusChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"design procedure","design status","not designed","design excluded","no design","check only","design group","tasarim durumu","tasarım durumu","tasarim disi","tasarım dışı");
        if(cues.isEmpty()){
            out.add(new Finding("ST-87",Status.DOGRULANAMADI,"Tasarım prosedürü / kapsam durumu okunamadı",
                "Taşıyıcı elemanların tasarım/check kapsamına dahil edilip edilmediğini gösteren açık model raporu ayrıştırılamadı.",
                "Tasarım dışı bırakılan, yalnız check edilen veya farklı tasarım prosedürü kullanan elemanları raporda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"not designed","design excluded","no design","excluded from design","tasarim disi","tasarım dışı","design failed","error","failed");
        out.add(new Finding("ST-87",s,"Tasarım prosedürü / kapsam kontrolü",
            "Model raporundan tasarım durumu verisi okundu: "+cueSummary(cues,10)+".",
            "Taşıyıcı sistemde gerekli elemanların yanlışlıkla tasarım dışı kalmadığını ve doğru tasarım prosedürünün seçildiğini doğrulayın.",Collections.emptyList()));
    }

    private static void autoMeshAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"auto mesh","automatic mesh","area mesh assignment","mesh assignment","mesh not assigned","unmeshed area","otomatik mesh","mesh atama");
        if(cues.isEmpty()){
            out.add(new Finding("ST-88",Status.DOGRULANAMADI,"Auto-mesh atama kapsamı okunamadı",
                "Area/shell elemanların mesh atama kapsamına ilişkin açık model-check satırı ayrıştırılamadı.",
                "Döşeme, perde ve radye area elemanları için auto-mesh/mesh assignment kapsamını model raporunda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"mesh not assigned","unmeshed area","mesh assignment missing","auto mesh disabled","otomatik mesh yok","mesh atanmamis","mesh atanmamış","error","failed");
        out.add(new Finding("ST-88",s,"Auto-mesh atama kapsamı kontrolü",
            "Model raporundan mesh atama bilgisi okundu: "+cueSummary(cues,10)+".",
            "Mesh kalitesinden ayrı olarak, tüm gerekli area/shell elemanların gerçekten mesh kapsamına girdiğini doğrulayın.",Collections.emptyList()));
    }

    private static void loadPatternTypeChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"load pattern type","load pattern category","pattern type mismatch","dead pattern","live pattern","wind pattern","snow pattern","quake pattern","yuk pattern tipi","yük pattern tipi");
        if(cues.isEmpty()){
            out.add(new Finding("ST-89",Status.DOGRULANAMADI,"Load pattern tür / kategori bilgisi okunamadı",
                "Model raporundan DEAD/LIVE/WIND/SNOW/QUAKE gibi load-pattern tür atamalarına ilişkin açık kontrol satırı ayrıştırılamadı.",
                "Load pattern adları ile program içindeki pattern type/kategori atamalarını raporda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"pattern type mismatch","wrong load pattern type","incorrect pattern type","yuk pattern tipi uyumsuz","yük pattern tipi uyumsuz","error","failed");
        out.add(new Finding("ST-89",s,"Load pattern tür / kategori kontrolü",
            "Model raporundan load-pattern tür bilgisi okundu: "+cueSummary(cues,10)+".",
            "Sabit, hareketli, kar, rüzgâr ve deprem patternlerinin doğru kategoriyle tanımlandığını ve isim-tür çelişkisi olmadığını doğrulayın.",Collections.emptyList()));
    }

    private static void responseSpectrumAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"response spectrum function","spectrum function","spectrum direction","ux uy uz","rs direction","spektrum fonksiyonu","spektrum yonu","spektrum yönü");
        if(cues.isEmpty()){
            out.add(new Finding("ST-90",Status.DOGRULANAMADI,"Response spectrum fonksiyon / yön ataması okunamadı",
                "Response-spectrum case içinde kullanılan spektrum fonksiyonu ve UX/UY/UZ yön atamalarına ilişkin açık rapor satırı ayrıştırılamadı.",
                "RSX/RSY gibi case'lerde fonksiyon adı, yön ve ölçek bilgisini model raporunda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"spectrum function missing","wrong spectrum function","spectrum direction missing","direction mismatch","rs direction mismatch","spektrum yonu uyumsuz","spektrum yönü uyumsuz","error","failed");
        out.add(new Finding("ST-90",s,"Response spectrum fonksiyon / yön kontrolü",
            "Model raporundan response-spectrum atama verisi okundu: "+cueSummary(cues,10)+".",
            "Her response-spectrum case için doğru spektrum fonksiyonu ve global yönün seçildiğini doğrulayın.",Collections.emptyList()));
    }

    private static void modalCaseSetupChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"modal case method","eigen method","ritz vector","ritz vectors","modal source","modal setup","modal yontem","modal yöntem","ritz vektoru","ritz vektörü");
        if(cues.isEmpty()){
            out.add(new Finding("ST-91",Status.DOGRULANAMADI,"Modal case yöntem / kaynak ayarı okunamadı",
                "Modal analiz case'inin Eigen/Ritz yöntemi veya Ritz başlangıç vektörleri gibi kurulum bilgileri açık rapordan ayrıştırılamadı.",
                "Modal case yöntemini ve varsa Ritz vektör kaynaklarını model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"modal setup missing","modal source missing","ritz vector missing","invalid ritz","wrong modal method","modal yontem uyumsuz","modal yöntem uyumsuz","error","failed");
        out.add(new Finding("ST-91",s,"Modal case yöntem / kaynak kontrolü",
            "Model raporundan modal kurulum bilgisi okundu: "+cueSummary(cues,10)+".",
            "Seçilen Eigen/Ritz yönteminin proje analiz yaklaşımıyla uyumlu olduğunu ve gerekli modal kaynakların tanımlandığını doğrulayın.",Collections.emptyList()));
    }

    private static void dampingDefinitionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"damping ratio","modal damping","response spectrum damping","rayleigh damping","sonum orani","sönüm oranı","modal sonum","modal sönüm");
        if(cues.isEmpty()){
            out.add(new Finding("ST-92",Status.DOGRULANAMADI,"Sönüm oranı / damping tanımı okunamadı",
                "Modal veya response-spectrum analizlerinde kullanılan damping/sönüm tanımına ilişkin açık rapor satırı ayrıştırılamadı.",
                "Kullanılan damping oranını ve varsa frekansa bağlı/Rayleigh tanımını model raporuna dahil edin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"damping missing","damping ratio invalid","negative damping","damping mismatch","sonum orani uyumsuz","sönüm oranı uyumsuz","error","failed");
        out.add(new Finding("ST-92",s,"Sönüm tanımı kontrolü",
            "Model raporundan damping/sönüm bilgisi okundu: "+cueSummary(cues,10)+".",
            "Sönüm tanımının ilgili dinamik case'lere doğru atandığını ve proje kabulüyle uyumlu olduğunu doğrulayın.",Collections.emptyList()));
    }

    private static void dynamicCombinationMethodChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"modal combination","cqc","srss","directional combination","direction combination","modal birlestirme","modal birleştirme","yon birlestirme","yön birleştirme");
        if(cues.isEmpty()){
            out.add(new Finding("ST-93",Status.DOGRULANAMADI,"Modal / yönsel kombinasyon yöntemi okunamadı",
                "Response-spectrum sonuçlarının modal ve yönsel birleştirme yöntemlerine ilişkin açık ayar rapordan ayrıştırılamadı.",
                "CQC/SRSS gibi modal kombinasyon ile yönsel kombinasyon ayarlarını model raporunda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"combination method missing","invalid modal combination","invalid directional combination","modal combination mismatch","direction combination mismatch","birlesim yontemi uyumsuz","birleşim yöntemi uyumsuz","error","failed");
        out.add(new Finding("ST-93",s,"Modal / yönsel kombinasyon yöntemi kontrolü",
            "Model raporundan dinamik kombinasyon ayarı okundu: "+cueSummary(cues,10)+".",
            "Modal ve yönsel birleştirme yöntemlerinin proje analiz esaslarıyla uyumunu doğrulayın.",Collections.emptyList()));
    }

    private static void timeHistoryFunctionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"time history function","time history case","ground motion function","record function","zaman tanim alani","zaman tanım alanı","zaman gecmisi","zaman geçmişi");
        if(cues.isEmpty()){
            out.add(new Finding("ST-94",Status.DOGRULANAMADI,"Zaman tanım alanı kayıt/fonksiyon bilgisi okunamadı",
                "Model raporundan time-history case ile kullanılan yer hareketi/kayıt fonksiyonunun eşleşmesine ilişkin açık kayıt ayrıştırılamadı.",
                "Her time-history case için fonksiyon adı, yönü ve ölçek ilişkisini model raporunda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"function missing","record missing","invalid function","wrong record","time history function mismatch","zaman tanim fonksiyonu uyumsuz","zaman tanım fonksiyonu uyumsuz","error","failed");
        out.add(new Finding("ST-94",s,"Zaman tanım alanı kayıt/fonksiyon kontrolü",
            "Model raporundan time-history fonksiyon verisi okundu: "+cueSummary(cues,10)+".",
            "Kayıt fonksiyonu, yön ve ölçek tanımlarının ilgili analiz case'i ile doğru eşleştiğini doğrulayın.",Collections.emptyList()));
    }

    private static void timeHistoryStepChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"time step","time increment","number of output steps","output time step","duration","zaman adimi","zaman adımı","analiz suresi","analiz süresi");
        if(cues.isEmpty()){
            out.add(new Finding("ST-95",Status.DOGRULANAMADI,"Time-step / analiz süresi okunamadı",
                "Zaman tanım alanı analizinde kullanılan zaman adımı, çıktı adımı veya toplam analiz süresine ilişkin açık kayıt ayrıştırılamadı.",
                "Time-step, çıktı adımı sayısı ve toplam süreyi case özetinde görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"time step invalid","time step zero","duration mismatch","insufficient steps","zaman adimi hatali","zaman adımı hatalı","error","failed");
        out.add(new Finding("ST-95",s,"Time-step / analiz süresi kontrolü",
            "Model raporundan zaman adımı/süre verisi okundu: "+cueSummary(cues,10)+".",
            "Zaman adımı ve analiz süresinin kullanılan yer hareketi kaydı ve çözüm yöntemiyle uyumunu doğrulayın.",Collections.emptyList()));
    }

    private static void nonlinearHingeAssignmentChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"nonlinear hinge assignment","plastic hinge assignment","hinge property","hinge assignment","nonlinear hinge","plastik mafsal atama","plastik mafsal");
        if(cues.isEmpty()){
            out.add(new Finding("ST-96",Status.DOGRULANAMADI,"Doğrusal olmayan mafsal atamaları okunamadı",
                "Plastik/nonlinear mafsal property ve eleman atamalarına ilişkin açık model raporu ayrıştırılamadı.",
                "Nonlinear analiz kullanılan projelerde mafsal property adlarını, konumlarını ve atanan elemanları raporda görünür hale getirin.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"hinge not assigned","missing hinge","undefined hinge","invalid hinge","plastik mafsal atanmamis","plastik mafsal atanmamış","error","failed");
        out.add(new Finding("ST-96",s,"Doğrusal olmayan mafsal atama kontrolü",
            "Model raporundan nonlinear/plastik mafsal verisi okundu: "+cueSummary(cues,10)+".",
            "Mafsal property tipi, eleman uç konumu ve ilgili taşıyıcı elemanla eşleşmesini doğrulayın.",Collections.emptyList()));
    }

    private static void nonlinearCaseControlChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"nonlinear case parameters","nonlinear solution control","maximum iterations","iteration tolerance","event stepping","nonlinear control","dogrusal olmayan analiz ayari","doğrusal olmayan analiz ayarı");
        if(cues.isEmpty()){
            out.add(new Finding("ST-97",Status.DOGRULANAMADI,"Doğrusal olmayan çözüm kontrol ayarları okunamadı",
                "İterasyon sayısı, tolerans veya event-stepping gibi nonlinear çözüm kontrol parametreleri açık rapordan ayrıştırılamadı.",
                "Nonlinear case çözüm parametrelerini model raporuna dahil edin; yalnız yakınsama sonucuna değil çözüm ayarlarına da bakın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"invalid tolerance","iteration limit invalid","solution control missing","nonlinear control missing","event stepping invalid","error","failed");
        out.add(new Finding("ST-97",s,"Doğrusal olmayan çözüm kontrol ayarları",
            "Model raporundan nonlinear çözüm kontrol verisi okundu: "+cueSummary(cues,10)+".",
            "İterasyon/tolerans/event-stepping ayarlarının kullanılan nonlinear analiz yaklaşımıyla tutarlı olduğunu doğrulayın.",Collections.emptyList()));
    }

    private static void stagedConstructionChecks(MusaAiStructuralCalc.Model calc,List<Finding>out){
        if(calc==null)return;
        List<String>cues=serviceReportCues(calc,"staged construction","construction stage","stage definition","stage sequence","staged nonlinear","asama tanimi","aşama tanımı","yapim asamasi","yapım aşaması");
        if(cues.isEmpty()){
            out.add(new Finding("ST-98",Status.DOGRULANAMADI,"Yapım aşaması / staged-construction tanımı okunamadı",
                "Yapım aşamalı analiz kullanılıp kullanılmadığı veya stage sıralamasına ilişkin açık model raporu ayrıştırılamadı.",
                "Staged-construction kullanılan projelerde aşama sırası, eklenen/çıkarılan elemanlar ve yüklerin hangi aşamada devreye girdiğini raporlayın.",Collections.emptyList()));
            return;
        }
        Status s=assignmentCueStatus(cues,"stage missing","invalid stage","stage sequence error","duplicate stage","construction stage error","asama sirasi hatali","aşama sırası hatalı","error","failed");
        out.add(new Finding("ST-98",s,"Yapım aşaması / staged-construction kontrolü",
            "Model raporundan yapım aşaması verisi okundu: "+cueSummary(cues,10)+".",
            "Aşamaların sırasını, aktive/deaktive edilen elemanları ve aşama yüklerini proje yapım senaryosuyla karşılaştırın.",Collections.emptyList()));
    }

    private static List<String> loadCues(MusaAiStructuralCalc.Model calc,String...terms){
        ArrayList<String>out=new ArrayList<>();
        if(calc==null)return out;
        for(String cue:calc.loadCues){
            String q=MusaAiDrawingIndex.normalize(cue);
            if(has(q,terms)&&!out.contains(cue))out.add(cue);
        }
        return out;
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
            "zemin sinifi","zemin tasima gucu","zemin emniyet gerilmesi","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu");
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
