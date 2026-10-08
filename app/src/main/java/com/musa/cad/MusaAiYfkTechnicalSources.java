package com.musa.cad;

import java.net.URI;
import java.text.Normalizer;
import java.util.*;
import java.util.regex.*;

/**
 * Source registry and search intent for the 2026 YFK mechanical analysis books.
 * Only URLs, classifications and program logic are committed to GitHub.
 * The books and their extracted text are downloaded into app-private storage
 * on a user's explicit request; no official PDF or copied catalog is bundled.
 */
public final class MusaAiYfkTechnicalSources {
    public static final String EDITION = "2026-01";
    public static final class Volume {
        public final String id, title, url;
        public Volume(String id, String title, String url) {
            this.id=id; this.title=title; this.url=url;
            if (!isOfficial(url)) throw new IllegalArgumentException("Nonofficial YFK PDF");
        }
    }
    public static final Volume[] VOLUMES = {
        new Volume("mekanik1", "Mekanik tesisat analizleri 1 — sıhhi/ısıtma",
            "https://webdosya.csb.gov.tr/v2/yfk/2026/01/mekanik-analiz-1-sihhi-s-tma-tes-20260115162956.pdf"),
        new Volume("mekanik2", "Mekanik tesisat analizleri 2",
            "https://webdosya.csb.gov.tr/v2/yfk/2026/01/mekanik-analiz-2-m-t-tes-20260115163006.pdf"),
        new Volume("mekanik3", "Mekanik tesisat analizleri 3",
            "https://webdosya.csb.gov.tr/v2/yfk/2026/01/Mekanik-Tesisat-Birim-Fiyat-Analizleri-3-20260121121534.pdf")
    };
    private static final Pattern POZ=Pattern.compile(
        "(?<![0-9])25\\.\\d{3}\\.\\d{4}(?:/\\d{1,3})?(?![0-9])");
    public static String normalize(String text) {
        if(text==null)return "";
        String lower=text.toLowerCase(new Locale("tr","TR")).replace('ı','i');
        String basic=Normalizer.normalize(lower,Normalizer.Form.NFD)
            .replaceAll("\\p{M}+","").replaceAll("[^a-z0-9./ ]+"," ");
        return basic.trim().replaceAll("\\s+"," ");
    }
    public static boolean isOfficial(String raw) {
        try {
            URI u=new URI(raw);
            return "https".equalsIgnoreCase(u.getScheme())
                && "webdosya.csb.gov.tr".equalsIgnoreCase(u.getHost())
                && u.getUserInfo()==null && u.getPort()==-1
                && u.getFragment()==null && u.getQuery()==null
                && u.getPath()!=null && u.getPath().startsWith("/v2/yfk/2026/01/")
                && u.getPath().endsWith(".pdf");
        }catch(Exception ignored){return false;}
    }
    public static List<String> codesIn(String text) {
        LinkedHashSet<String> codes=new LinkedHashSet<>();
        if(text!=null) {
            Matcher match=POZ.matcher(text);
            while(match.find() && codes.size()<250)codes.add(match.group());
        }
        return new ArrayList<>(codes);
    }
    public static String query(String text) {
        if(text==null)return "";
        Matcher code=POZ.matcher(text);
        if(code.find())return code.group();
        String q=normalize(text);
        q=q.replaceAll("\\b(mekanik|tesisat|yfk|csb|csidb|analizleri|analizi|analizde|analiz|kitabinda|kitabindan|pozlari|pozunu|poz|ara|bul|sorgula|tarifi|tarifini|tanimini|tanim|fiyat|incele|goster|bana|bir)\\b"," ")
            .trim().replaceAll("\\s+"," ");
        return q.length()<3?"":q.substring(0,Math.min(q.length(),80));
    }
    public static boolean wantsInstall(String text) {
        String n=normalize(text);
        return (n.contains("mekanik analiz")||n.contains("yfk analiz")||
                n.contains("poz analiz")||n.contains("mekanik poz arsiv"))
            && (n.contains("indir")||n.contains("kur")||n.contains("yukle"));
    }
    public static boolean wantsStatus(String text) {
        String n=normalize(text);
        return (n.contains("mekanik analiz")||n.contains("yfk analiz"))
            && (n.contains("durum")||n.contains("hazir mi"));
    }
    public static boolean wantsLookup(String text) {
        String n=normalize(text);
        return (n.contains("mekanik analiz")||n.contains("yfk analiz")||
                n.contains("poz tarifi")||n.contains("poz analizi"))
            && !wantsInstall(text) && !wantsStatus(text)
            && !wantsMatch(text);
    }
    public static boolean wantsMatch(String text) {
        String n=normalize(text);
        return n.contains("poz eslestir")||n.contains("pozlarla eslestir")||
               n.contains("malzemeleri pozlarla eslestir")||
               n.contains("kesif pozlarini oner");
    }
    /** Domain hints only; not proof that a DWG item matches a published position. */
    public static String category(String code) {
        if(code==null || !POZ.matcher(code).matches())return "Doğrulanmamış mekanik poz";
        int group=Integer.parseInt(code.substring(3,6));
        if(group<200)return "Sıhhi tesisat";
        if(group<300)return "Isıtma sistemleri";
        if(group<450)return "Müşterek tesisat";
        if(group<550)return "Havalandırma ve klima";
        if(group<600)return "Otomatik kontrol";
        if(group<650)return "Mutfak ve çamaşırhane";
        if(group<700)return "Hastane tesisatı";
        if(group<800)return "Yangın tesisatı";
        return "Diğer mekanik tesisat";
    }
    private MusaAiYfkTechnicalSources(){}
}
