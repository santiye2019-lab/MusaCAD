package com.musa.cad;

import java.util.*;

/** Parses DXF STYLE table entries and exposes safe text-style defaults. */
public final class DxfTextStyles {
    public static final String STANDARD="STANDARD";

    public static final class Entry {
        public final String name,font,bigFont;
        public final double fixedHeight,widthFactor,oblique;
        public final int flags,generationFlags;
        Entry(String name,String font,String bigFont,double fixedHeight,double widthFactor,double oblique,int flags,int generationFlags){
            this.name=name;this.font=font;this.bigFont=bigFont;
            this.fixedHeight=safeNonNegative(fixedHeight);this.widthFactor=safePositive(widthFactor,1d);
            this.oblique=Double.isFinite(oblique)?oblique:0d;this.flags=flags;this.generationFlags=generationFlags;
        }
        public String familyHint(){return fontFamilyHint(font);}
    }

    public static final class Table {
        private final LinkedHashMap<String,Entry> entries=new LinkedHashMap<>();
        public Table(){ensureStandard();}
        public void add(String name,String font,String bigFont,double fixedHeight,double widthFactor,double oblique,int flags,int generationFlags){
            String n=key(name);if(n.isEmpty())return;
            entries.put(n,new Entry(n,clean(font),clean(bigFont),fixedHeight,widthFactor,oblique,flags,generationFlags));
        }
        public Entry get(String name){Entry e=entries.get(key(name));return e!=null?e:entries.get(STANDARD);}
        public int size(){return entries.size();}
        public Set<String> names(){return Collections.unmodifiableSet(entries.keySet());}
        public void ensureStandard(){if(!entries.containsKey(STANDARD))entries.put(STANDARD,new Entry(STANDARD,"","",0,1,0,0,0));}
    }

    public static Table parse(List<String> tags){
        Table table=new Table();
        for(int i=0;i+1<tags.size();i+=2){
            if(code(tags.get(i))!=0||!"STYLE".equalsIgnoreCase(tags.get(i+1).trim()))continue;
            int from=i+2,to=from;while(to+1<tags.size()&&code(tags.get(to))!=0)to+=2;
            table.add(text(tags,from,to,2,STANDARD),text(tags,from,to,3,""),text(tags,from,to,4,""),
                number(tags,from,to,40,0),number(tags,from,to,41,1),number(tags,from,to,50,0),
                (int)number(tags,from,to,70,0),(int)number(tags,from,to,71,0));
            i=Math.max(i,to-2);
        }
        table.ensureStandard();return table;
    }

    /** Android font-family hint. SHX names fall back to a visually similar system family. */
    public static String fontFamilyHint(String font){
        String raw=clean(font);if(raw.isEmpty())return "sans-serif";
        int slash=Math.max(raw.lastIndexOf('/'),raw.lastIndexOf('\\'));if(slash>=0)raw=raw.substring(slash+1);
        int dot=raw.lastIndexOf('.');String base=(dot>0?raw.substring(0,dot):raw).trim();String k=base.toLowerCase(Locale.ROOT);
        if(k.contains("mono")||k.contains("simplex")||k.equals("txt")||k.contains("gothic"))return "monospace";
        if(k.contains("roman")||k.contains("times")||k.contains("serif"))return "serif";
        if(k.contains("arial")||k.contains("helvetica")||k.contains("calibri")||k.contains("sans"))return "sans-serif";
        return base.isEmpty()?"sans-serif":base;
    }

    private static String key(String s){return clean(s).toUpperCase(Locale.ROOT);}
    private static String clean(String s){return s==null?"":s.trim().replace("\"","");}
    private static int code(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static String text(List<String>a,int from,int to,int wanted,String fallback){for(int i=from;i+1<to;i+=2)if(code(a.get(i))==wanted)return a.get(i+1).trim();return fallback;}
    private static double number(List<String>a,int from,int to,int wanted,double fallback){try{return Double.parseDouble(text(a,from,to,wanted,Double.toString(fallback)));}catch(Exception e){return fallback;}}
    private static double safePositive(double v,double fallback){return Double.isFinite(v)&&v>1e-9?v:fallback;}
    private static double safeNonNegative(double v){return Double.isFinite(v)&&v>=0?v:0;}
    private DxfTextStyles(){}
}
