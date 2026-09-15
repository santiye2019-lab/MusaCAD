package com.musa.cad;

import java.util.List;

/** Parses AutoCAD $PDMODE/$PDSIZE and resolves point marker sizing. */
public final class DxfPointStyle {
    public static final class Style {
        public final int mode;public final double size;
        public Style(int mode,double size){this.mode=mode;this.size=Double.isFinite(size)?size:0;}
        public int base(){return mode&15;}
        public boolean circle(){return (mode&32)!=0;}
        public boolean square(){return (mode&64)!=0;}
        /** PDMODE=1 alone suppresses the point. Modifier combinations such as 33 still draw their surround. */
        public boolean hidden(){return mode==1;}
    }

    public static Style parse(List<String> tags){return parseRange(tags,0,tags==null?0:tags.size());}
    public static Style parseHeaderRecord(List<String> tags){return parse(tags);}

    private static Style parseRange(List<String> tags,int from,int to){
        if(tags==null)return new Style(0,0);int mode=0;double size=0;
        for(int i=Math.max(0,from);i+1<Math.min(to,tags.size());i+=2){
            if(code(tags.get(i))!=9)continue;String name=tags.get(i+1).trim();
            if("$PDMODE".equalsIgnoreCase(name)){
                for(int j=i+2;j+1<to;j+=2){int c=code(tags.get(j));if(c==9||c==0)break;if(c==70||c==90){mode=(int)number(tags.get(j+1),mode);break;}}
            }else if("$PDSIZE".equalsIgnoreCase(name)){
                for(int j=i+2;j+1<to;j+=2){int c=code(tags.get(j));if(c==9||c==0)break;if(c==40){size=number(tags.get(j+1),size);break;}}
            }
        }
        return new Style(mode,size);
    }

    /** Radius/half-size in device pixels. Positive PDSIZE is drawing units; zero is 5% of view; negative is percent of view. */
    public static double deviceHalfSize(Style style,double deviceScale,double viewPixels){
        if(style==null)style=new Style(0,0);double pixels;
        if(style.size>0)pixels=Math.abs(style.size)*Math.max(1e-9,Math.abs(deviceScale));
        else if(style.size<0)pixels=Math.abs(style.size)*.01*Math.max(1,viewPixels);
        else pixels=.05*Math.max(1,viewPixels);
        return Math.max(1,Math.min(500,pixels*.5));
    }

    private static int code(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double number(String s,double fallback){try{double v=Double.parseDouble(s.trim());return Double.isFinite(v)?v:fallback;}catch(Exception e){return fallback;}}
    private DxfPointStyle(){}
}
