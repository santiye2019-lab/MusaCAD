package com.musa.cad;

import java.util.List;
import java.util.Locale;

/** Pure DXF HATCH gradient metadata and geometry helpers. */
public final class DxfHatchGradient {
    public static final class Data {
        public final boolean enabled,oneColor;
        public final double rotation,centered,tint;
        public final int color1,color2;
        public final String name;
        Data(boolean enabled,boolean oneColor,double rotation,double centered,double tint,int color1,int color2,String name){
            this.enabled=enabled;this.oneColor=oneColor;this.rotation=finite(rotation)?rotation:0d;
            this.centered=clamp01(centered);this.tint=clamp01(tint);this.color1=color1&0x00ffffff;this.color2=color2&0x00ffffff;
            String n=name==null?"LINEAR":name.trim().toUpperCase(Locale.ROOT);this.name=n.isEmpty()?"LINEAR":n;
        }
        public boolean linear(){return enabled&&"LINEAR".equals(name);}
    }

    /** Parses alternating DXF code/value tags. Group 450=1 activates gradient data. */
    public static Data parse(List<String> tags,int from,int to){
        if(tags==null)return none();from=Math.max(0,from);to=Math.min(tags.size(),Math.max(from,to));
        int mode=-1,one=0,c1=-1,c2=-1,pending=-1;double rotation=0,centered=0,tint=0;String name="LINEAR";
        for(int i=from;i+1<to;i+=2){
            int code=integer(tags.get(i));String value=tags.get(i+1)==null?"":tags.get(i+1).trim();
            if(code==450)mode=integer(value);
            else if(code==452)one=integer(value);
            else if(code==460)rotation=number(value,0);
            else if(code==461)centered=number(value,0);
            else if(code==462)tint=number(value,0);
            else if(code==463)pending=integer(value);
            else if(code==421){int color=integer(value)&0x00ffffff;if(pending==0)c1=color;else if(pending==1)c2=color;pending=-1;}
            else if(code==470)name=value;
        }
        if(mode!=1)return none();
        if(c1<0)c1=0xffffff;
        boolean oneColor=one==1;
        if(oneColor){int gray=(int)Math.round(clamp01(tint)*255d);c2=(gray<<16)|(gray<<8)|gray;}
        else if(c2<0)c2=c1;
        return new Data(true,oneColor,rotation,centered,tint,c1,c2,name);
    }

    /** Gradient axis through the bounds center: x0,y0,x1,y1 in entity coordinates. */
    public static double[] axis(double left,double top,double right,double bottom,double radians){
        if(!finite(left,top,right,bottom,radians)||right<left||bottom<top)return new double[0];
        double cx=(left+right)*.5,cy=(top+bottom)*.5,dx=Math.cos(radians),dy=Math.sin(radians);
        double half=Math.abs(dx)*(right-left)*.5+Math.abs(dy)*(bottom-top)*.5;
        if(!(half>1e-12))half=.5;
        return new double[]{cx-dx*half,cy-dy*half,cx+dx*half,cy+dy*half};
    }

    public static Data none(){return new Data(false,false,0,0,0,0,0,"LINEAR");}
    private static int integer(String raw){try{return Integer.parseInt(raw.trim());}catch(Exception e){try{return (int)Double.parseDouble(raw.trim());}catch(Exception ignored){return -1;}}}
    private static double number(String raw,double fallback){try{double v=Double.parseDouble(raw.trim());return Double.isFinite(v)?v:fallback;}catch(Exception e){return fallback;}}
    private static double clamp01(double v){if(!Double.isFinite(v))return 0;return Math.max(0,Math.min(1,v));}
    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private DxfHatchGradient(){}
}
