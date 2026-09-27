package com.musa.cad;

/** Serializable raster placement stored in drawing/world coordinates. */
public final class CadImagePlacement {
    public final String uri,name;
    public final float centerX,centerY,width,height,rotationDegrees;
    public CadImagePlacement(String uri,String name,float centerX,float centerY,float width,float height,float rotationDegrees){
        this.uri=uri==null?"":uri.trim();this.name=name==null?"Görüntü":name.trim();
        if(this.uri.isEmpty())throw new IllegalArgumentException("uri");
        if(!Float.isFinite(centerX)||!Float.isFinite(centerY)||!Float.isFinite(width)||!Float.isFinite(height)||width<=0f||height<=0f)throw new IllegalArgumentException("geometry");
        this.centerX=centerX;this.centerY=centerY;this.width=width;this.height=height;this.rotationDegrees=normalize(rotationDegrees);
    }
    public CadImagePlacement copy(){return new CadImagePlacement(uri,name,centerX,centerY,width,height,rotationDegrees);}
    private static float normalize(float value){float v=Float.isFinite(value)?value%360f:0f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
}
