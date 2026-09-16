package com.musa.cad;

/** Pure MTEXT background-mask semantics shared by buffered and streaming DXF rendering. */
public final class DxfMTextBackground {
    public static boolean enabled(int flags){return (flags&3)!=0;}
    public static boolean useDrawingBackground(int flags){return (flags&2)!=0;}

    /** AutoCAD normally writes 1.5; reject corrupt/extreme values without hiding the text. */
    public static double boxScale(double raw){
        if(!Double.isFinite(raw)||raw<1d)return 1.5d;
        return Math.min(raw,10d);
    }

    /** Extra border on each side in drawing units. */
    public static double padding(double textHeight,double scale){
        if(!Double.isFinite(textHeight)||textHeight<=0)return 0d;
        scale=boxScale(scale);return Math.max(0d,textHeight*(scale-1d)*.5d);
    }

    /** Resolve MTEXT group 90/63/421 fill color, falling back to the drawing background. */
    public static int color(int flags,int aci,long trueColorRaw,int drawingBackground){
        if(useDrawingBackground(flags))return drawingBackground;
        if(trueColorRaw>=0)return 0xff000000|DxfColor.trueColor(trueColorRaw);
        if(aci>=1&&aci<=255)return DxfColor.aciArgb(aci);
        return drawingBackground;
    }

    private DxfMTextBackground(){}
}
