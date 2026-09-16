package com.musa.cad;

/** Pure display semantics shared by buffered and streaming MPOLYGON rendering. */
public final class DxfMPolygon {
    public static boolean solid(int solidFillFlag){return solidFillFlag!=0;}

    /** MPOLYGON group 63 overrides only the solid fill color; boundary color stays the entity color. */
    public static int solidFillArgb(int aci,int entityArgb){
        if(aci<1||aci>255)return entityArgb;
        int rgb=DxfColor.aciArgb(aci)&0x00ffffff;
        return (entityArgb&0xff000000)|rgb;
    }

    /** Applies the MPOLYGON boundary offset vector in OCS. */
    public static double[] offset(double[] xy,double dx,double dy){
        if(xy==null)return new double[0];double[] out=xy.clone();
        if(!Double.isFinite(dx)||!Double.isFinite(dy))return out;
        for(int i=0;i+1<out.length;i+=2){if(Double.isFinite(out[i])&&Double.isFinite(out[i+1])){out[i]+=dx;out[i+1]+=dy;}}
        return out;
    }
    private DxfMPolygon(){}
}
