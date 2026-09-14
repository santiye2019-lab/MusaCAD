package com.musa.cad;

import java.util.ArrayList;
import java.util.List;

/** Pure geometry helper for IMAGE/WIPEOUT clipping boundaries. */
public final class DxfWipeout {
    /**
     * Convert raster-space clipping vertices to drawing coordinates.
     * DXF IMAGE/WIPEOUT clip coordinates are pixel coordinates relative to (-0.5,-0.5).
     */
    public static double[] boundary(double ix,double iy,double ux,double uy,double vx,double vy,
                                    double width,double height,List<double[]> clip){
        ArrayList<double[]> points=new ArrayList<>();
        if(clip!=null)for(double[] p:clip)if(p!=null&&p.length>=2&&Double.isFinite(p[0])&&Double.isFinite(p[1]))points.add(p);
        if(points.size()<3){
            points.clear();
            double w=Math.max(1d,Math.abs(width)),h=Math.max(1d,Math.abs(height));
            points.add(new double[]{-.5,-.5});points.add(new double[]{w-.5,-.5});
            points.add(new double[]{w-.5,h-.5});points.add(new double[]{-.5,h-.5});
        }
        double[] out=new double[points.size()*2];
        for(int i=0;i<points.size();i++){
            double px=points.get(i)[0]+.5,py=points.get(i)[1]+.5;
            out[i*2]=ix+px*ux+py*vx;out[i*2+1]=iy+px*uy+py*vy;
        }
        return out;
    }
    private DxfWipeout(){}
}
