package com.musa.cad;

import java.util.Locale;

/** Pure font-name policy shared by rendering, DXF persistence and tests. */
public final class CadFontPolicy {
    public static boolean isShx(String value){
        String v=baseName(value).toLowerCase(Locale.ROOT);
        return v.endsWith(".shx")||(!v.isEmpty()&&!v.contains(".")&&knownShxBase(v));
    }

    public static boolean isTtfOrOtf(String value){
        String v=baseName(value).toLowerCase(Locale.ROOT);
        return v.endsWith(".ttf")||v.endsWith(".otf");
    }

    public static String baseName(String value){
        String v=value==null?"":value.trim().replace('\\','/');
        int slash=v.lastIndexOf('/');return slash>=0?v.substring(slash+1):v;
    }

    public static String key(String value){
        String v=baseName(value).toLowerCase(Locale.ROOT);
        int dot=v.lastIndexOf('.');if(dot>0)v=v.substring(0,dot);
        StringBuilder out=new StringBuilder();
        for(int i=0;i<v.length();i++){
            char c=v.charAt(i);
            if(Character.isLetterOrDigit(c))out.append(c);
        }
        return out.toString();
    }

    public static boolean knownShx(String value){return knownShxBase(key(value));}

    private static boolean knownShxBase(String k){
        if(k==null||k.isEmpty())return false;
        return k.equals("txt")||k.equals("simplex")||k.equals("complex")||k.equals("romans")||k.equals("romand")||
            k.equals("romant")||k.equals("romanc")||k.equals("script")||k.equals("scripts")||k.equals("scriptc")||
            k.equals("italic")||k.equals("italict")||k.equals("gothicg")||k.equals("gothice")||k.equals("gothici")||
            k.startsWith("iso")||k.startsWith("isocp")||k.startsWith("greek");
    }

    public static String shxFallbackFamily(String value){
        String k=key(value);
        if(k.startsWith("roman")||k.startsWith("gothic"))return "serif";
        if(k.startsWith("script")||k.startsWith("italic"))return "sans-serif";
        return "monospace";
    }

    public static String androidFallbackFamily(String value){
        String v=baseName(value);
        if(v.isEmpty())return "sans-serif";
        String k=key(v);
        if(k.contains("arialnarrow")||k.contains("helveticacondensed")||k.contains("robotocondensed"))return "sans-serif-condensed";
        if(k.contains("courier")||k.contains("consolas")||k.contains("lucidaconsole")||k.equals("monaco"))return "monospace";
        if(k.contains("timesnewroman")||k.equals("times")||k.contains("cambria")||k.contains("georgia")||k.contains("palatino")||k.contains("baskerville")||k.contains("goudy"))return "serif";
        if(k.contains("calibri")||k.contains("segoeui")||k.contains("centurygothic")||k.contains("frutiger")||k.equals("arial")||k.equals("helvetica")||k.equals("tahoma")||k.equals("verdana")||k.equals("roboto"))return "sans-serif";
        if(k.equals("sans")||k.equals("sansserif"))return "sans-serif";
        if(k.equals("serif"))return "serif";
        if(k.equals("monospace")||k.equals("mono"))return "monospace";
        return v.replace('_',' ').replace('-',' ').replaceAll("\\.(?i:ttf|otf)$","").trim();
    }

    public static String dxfStyleName(String fontHint){
        String k=key(fontHint).toUpperCase(Locale.ROOT);
        if(k.isEmpty())k="SANS";
        if(k.length()>24)k=k.substring(0,24);
        return "MUSA_"+k;
    }

    public static String dxfFontFile(String fontHint,boolean shx){
        String base=baseName(fontHint);
        String lower=base.toLowerCase(Locale.ROOT);
        if(lower.endsWith(".ttf")||lower.endsWith(".otf")||lower.endsWith(".shx"))return base;
        if(shx){
            String k=key(base);
            return (k.isEmpty()?"txt":k)+".shx";
        }
        String family=androidFallbackFamily(base);
        if("serif".equals(family))return "times.ttf";
        if("monospace".equals(family))return "cour.ttf";
        return "arial.ttf";
    }

    private CadFontPolicy(){}
}
