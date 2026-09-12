package com.musa.cad;

/** Pixel bounds, independent of Android so edge cases can run on the JVM. */
public final class SelectionBounds {
    public final int left, top, width, height;
    private SelectionBounds(int l,int t,int w,int h){left=l;top=t;width=w;height=h;}
    public static SelectionBounds clip(float x1,float y1,float x2,float y2,int width,int height,int minimum){
        if(width<=0||height<=0||minimum<=0||!Float.isFinite(x1)||!Float.isFinite(y1)||!Float.isFinite(x2)||!Float.isFinite(y2))return null;
        int l=(int)Math.floor(Math.max(0,Math.min(width,Math.min(x1,x2))));
        int t=(int)Math.floor(Math.max(0,Math.min(height,Math.min(y1,y2))));
        int r=(int)Math.ceil(Math.max(0,Math.min(width,Math.max(x1,x2))));
        int b=(int)Math.ceil(Math.max(0,Math.min(height,Math.max(y1,y2))));
        if(r-l<minimum||b-t<minimum)return null;
        return new SelectionBounds(l,t,r-l,b-t);
    }
}
