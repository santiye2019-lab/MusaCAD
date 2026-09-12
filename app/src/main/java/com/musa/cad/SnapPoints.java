package com.musa.cad;

/** Candidates use drawing-image coordinates; the hit radius is in screen pixels. */
public final class SnapPoints {
    public static int nearest(float[] points,float x,float y,float scale,float radius){
        if(points==null||points.length%2!=0||!Float.isFinite(x)||!Float.isFinite(y)||
            !Float.isFinite(scale)||scale<=0||!Float.isFinite(radius)||radius<=0)return -1;
        double limit=(double)radius/scale;
        double best=limit*limit;
        int found=-1;
        for(int i=0;i<points.length;i+=2){
            if(!Float.isFinite(points[i])||!Float.isFinite(points[i+1]))continue;
            double dx=(double)points[i]-x,dy=(double)points[i+1]-y,d=dx*dx+dy*dy;
            if(d<=best&&(found<0||d<best)){found=i;best=d;}
        }
        return found;
    }
    private SnapPoints(){}
}
