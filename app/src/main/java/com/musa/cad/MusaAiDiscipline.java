package com.musa.cad;

import java.util.*;

/** Shared discipline taxonomy for MusaCAD AI project review, takeoff and reports. */
public enum MusaAiDiscipline {
    ARCHITECTURAL("MIM","Mimari"),
    STRUCTURAL("STA","Statik"),
    MECHANICAL("MEK","Mekanik"),
    ELECTRICAL("ELK","Elektrik"),
    LANDSCAPE("PEY","Peyzaj"),
    INFRASTRUCTURE("ALT","Altyapı"),
    ELEVATOR("ASN","Asansör"),
    FIRE_SAFETY("YNG","Yangın ve Can Güvenliği"),
    UNKNOWN("GEN","Genel");

    public final String code,label;
    MusaAiDiscipline(String code,String label){this.code=code;this.label=label;}

    public static MusaAiDiscipline fromQuery(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return UNKNOWN;
        if(has(q,"mimari","architect"))return ARCHITECTURAL;
        if(has(q,"statik","structural","kolon","kiris","perde","doseme","temel","radye","kazik",
            "zimbalama","modal analiz","modal","kat otelemesi","goreli kat otelemesi","kat otelen","goreli kat otelen","story drift",
            "burulma","torsion","yumusak kat","soft story","zayif kat","weak story",
            "guclu kolon","strong column","zayif kiris","weak beam","sarilma bolgesi","confinement",
            "transfer kiris","transfer doseme","transfer kat","konsol","cantilever",
            "kisa kolon","short column","perde bag kirisi","coupling beam","rijit diyafram","rigid diaphragm","semi rigid",
            "bodrum perdesi","basement wall","cevre perdesi","zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus",
            "groundwater","yeralti suyu","yer alti suyu","temel alt kotu",
            "zemin basinci","temel basinci","soil pressure","oturma","settlement","kazik kapasitesi","pile capacity","pile load",
            "uplift","yuzme","hidrostatik","p-delta","p delta","ikinci mertebe","second order",
            "taban kesme","base shear","spektrum olcekle","spectrum scale","zemin yapi etkilesimi","soil structure interaction","yay katsayisi","spring stiffness",
            "kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass",
            "kutle merkezi","rijitlik merkezi","center of mass","center of rigidity","eksantrisite","eccentricity",
            "tesadufi eksantrisite","accidental eccentricity","collector","drag strut","diyafram kirisi","diaphragm chord",
            "dusey deprem","dikey deprem","vertical earthquake","vertical seismic",
            "yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon",
            "deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","rsx","rsy",
            "r/d/i","r d i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor",
            "etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier","property modifier",
            "mafsal","hinge","release","end release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset",
            "pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı","kapasite orani","kapasite oranı","capacity ratio",
            "utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio",
            "kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment",
            "perde kesme","wall shear","shear capacity","kapasite asimi","kapasite aşımı","yetersiz eleman","uygunsuz eleman","kritik eleman",
            "sehim","deflection","servisabilite","serviceability","catlak genisligi","çatlak genişliği","crack width",
            "titresim","titreşim","vibration","comfort frequency","floor frequency",
            "sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection",
            "servis siniri","servis sınırı","serviceability limit",
            "singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness",
            "unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint",
            "mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi",
            "nonconvergence","non-convergence","did not converge","not converged","convergence failed","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti",
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
            "staged construction","construction stage","stage definition","stage sequence","staged nonlinear","asama tanimi","aşama tanımı","asamalar","aşamalar","yapim asamasi","yapım aşaması","yapim asamalari","yapım aşamaları"))return STRUCTURAL;
        if(has(q,"mekanik","hvac","vrf","pis su","temiz su","havalandirma","isitma","sogutma"))return MECHANICAL;
        if(has(q,"elektrik","kuvvetli akim","zayif akim","kablo","pano","aydinlatma","topraklama","jenerator","ups"))return ELECTRICAL;
        if(has(q,"peyzaj","bitkilendirme","sulama","sert zemin","yesil alan"))return LANDSCAPE;
        if(has(q,"altyapi","alt yapi","kanalizasyon","rog ar","rogar","telekom","isale","sebek e","sebeke"))return INFRASTRUCTURE;
        if(has(q,"asansor","elevator","lift","kuyu dibi","kuyu ustu"))return ELEVATOR;
        if(has(q,"yangin","sprinkler","hidrant","duman","basinclandirma","itfaiye"))return FIRE_SAFETY;
        return UNKNOWN;
    }

    /** Conservative discipline inference from CAD layer/text or discovery/estimate description. */
    public static MusaAiDiscipline classify(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return UNKNOWN;
        if(has(q,"yangin","sprink","hidrant","fkc","fire","duman","basinclandirma","itfaiye"))return FIRE_SAFETY;
        if(has(q,"asansor","elevator","lift","kuyu","ray","kabin","makine dairesi"))return ELEVATOR;
        if(has(q,"elektrik","elk","kablo","tava","pano","priz","armat ur","armatur","aydinlat","toprak","paratoner","jenerator","ups","data","cctv","zayif akim"))return ELECTRICAL;
        if(has(q,"mekanik","mek","pis su","atik su","temiz su","sihhi","vrf","hvac","havaland","kanal","fan","klima","isitma","sogutma","kazan","hidrofor","pompa","dogalgaz","dogal gaz","boyler"))return MECHANICAL;
        if(has(q,"statik","beton","betonarme","donati","kolon","kiris","perde","doseme","temel","radye","fore kazik","kazik",
            "zimbalama","modal analiz","kat otelemesi","goreli kat otelemesi","kat otelen","goreli kat otelen","burulma","yumusak kat","zayif kat",
            "guclu kolon","zayif kiris","sarilma bolgesi","transfer kiris","transfer doseme","transfer kat","konsol",
            "kisa kolon","perde bag kirisi","rijit diyafram","semi rigid","bodrum perdesi","cevre perdesi",
            "zemin tasima","zemin emniyet","yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu",
            "zemin basinci","temel basinci","soil pressure","oturma","settlement","kazik kapasitesi","pile capacity","pile load",
            "uplift","yuzme","hidrostatik","p-delta","p delta","ikinci mertebe","second order",
            "taban kesme","base shear","spektrum olcekle","spectrum scale","zemin yapi etkilesimi","soil structure interaction","yay katsayisi","spring stiffness",
            "kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass",
            "kutle merkezi","rijitlik merkezi","center of mass","center of rigidity","eksantrisite","eccentricity",
            "tesadufi eksantrisite","accidental eccentricity","collector","drag strut","diyafram kirisi","diaphragm chord",
            "dusey deprem","dikey deprem","vertical earthquake","vertical seismic",
            "yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon",
            "deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","rsx","rsy",
            "r/d/i","r d i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor",
            "etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier","property modifier",
            "mafsal","hinge","release","end release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset",
            "pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı","kapasite orani","kapasite oranı","capacity ratio",
            "utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio",
            "kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment",
            "perde kesme","wall shear","shear capacity","kapasite asimi","kapasite aşımı","yetersiz eleman","uygunsuz eleman","kritik eleman",
            "sehim","deflection","servisabilite","serviceability","catlak genisligi","çatlak genişliği","crack width",
            "titresim","titreşim","vibration","comfort frequency","floor frequency",
            "sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection",
            "servis siniri","servis sınırı","serviceability limit",
            "singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness",
            "unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint",
            "mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi",
            "nonconvergence","non-convergence","did not converge","not converged","convergence failed","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti",
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
            "duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","load case referans","load case referansi","load case referansı","yuk durumu referansi","yük durumu referansı"))return STRUCTURAL;
        if(has(q,"peyzaj","bitki","agac","sulama","cim","sert zemin","yumusak zemin","bordur","peyz"))return LANDSCAPE;
        if(has(q,"altyapi","kanalizasyon","rogar","yagmur suyu","drenaj","telekom","dogalgaz hatti","icme suyu","kaz i","kazi","dolgu"))return INFRASTRUCTURE;
        if(has(q,"mimari","mim","duvar","kapi","pencere","mahal","seramik","boya","asma tavan","cephe","cati","merdiven","rampa"))return ARCHITECTURAL;
        return UNKNOWN;
    }

    public static List<MusaAiDiscipline> engineering(){
        return Arrays.asList(ARCHITECTURAL,STRUCTURAL,MECHANICAL,ELECTRICAL,LANDSCAPE,INFRASTRUCTURE,ELEVATOR,FIRE_SAFETY);
    }

    private static boolean has(String q,String... terms){
        for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;
        return false;
    }
}
