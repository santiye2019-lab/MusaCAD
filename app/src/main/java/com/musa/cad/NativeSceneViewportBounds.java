package com.musa.cad;

/**
 * Validated viewport framing for a native v6 drawing's original XY bounds.
 * A real DWG with a single POINT, or exclusively vertical/horizontal LINE,
 * has a zero-width or zero-height geometric extent. This is VALID input:
 * pad ONLY the CAMERA bounds, never the source geometry/coordinates.
 *
 * Pure Java used by Android and checked by ordinary host JVM tests.
 */
public final class NativeSceneViewportBounds {
    private NativeSceneViewportBounds() {}

    /**
     * [minX,minY,maxX,maxY] for the Android viewport or null when invalid.
     * Original source XY bounds are never modified. No unbounded allocations.
     */
    public static float[] forScene(float minX,float minY,float maxX,float maxY) {
        if (!Float.isFinite(minX) || !Float.isFinite(minY) ||
            !Float.isFinite(maxX) || !Float.isFinite(maxY) ||
            maxX < minX || maxY < minY)
            return null;
        if (minX == maxX) {
            final float padding=padding(minX,maxY-minY);
            minX -= padding;
            maxX += padding;
        }
        if (minY == maxY) {
            final float padding=padding(minY,maxX-minX);
            minY -= padding;
            maxY += padding;
        }
        if (!Float.isFinite(minX) || !Float.isFinite(minY) ||
            !Float.isFinite(maxX) || !Float.isFinite(maxY) ||
            maxX <= minX || maxY <= minY)
            return null;
        return new float[]{minX,minY,maxX,maxY};
    }

    private static float padding(float center,float otherSpan) {
        /* Use a reproducible minimum screen framing of 1 drawing unit,
         * expand if the coordinate magnitude's float ULP is enormous. */
        final double pad=Math.max(1.0,
               Math.max((double)Math.ulp(center)*16.0,
                        (double)otherSpan*0.001));
        return pad<=Float.MAX_VALUE?(float)pad:Float.POSITIVE_INFINITY;
    }
    /* ARC/elliptical v6 packet screen-culling, not CAD geometry decoding.
     * On failure the Android renderer keeps whole-ellipse safe bounds. */
    private static final double TAU=2.0*Math.PI;
    private static final double MEMBERSHIP_TOLERANCE=1e-6;

    private static boolean contains(double start,double sweep,double t){
        double delta=(sweep>0.0?t-start:start-t)%TAU;
        if(delta<0.0)delta+=TAU;
        return delta<=Math.abs(sweep)+MEMBERSHIP_TOLERANCE;
    }
    private static double x(double cx,double ux,double vx,double t){
        return cx+ux*Math.cos(t)+vx*Math.sin(t);
    }
    private static double y(double cy,double uy,double vy,double t){
        return cy+uy*Math.cos(t)+vy*Math.sin(t);
    }

    /**
     * Conservatively bounds a native v6 ARC in source/world XY.
     * Returns false on invalid or nonrepresentable data WITHOUT changing out.
     * Reuses caller-owned 4-float scratch; no allocations in the arc loop.
     */
    public static boolean arcCullingBounds(float cx,float cy,float ux,float uy,
                           float vx,float vy,float start,float sweep,
                           float[] out){
        if(out==null||out.length<4||
           !Float.isFinite(cx)||!Float.isFinite(cy)||
           !Float.isFinite(ux)||!Float.isFinite(uy)||
           !Float.isFinite(vx)||!Float.isFinite(vy)||
           !Float.isFinite(start)||!Float.isFinite(sweep)||
           sweep==0f)return false;
        final double abs=Math.abs((double)sweep);
        /* Unknown multi-turn form: a complete ellipse is a SAFE culling
         * enclosure. Do not silently clip any sub-arc. */
        final double left,right,top,bottom;
        if(abs>=TAU-1e-6){
            double rx=Math.hypot((double)ux,(double)vx);
            double ry=Math.hypot((double)uy,(double)vy);
            left=(double)cx-rx;right=(double)cx+rx;
            top=(double)cy-ry;bottom=(double)cy+ry;
        }else{
            final double s=(double)start, e=s+(double)sweep;
            double loX=Math.min(x(cx,ux,vx,s),x(cx,ux,vx,e));
            double hiX=Math.max(x(cx,ux,vx,s),x(cx,ux,vx,e));
            double loY=Math.min(y(cy,uy,vy,s),y(cy,uy,vy,e));
            double hiY=Math.max(y(cy,uy,vy,s),y(cy,uy,vy,e));
            final double xPeak=Math.atan2((double)vx,(double)ux);
            final double yPeak=Math.atan2((double)vy,(double)uy);
            for(int i=0;i<2;i++){
                final double tx=xPeak+i*Math.PI;
                if(contains(s,sweep,tx)){
                    double candidate=x(cx,ux,vx,tx);
                    loX=Math.min(loX,candidate);
                    hiX=Math.max(hiX,candidate);
                }
                final double ty=yPeak+i*Math.PI;
                if(contains(s,sweep,ty)){
                    double candidate=y(cy,uy,vy,ty);
                    loY=Math.min(loY,candidate);
                    hiY=Math.max(hiY,candidate);
                }
            }
            left=loX;right=hiX;top=loY;bottom=hiY;
        }
        if(!Double.isFinite(left)||!Double.isFinite(right)||
           !Double.isFinite(top)||!Double.isFinite(bottom)||
           left>right||top>bottom||
           left< -Float.MAX_VALUE||top< -Float.MAX_VALUE||
           right>Float.MAX_VALUE||bottom>Float.MAX_VALUE)return false;
        /* Expand once in float32 space: interpolation, trig evaluation and
         * raster AA must never make the grid hide a source-valid arc. */
        float a=Math.nextAfter((float)left,Double.NEGATIVE_INFINITY);
        float b=Math.nextAfter((float)top,Double.NEGATIVE_INFINITY);
        float c=Math.nextAfter((float)right,Double.POSITIVE_INFINITY);
        float d=Math.nextAfter((float)bottom,Double.POSITIVE_INFINITY);
        if(!Float.isFinite(a)||!Float.isFinite(b)||
           !Float.isFinite(c)||!Float.isFinite(d))return false;
        out[0]=a;out[1]=b;out[2]=c;out[3]=d;
        return true;
    }
}
