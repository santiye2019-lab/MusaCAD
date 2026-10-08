package com.musa.cad;

import java.net.URI;
import java.util.Locale;
import java.util.regex.*;

/** Pinned official YFK catalog source and safe on-device code/term lookup rules. */
public final class MusaAiYfkCatalogQuery {
    public static final String SOURCE_URL=
        "https://webdosya.csb.gov.tr/v2/yfk/2026/01/1-BF-202619011535-20260119155143.pdf";
    public static final String BOOK_YEAR="2026";
    public static final String PRICE_PERIOD="2026-01";
    private static final Pattern CODE=Pattern.compile(
        "(?<![0-9])(?:10|15|20|25|30|35)\\.\\d{3}\\.\\d{4}(?:/\\d{1,3})?(?![0-9])");

    public static boolean isOfficialUrl(String raw){
        try{
            URI value=new URI(raw);
            return "https".equalsIgnoreCase(value.getScheme())&&
                "webdosya.csb.gov.tr".equalsIgnoreCase(value.getHost())&&
                value.getUserInfo()==null&&value.getPort()==-1&&
                value.getQuery()==null&&value.getFragment()==null&&
                value.getPath()!=null&&value.getPath().startsWith("/v2/yfk/2026/01/")&&
                value.getPath().endsWith(".pdf");
        }catch(Exception e){return false;}
    }

    public static boolean wantsDownload(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return (q.contains("csb")||q.contains("csidb")||q.contains("yfk")||
            q.contains("birim fiyat"))&&
            (q.contains("kitabini indir")||q.contains("kitabi indir")||
             q.contains("arsivi indir")||q.contains("kitabini kur")||
             q.contains("kitabi yukle")||q.contains("arsivi kur"));
    }

    public static boolean wantsStatus(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return (q.contains("csb")||q.contains("csidb")||q.contains("yfk"))&&
            (q.contains("kitap durumu")||q.contains("arsiv durumu")||
            q.contains("katalog durumu"));
    }

    public static String lookup(String raw){
        if(raw==null)return "";
        Matcher m=CODE.matcher(raw.replace(" ",""));
        if(m.find())return m.group();
        String n=MusaAiDrawingIndex.normalize(raw);
        if(!(n.contains("csb")||n.contains("csidb")||n.contains("yfk")||
             n.contains("poz")||n.contains("birim fiyat")))return "";
        if(!(n.contains("ara")||n.contains("bul")||n.contains("sorgula")||
             n.contains("hangi")||n.contains("fiyat")||n.contains("tanim")))return "";
        String filtered=(" "+n+" ")
            .replace(" csb "," ").replace(" csidb "," ").replace(" yfk "," ")
            .replace(" kitabinda "," ").replace(" kitabini "," ").replace(" katalogda "," ")
            .replace(" kitabindan "," ").replace(" kitabı "," ").replace(" arsivinde "," ")
            .replace(" pozunu "," ").replace(" pozlari "," ").replace(" poz "," ")
            .replace(" fiyatlari "," ").replace(" fiyati "," ").replace(" birim "," ")
            .replace(" ara "," ").replace(" bul "," ").replace(" sorgula "," ")
            .replace(" 2026 "," ").trim().replaceAll("\\s+"," ");
        return filtered.length()>=3?filtered.substring(0,Math.min(85,filtered.length())):"";
    }

    /** Fold Turkish characters without changing offsets in common PDF-extracted lines. */
    public static String fold(String raw){
        if(raw==null)return "";
        return raw.toLowerCase(new Locale("tr","TR"))
            .replace('ı','i').replace('ğ','g').replace('ü','u')
            .replace('ş','s').replace('ö','o').replace('ç','c');
    }

    public static String safeSnippet(String pageText,String term){
        if(pageText==null||pageText.isEmpty()||term==null||term.isEmpty())return "";
        int i=fold(pageText).indexOf(fold(term));
        if(i<0)return "";
        int a=Math.max(0,i-135),b=Math.min(pageText.length(),i+260);
        return (a>0?"…":"")+pageText.substring(a,b)
            .replaceAll("[\\r\\n]+"," ").replaceAll("[ \\t]+"," ").trim()+
            (b<pageText.length()?"…":"");
    }

    private MusaAiYfkCatalogQuery(){}
}
