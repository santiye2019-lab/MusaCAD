package com.musa.cad;

import java.text.Normalizer;
import java.util.*;
import java.util.regex.*;

/**
 * Extracts *candidates* from a user's lawfully held YFK price-book PDF.
 * No price, code, unit or technical specification is manufactured.
 * PDF text is parsed only on the user's device, not sent to MusaCAD servers.
 */
public final class MusaAiYfk2025Parser {
    private static final Pattern ROW=Pattern.compile(
        "^[\\s\\u00a0]*(?:[NSoıaır]\\s*)?((?:10|15|20|25|30|35)\\.\\d{3}\\.\\d{4}(?:/\\d{1,3})?)\\s+(.+)$");
    private static final Pattern AMOUNT=Pattern.compile(
        "(?:^|\\s)(\\d{1,3}(?:\\.\\d{3})*,\\d{2}|\\d+,\\d{2})\\s*$");
    private static final Pattern TWO_AMOUNTS=Pattern.compile(
        "(?:^|\\s)(\\d{1,3}(?:\\.\\d{3})*,\\d{2}|\\d+,\\d{2})\\s+"+
        "(\\d{1,3}(?:\\.\\d{3})*,\\d{2}|\\d+,\\d{2})\\s*$");
    private static final Pattern UNIT=Pattern.compile(
        "(?i)(?:^|\\s)(m²|m³|m2|m3|m|ad\\.?|adet|sa|kg|ton|tk\\.?|takım|km|lt|l|çift)\\s*$");
    private static final Pattern EXPLICIT_UNIT=Pattern.compile(
        "(?i)\\(\\s*[ÖO]lçü\\s*:\\s*([^)]{1,25})\\)");
    private static final Pattern ANY_CODE=Pattern.compile(
        "(?<!\\d)(?:10|15|20|25|30|35)\\.\\d{3}\\.\\d{4}(?:/\\d{1,3})?(?!\\d)");
    public static final class Item {
        public final String code,description,unit,price2025,mounting2025,section;
        public final int pdfPage;
        Item(String code,String description,String unit,String price,String mounting,String section,int pdfPage){
            this.code=code;this.description=description;this.unit=unit;
            this.price2025=price;this.mounting2025=mounting;
            this.section=section;this.pdfPage=pdfPage;
        }
    }
    public static String fold(String text){
        if(text==null)return "";
        return Normalizer.normalize(text.toLowerCase(new Locale("tr","TR"))
            .replace('ı','i'),Normalizer.Form.NFD)
            .replaceAll("\\p{M}+","").replaceAll("\\s+"," ").trim();
    }
    public static String findCode(String input){
        if(input==null)return "";
        Matcher m=ANY_CODE.matcher(input.replace(" ",""));
        return m.find()?m.group():"";
    }
    public static String searchTerm(String input){
        if(input==null)return "";
        String code=findCode(input);
        if(!code.isEmpty())return code;
        String n=fold(input).replaceAll(
            "\\b(2025|kitabinda|kitabini|kitabi|katalogda|katalog|pozlari|pozunu|pozun|poz|fiyati|fiyatini|fiyat|tanimi|tanimi|tarifi|ara|bul|sorgula|yfk|csb|csidb|goster|bana)\\b"," ")
            .replaceAll("\\s+"," ").trim();
        return n.length()>=3?n.substring(0,Math.min(n.length(),65)):"";
    }
    public static boolean wantsImport(String input){
        String n=fold(input);
        return (n.contains("2025")&&
            (n.contains("kitab")||n.contains("katalog")||n.contains("birim fiyat")||
                n.contains("poz listesi"))) &&
            (n.contains("yukle")||n.contains("aktar")||n.contains("ice aktar")||n.contains("kur"));
    }
    public static boolean wantsStatus(String input){
        String n=fold(input);
        return n.contains("2025")&&
            (n.contains("katalog durumu")||n.contains("kitap durumu")||n.contains("poz durumu"));
    }
    public static boolean wantsCompare(String input){
        String n=fold(input);
        return n.contains("2025")&&n.contains("2026")&&
            (n.contains("yeni poz")||n.contains("karsilastir")||n.contains("fark"));
    }
    public static boolean wantsLookup(String input){
        String n=fold(input);
        return n.contains("2025")&&!wantsImport(input)&&!wantsCompare(input)&&!wantsStatus(input)&&
            (n.contains("poz")||n.contains("katalog")||n.contains("kitab"));
    }

    /**
     * Preserves source row texts and only separates clearly delimited monetary
     * columns. Returns empty price on ambiguous layouts; never guesses.
     * Parent-item technical scope can be on a previous page; the returned
     * description is a page-local excerpt, NOT the complete official tariff.
     */
    public static List<Item> parsePage(String page,int pdfPage){
        if(page==null||page.trim().isEmpty())return Collections.emptyList();
        String[] lines=page.replace("\r","").split("\n");
        ArrayList<Item> result=new ArrayList<>();
        String section="", inheritedUnit="", code=null;
        StringBuilder desc=new StringBuilder();
        String firstPrice="",firstMount="",explicitUnit="";
        for(String raw:lines){
            String line=raw.trim();
            if(line.isEmpty())continue;
            if(line.matches("(?i)^.*(?:Sıhhi Tesisat|Isıtma Sistemleri Tesisatı|Müşterek Tesisat|Havalandırma ve Klima Tesisatı|Asansör Tesisatı|Kuvvetli Akım|Zayıf Akım|Yangından Korunma).*")
                &&line.length()<135)section=line.substring(0,Math.min(line.length(),120));
            Matcher u=EXPLICIT_UNIT.matcher(line);
            if(u.find())inheritedUnit=u.group(1).trim();
            Matcher r=ROW.matcher(line);
            if(r.matches()){
                if(code!=null){
                    result.add(make(code,desc.toString(),explicitUnit,firstPrice,firstMount,
                        section,pdfPage));
                }
                code=r.group(1);desc.setLength(0);
                firstPrice="";firstMount="";explicitUnit="";
                String entry=r.group(2).trim();
                Matcher amounts=TWO_AMOUNTS.matcher(entry);
                if(amounts.find()){
                    firstPrice=amounts.group(1);firstMount=amounts.group(2);
                    entry=entry.substring(0,amounts.start()).trim();
                } else {
                    Matcher amount=AMOUNT.matcher(entry);
                    if(amount.find()){
                        firstPrice=amount.group(1);
                        entry=entry.substring(0,amount.start()).trim();
                    }
                }
                Matcher unit=UNIT.matcher(entry);
                if(unit.find()){
                    explicitUnit=unit.group(1).trim();
                    entry=entry.substring(0,unit.start()).trim();
                } else if(!inheritedUnit.isEmpty()&&entry.contains("(Ölçü:")){
                    explicitUnit=inheritedUnit;
                }
                desc.append(entry);
                continue;
            }
            if(code!=null && desc.length()<1200){
                if(line.matches("(?i)^(S|ı|r|a|N|o|Poz No|Birim Fiyat|Yapılacak İşin Cinsi|Montaj Bedeli|TL|Montajlı|Ölçü Birimi).*$")
                    ||line.matches("^[-–]?\\d+[-–]?$")
                    ||line.matches("^\\d\\d[.]\\d\\d[.]\\d\\d\\d\\d$"))continue;
                if(desc.length()>0)desc.append(' ');
                desc.append(line.substring(0,Math.min(220,line.length())));
            }
        }
        if(code!=null)result.add(make(code,desc.toString(),explicitUnit,firstPrice,firstMount,
            section,pdfPage));
        return result;
    }
    private static Item make(String code,String desc,String unit,String price,String mounting,
                             String section,int page){
        String concise=desc.replaceAll("\\s+"," ").trim();
        if(concise.length()>1000)concise=concise.substring(0,1000);
        return new Item(code,concise,unit,price,mounting,section,page);
    }
    public static Set<String> pageCodes(String page){
        LinkedHashSet<String> codes=new LinkedHashSet<>();
        if(page==null)return codes;
        for(String line:page.split("\\r?\\n")){
            Matcher m=ROW.matcher(line.trim());
            if(m.matches())codes.add(m.group(1));
        }
        return codes;
    }
    private MusaAiYfk2025Parser(){}
}
