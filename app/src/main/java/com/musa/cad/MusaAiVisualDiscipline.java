package com.musa.cad;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Locale;

/**
 * Selects the discipline for a Qwen-first visual CAD audit.
 * An explicit user discipline always wins. Otherwise use the open DWG name
 * and then independent layer evidence; do not guess from a generic floor plan.
 * This class never certifies findings or authorizes drawing edits.
 */
public final class MusaAiVisualDiscipline {
    private static final String[] SCOPES={
        "wastewater","sanitary","fire","ventilation","heating","cooling","gas",
        "mechanical","structural","architectural","electrical","landscape",
        "infrastructure","elevator"
    };

    public static String resolve(String requestedScope,String userPrompt,
                                 String fileName,MusaAiDrawingIndex index){
        String explicit=requestedScope==null?"all":requestedScope;
        if(!"all".equals(explicit))return explicit;
        String q=MusaAiDrawingIndex.normalize(userPrompt);
        if(has(q,"tum disiplin","butun disiplin","disiplinler arasi",
                 "multidisiplin","tüm disiplin","her disiplini"))return "all";

        String filename=MusaAiDrawingIndex.normalize(fileName);
        String named=inferName(filename);
        if(!"all".equals(named))return named;

        Map<String,Integer> scores=new LinkedHashMap<>();
        if(index!=null)for(String layer:index.allLayers){
            String n=MusaAiDrawingIndex.normalize(layer);
            String found=classifyLayer(n);
            if(!"all".equals(found))scores.put(found,scores.getOrDefault(found,0)+1);
        }
        String best="all";
        int highest=0,second=0;
        for(Map.Entry<String,Integer> e:scores.entrySet()){
            int score=e.getValue();
            if(score>highest){second=highest;highest=score;best=e.getKey();}
            else if(score>second)second=score;
        }
        // A few incidental xref layers must not change the report discipline.
        return highest>=3 && highest>=second*2 ? best:"all";
    }

    private static String inferName(String name){
        if(name.isEmpty())return "all";
        // Specialized mechanical drawing titles take precedence over generic "tesisat".
        for(String scope:new String[]{"wastewater","sanitary","fire","ventilation",
                "heating","cooling","gas","structural","architectural",
                "electrical","landscape","infrastructure","elevator"}){
            if(nameMatches(name,scope))return scope;
        }
        return nameMatches(name,"mechanical")?"mechanical":"all";
    }

    private static boolean nameMatches(String s,String scope){
        switch(scope){
            case "wastewater":return has(s,"pis su","atik su","kanalizasyon","wastewater");
            case "sanitary":return has(s,"sihhi tesisat","temiz su","sicak su tesisat");
            case "fire":return has(s,"yangin tesisat","sprinkler","hidrant");
            case "ventilation":return has(s,"havalandirma","hava kanali","ventilation");
            case "heating":return has(s,"isitma tesisat","kalorifer");
            case "cooling":return has(s,"sogutma","klima","vrf");
            case "gas":return has(s,"dogalgaz","gaz tesisat");
            case "mechanical":return has(s,"mekanik","tesisat","mechanical","hvac");
            case "structural":return has(s,"statik","betonarme","kalip plani","structural");
            case "architectural":return has(s,"mimari","mimarlik","architectural");
            case "electrical":return has(s,"elektrik","kuvvetli akim","zayif akim","electrical");
            case "landscape":return has(s,"peyzaj","bitkilendirme","landscape");
            case "infrastructure":return has(s,"altyapi","infrastructure");
            case "elevator":return has(s,"asansor","elevator");
            default:return false;
        }
    }

    private static String classifyLayer(String layer){
        // Strong layer prefixes, not generic one-letter or architectural xrefs.
        if(starts(layer,"mek-","mek_","mek ","m-tesisat","m-sihhi","m-pis"))return "mechanical";
        if(starts(layer,"sta-","sta_","statik","s-kiris","s-kolon","s-perde"))return "structural";
        if(starts(layer,"mim-","mim_","mimari","a-wall","a-door"))return "architectural";
        if(starts(layer,"elk-","elk_","elektrik","e-pano","e-kablo"))return "electrical";
        if(starts(layer,"pey-","pey_","peyzaj"))return "landscape";
        if(starts(layer,"alt-","alt_","altyapi"))return "infrastructure";
        if(starts(layer,"asn-","asn_","asansor"))return "elevator";
        if(starts(layer,"yng-","yng_","yangin"))return "fire";
        return "all";
    }

    public static MusaAiDiscipline localDiscipline(String scope){
        switch(scope==null?"all":scope){
            case "structural":return MusaAiDiscipline.STRUCTURAL;
            case "architectural":return MusaAiDiscipline.ARCHITECTURAL;
            case "electrical":return MusaAiDiscipline.ELECTRICAL;
            case "landscape":return MusaAiDiscipline.LANDSCAPE;
            case "infrastructure":return MusaAiDiscipline.INFRASTRUCTURE;
            case "elevator":return MusaAiDiscipline.ELEVATOR;
            case "fire":return MusaAiDiscipline.FIRE_SAFETY;
            default:return MusaAiDiscipline.MECHANICAL;
        }
    }

    public static boolean mechanicalScope(String scope){
        return "mechanical".equals(scope)||"sanitary".equals(scope)||
            "wastewater".equals(scope)||
            "ventilation".equals(scope)||"heating".equals(scope)||
            "cooling".equals(scope)||"gas".equals(scope);
    }

    public static String focus(String scope){
        switch(scope==null?"all":scope){
            case "wastewater":return "pis su kolonları, hat çapları, boru eğimleri, havalık, süzgeç ve şebeke çıkış kotları";
            case "sanitary":return "temiz ve sıcak su hatları, çaplar, vana/armatürler, hidrofor, boyler ve debi-basınç hesabı referansları";
            case "fire":return "sprinkler/hidrant yerleşimi, yangın pompası, zonlama, vana ve yangın senaryosu ilişkileri";
            case "ventilation":return "hava kanalı güzergâhı ve kesitleri, debiler, santral/fan, şaft, menfez ve yangın damperleri";
            case "heating":return "kazan/ısı pompası, borular, radyatörler, kolektörler, çaplar ve ısıtma zonları";
            case "cooling":return "VRF/klima üniteleri, soğutucu akışkan hatları, drenaj, kapasite ve iç-dış ünite bağlantıları";
            case "gas":return "doğalgaz hatları, çaplar, kesme vanaları, havalandırma, sayaç ve cihaz bağlantıları";
            case "mechanical":return "sıhhi tesisat, atık su, yangın, HVAC, cihaz kapasitesi, boru/kanal güzergâhları ve düşey şaftlar";
            case "structural":return "kolon-kiriş-perde-döşeme-temel, aks/kot, kesit/donatı notları, rezervasyonlar ve hesap raporuyla uyum";
            case "architectural":return "mahal ve kullanım alanları, ölçülendirme, kot, kapı/merdiven, erişilebilirlik, kaçış ve plan-kesit uyumu";
            case "electrical":return "pano, kablo/boru ve tava güzergâhı, devreler, aydınlatma, topraklama, güç ve kumanda beslemeleri";
            case "landscape":return "bitkilendirme, sert zemin, drenaj, sulama, topoğrafya kotları ve mimari sınır uyumu";
            case "infrastructure":return "su/atık su/yağmur suyu hatları, rögarlar, çap-eğim-kot, şebeke bağlantıları ve güzergâh çakışmaları";
            case "elevator":return "kuyu ölçüsü, durak kapıları, makine/ekipman alanı, pit ve üst boşluk, elektrik ve yangın koordinasyonu";
            default:return "tüm tespit edilebilir disiplinlerde ayrı teknik bulgular ve disiplinlerarası koordinasyon";
        }
    }

    /** Prepend to BOTH overview and closeup Qwen calls; no provider/permission changes. */
    public static String visionInstruction(String scope){
        String label=MusaAiAnalysisIntent.label(scope);
        StringBuilder b=new StringBuilder("GÖRSEL MÜHENDİSLİK RAPOR DİSİPLİNİ: ")
            .append(label).append(". Ana kontrol konuları: ").append(focus(scope)).append(".");
        b.append("\nAna bulguları yalnız bu disiplinin teknik konularına göre sınıflandır.");
        b.append(" Diğer disiplinleri sadece doğrudan etkileşen koordinasyon riski varsa ayrı başlıkta belirt.");
        b.append(" Ölçü/etiket okunamıyorsa tahmin etme; kesit, plan, vaziyet ve kotları bu disiplin açısından kontrol et.");
        b.append(" Bulguda çizim/görüntü bölgesi, okunan değer, olası sorun, kanıt durumu ve düzeltme önerisi olsun.");
        b.append(" Görsel gözlemi CAD/saha doğrulaması gibi sunma; çizimde görünmeyen ekipman ya da hesap uydurma.");
        if("all".equals(scope))b.append(" Bulguları disiplin isimlerine göre ayrı alt bölümlere ayır.");
        return b.toString();
    }

    public static String heading(String scope){
        return MusaAiAnalysisIntent.label(scope).toUpperCase(new Locale("tr","TR"))+
            " GÖRSEL + YEREL MÜHENDİSLİK DENETİM RAPORU";
    }

    private static boolean has(String s,String... tokens){
        for(String token:tokens)if(s.contains(MusaAiDrawingIndex.normalize(token)))return true;
        return false;
    }
    private static boolean starts(String s,String... tokens){
        for(String token:tokens)if(s.startsWith(token))return true;
        return false;
    }
    private MusaAiVisualDiscipline(){}
}
