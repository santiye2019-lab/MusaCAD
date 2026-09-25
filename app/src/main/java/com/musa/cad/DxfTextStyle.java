package com.musa.cad;

import java.util.*;

/** Pure-Java DXF STYLE table parser and text-style resolver. */
public final class DxfTextStyle {
    public static final String STANDARD="STANDARD";

    public static final class Style {
        public final String name,fontFile,bigFontFile;
        public final double fixedHeight,widthFactor,obliqueDegrees;
        public final int flags,generationFlags;
        Style(String name,String fontFile,String bigFontFile,double fixedHeight,double widthFactor,double obliqueDegrees,int flags,int generationFlags){
            this.name=normalize(name);this.fontFile=clean(fontFile);this.bigFontFile=clean(bigFontFile);
            this.fixedHeight=finitePositiveOrZero(fixedHeight);this.widthFactor=finitePositive(widthFactor,1d);
            this.obliqueDegrees=Double.isFinite(obliqueDegrees)?obliqueDegrees:0d;this.flags=flags;this.generationFlags=generationFlags;
        }
        public boolean usesShx(){String f=fontFile.toLowerCase(Locale.ROOT);return f.endsWith(".shx")||(!f.isEmpty()&&!f.contains("."));}
        public String familyHint(){
            if(usesShx())return "monospace";String f=fontFile.replace('\\','/');int slash=f.lastIndexOf('/');if(slash>=0)f=f.substring(slash+1);int dot=f.lastIndexOf('.');if(dot>0)f=f.substring(0,dot);f=f.replace('_',' ').replace('-',' ').trim();return f.isEmpty()?"sans":f;
        }
        public float textHeight(float entityHeight){return (float)(fixedHeight>0d?fixedHeight:Math.max(.01d,entityHeight));}
        public float width(float entityScale){return (float)(widthFactor*finitePositive(entityScale,1d));}
        public float oblique(float entityOblique){return (float)(Math.abs(entityOblique)>1e-9?entityOblique:obliqueDegrees);}
        public int generation(int entityFlags){return generationFlags|entityFlags;}
    }

    public static Map<String,Style> parse(List<String> tags){
        LinkedHashMap<String,Style> out=new LinkedHashMap<>();if(tags==null)return defaults(out);
        for(int i=0;i+1<tags.size();){
            if(code(tags.get(i))!=0||!"STYLE".equalsIgnoreCase(tags.get(i+1).trim())){i+=2;continue;}
            int from=i+2,to=from;while(to+1<tags.size()&&code(tags.get(to))!=0)to+=2;
            String name=text(tags,from,to,2,STANDARD);String font=text(tags,from,to,3,"");if(font.isEmpty())font=extendedFontHint(tags,from,to);Style style=new Style(name,font,text(tags,from,to,4,""),number(tags,from,to,40,0d),number(tags,from,to,41,1d),number(tags,from,to,50,0d),integer(tags,from,to,70,0),integer(tags,from,to,71,0));out.put(style.name,style);i=to;
        }
        return defaults(out);
    }

    public static Style resolve(Map<String,Style> styles,String name){
        if(styles!=null){Style s=styles.get(normalize(name));if(s!=null)return s;s=styles.get(STANDARD);if(s!=null)return s;}
        return defaultStyle();
    }

    public static Style defaultStyle(){return new Style(STANDARD,"", "",0d,1d,0d,0,0);}
    public static String normalize(String name){String s=name==null?"":name.trim();return s.isEmpty()?STANDARD:s.toUpperCase(Locale.ROOT);}

    private static Map<String,Style> defaults(LinkedHashMap<String,Style> out){if(!out.containsKey(STANDARD))out.put(STANDARD,defaultStyle());return Collections.unmodifiableMap(out);}
    private static String clean(String s){return s==null?"":s.trim();}
    private static double finitePositive(double v,double fallback){return Double.isFinite(v)&&v>0d?v:fallback;}
    private static double finitePositiveOrZero(double v){return Double.isFinite(v)&&v>0d?v:0d;}
    private static int code(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static String extendedFontHint(List<String>a,int from,int to){for(int i=from;i+1<to;i+=2)if(code(a.get(i))==1000){String v=a.get(i+1).trim();if(!v.isEmpty()&&v.length()<=128)return v;}return "";}
    private static String text(List<String>a,int from,int to,int wanted,String fallback){for(int i=from;i+1<to;i+=2)if(code(a.get(i))==wanted)return a.get(i+1).trim();return fallback;}
    private static double number(List<String>a,int from,int to,int wanted,double fallback){try{return Double.parseDouble(text(a,from,to,wanted,Double.toString(fallback)));}catch(Exception e){return fallback;}}
    private static int integer(List<String>a,int from,int to,int wanted,int fallback){try{return Integer.parseInt(text(a,from,to,wanted,Integer.toString(fallback)));}catch(Exception e){return fallback;}}
    private DxfTextStyle(){}
}
