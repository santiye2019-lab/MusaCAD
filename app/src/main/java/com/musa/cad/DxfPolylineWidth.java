package com.musa.cad;

/** Geometry-independent helpers for DXF geometric polyline widths. */
public final class DxfPolylineWidth {
    public static double constant(double raw){return Double.isFinite(raw)?Math.abs(raw):0d;}

    /** Returns a constant width only when start/end widths describe the same geometric width. */
    public static double uniform(double start,double end){
        double a=constant(start),b=constant(end);
        if(a<=1e-12&&b<=1e-12)return 0d;
        if(a<=1e-12||b<=1e-12)return 0d;
        double tolerance=Math.max(1e-9,Math.max(a,b)*1e-6);
        return Math.abs(a-b)<=tolerance?(a+b)*.5:0d;
    }

    public static double device(double width,double axisScaleX,double axisScaleY){
        width=constant(width);if(width<=0)return 0;
        double sx=Math.abs(axisScaleX),sy=Math.abs(axisScaleY),scale=(sx+sy)*.5;
        if(!Double.isFinite(scale)||scale<=1e-12)return 0;
        return Math.max(.5,Math.min(10000d,width*scale));
    }
    private DxfPolylineWidth(){}
}
