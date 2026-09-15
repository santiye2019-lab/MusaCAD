package com.musa.cad;

/** Geometry-independent helpers for DXF geometric polyline widths. */
public final class DxfPolylineWidth {
    public static double constant(double raw){return Double.isFinite(raw)?Math.abs(raw):0d;}
    public static double device(double width,double axisScaleX,double axisScaleY){
        width=constant(width);if(width<=0)return 0;
        double sx=Math.abs(axisScaleX),sy=Math.abs(axisScaleY),scale=(sx+sy)*.5;
        if(!Double.isFinite(scale)||scale<=1e-12)return 0;
        return Math.max(.5,Math.min(10000d,width*scale));
    }
    private DxfPolylineWidth(){}
}
