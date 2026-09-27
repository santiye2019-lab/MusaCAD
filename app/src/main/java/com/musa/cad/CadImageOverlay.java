package com.musa.cad;

import java.util.Locale;

/** Immutable placed raster image in MusaCAD drawing/content coordinates. */
public final class CadImageOverlay {
    public final String key,name;
    public final float centerX,centerY,width,height,rotationDegrees;

    public CadImageOverlay(String key,String name,float centerX,float centerY,float width,float height,float rotationDegrees){
        String safeKey=key==null?"":key.trim();
        if(safeKey.isEmpty()||safeKey.indexOf('|')>=0)throw new IllegalArgumentException("image key");
        if(!finite(centerX)||!finite(centerY)||!finite(width)||!finite(height)||width<=0f||height<=0f)throw new IllegalArgumentException("image geometry");
        this.key=safeKey;this.name=name==null||name.trim().isEmpty()?safeKey:name.trim();
        this.centerX=centerX;this.centerY=centerY;this.width=width;this.height=height;this.rotationDegrees=normalize(rotationDegrees);
    }

    public CadImageOverlay copy(){return new CadImageOverlay(key,name,centerX,centerY,width,height,rotationDegrees);}
    public CadImageOverlay movedTo(float x,float y){return new CadImageOverlay(key,name,x,y,width,height,rotationDegrees);}
    public CadImageOverlay translated(float dx,float dy){return new CadImageOverlay(key,name,centerX+dx,centerY+dy,width,height,rotationDegrees);}
    public CadImageOverlay scaled(float factor){
        if(!finite(factor)||factor<=0f)throw new IllegalArgumentException("scale");
        return new CadImageOverlay(key,name,centerX,centerY,width*factor,height*factor,rotationDegrees);
    }
    public CadImageOverlay rotated(float degrees){return new CadImageOverlay(key,name,centerX,centerY,width,height,rotationDegrees+degrees);}

    /** Hit-test in the image's local rotated rectangle. */
    public boolean contains(float x,float y,float tolerance){
        double r=Math.toRadians(-rotationDegrees),co=Math.cos(r),si=Math.sin(r);
        float dx=x-centerX,dy=y-centerY;
        double lx=dx*co-dy*si,ly=dx*si+dy*co;
        float t=Math.max(0f,tolerance);
        return Math.abs(lx)<=width*.5f+t&&Math.abs(ly)<=height*.5f+t;
    }

    public float[] corners(){
        float[] out=new float[8];double r=Math.toRadians(rotationDegrees),co=Math.cos(r),si=Math.sin(r);
        float hw=width*.5f,hh=height*.5f;float[] local={-hw,-hh, hw,-hh, hw,hh, -hw,hh};
        for(int i=0;i<8;i+=2){out[i]=(float)(centerX+local[i]*co-local[i+1]*si);out[i+1]=(float)(centerY+local[i]*si+local[i+1]*co);}
        return out;
    }

    public String summary(){
        return String.format(Locale.getDefault(),"%s • %.3f × %.3f • %.1f°",name,width,height,rotationDegrees);
    }

    private static boolean finite(float v){return Float.isFinite(v)&&Math.abs(v)<=1e9f;}
    private static float normalize(float degrees){if(!Float.isFinite(degrees))return 0f;float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
}
