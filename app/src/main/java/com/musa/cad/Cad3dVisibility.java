package com.musa.cad;

/**
 * Conservative view-space visibility checks for the software 3D renderer.
 * Objects are skipped only when their full bounding box is outside.
 * A crossing triangle or line remains visible even with offscreen vertices.
 */
public final class Cad3dVisibility {
    private Cad3dVisibility(){}

    public static boolean edgeVisible(float x1,float y1,float x2,float y2,
                                      float width,float height,float margin) {
        if(!Float.isFinite(x1)||!Float.isFinite(y1)||!Float.isFinite(x2)||!Float.isFinite(y2))
            return false;
        return Math.max(x1,x2)>=-margin && Math.min(x1,x2)<=width+margin
            && Math.max(y1,y2)>=-margin && Math.min(y1,y2)<=height+margin;
    }

    public static boolean triangleVisible(float x1,float y1,float x2,float y2,
                                          float x3,float y3,
                                          float width,float height,float margin) {
        if(!Float.isFinite(x1)||!Float.isFinite(y1)||!Float.isFinite(x2)||!Float.isFinite(y2)
            ||!Float.isFinite(x3)||!Float.isFinite(y3))return false;
        return Math.max(x1,Math.max(x2,x3))>=-margin
            && Math.min(x1,Math.min(x2,x3))<=width+margin
            && Math.max(y1,Math.max(y2,y3))>=-margin
            && Math.min(y1,Math.min(y2,y3))<=height+margin;
    }
}
