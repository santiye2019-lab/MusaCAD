package com.musa.cad;

import android.graphics.Bitmap;
import android.graphics.PointF;
import android.graphics.RectF;

/** A raster image placed over the CAD canvas in content coordinates. */
public final class CadImageOverlay {
    public final Bitmap bitmap;
    public final String name;
    public final String uri;
    private float centerX,centerY,width,height,rotationDegrees;

    public CadImageOverlay(Bitmap bitmap,String name,String uri,float centerX,float centerY,float width,float height,float rotationDegrees){
        if(bitmap==null||bitmap.isRecycled())throw new IllegalArgumentException("bitmap");
        if(!Float.isFinite(centerX)||!Float.isFinite(centerY)||!Float.isFinite(width)||!Float.isFinite(height)||width<=0f||height<=0f)throw new IllegalArgumentException("geometry");
        this.bitmap=bitmap;
        this.name=name==null||name.trim().isEmpty()?"Görüntü":name.trim();
        this.uri=uri==null?"":uri;
        this.centerX=centerX;this.centerY=centerY;this.width=width;this.height=height;this.rotationDegrees=normalize(rotationDegrees);
    }

    public CadImageOverlay copy(){return new CadImageOverlay(bitmap,name,uri,centerX,centerY,width,height,rotationDegrees);}
    public float centerX(){return centerX;}
    public float centerY(){return centerY;}
    public float width(){return width;}
    public float height(){return height;}
    public float rotationDegrees(){return rotationDegrees;}

    public void moveTo(float x,float y){if(Float.isFinite(x)&&Float.isFinite(y)){centerX=x;centerY=y;}}
    public boolean scaleBy(float factor){
        if(!Float.isFinite(factor)||factor<=0f)return false;
        float nextW=width*factor,nextH=height*factor;
        if(!Float.isFinite(nextW)||!Float.isFinite(nextH)||nextW<1e-4f||nextH<1e-4f)return false;
        width=nextW;height=nextH;return true;
    }
    public void rotateBy(float degrees){if(Float.isFinite(degrees))rotationDegrees=normalize(rotationDegrees+degrees);}

    public boolean hit(float x,float y,float tolerance){
        if(!Float.isFinite(x)||!Float.isFinite(y))return false;
        double r=Math.toRadians(-rotationDegrees),co=Math.cos(r),si=Math.sin(r);
        float dx=x-centerX,dy=y-centerY;
        float localX=(float)(dx*co-dy*si),localY=(float)(dx*si+dy*co);
        float pad=Math.max(0f,tolerance);
        return Math.abs(localX)<=width*.5f+pad&&Math.abs(localY)<=height*.5f+pad;
    }

    public PointF[] corners(){
        float hw=width*.5f,hh=height*.5f;
        float[][] local={{-hw,-hh},{hw,-hh},{hw,hh},{-hw,hh}};
        PointF[] out=new PointF[4];double r=Math.toRadians(rotationDegrees),co=Math.cos(r),si=Math.sin(r);
        for(int i=0;i<4;i++){float x=local[i][0],y=local[i][1];out[i]=new PointF(centerX+(float)(x*co-y*si),centerY+(float)(x*si+y*co));}
        return out;
    }

    public RectF axisAlignedBounds(){
        PointF[] c=corners();float l=Float.POSITIVE_INFINITY,t=Float.POSITIVE_INFINITY,r=Float.NEGATIVE_INFINITY,b=Float.NEGATIVE_INFINITY;
        for(PointF p:c){l=Math.min(l,p.x);t=Math.min(t,p.y);r=Math.max(r,p.x);b=Math.max(b,p.y);}
        return new RectF(l,t,r,b);
    }

    private static float normalize(float degrees){
        float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;
    }
}
