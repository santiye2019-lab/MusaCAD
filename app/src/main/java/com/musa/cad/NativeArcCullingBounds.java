package com.musa.cad;

/**
 * Android-only VIEWPORT CULLING acceleration for native v6 ARC packets.
 * This does not decode CAD geometry, certify layers, or change the C11 CAD
 * engine: it computes conservative projected bounds of already-decoded
 * packet coordinates, using affine ellipse:
 *    x(t)=cx+ux*cos(t)+vx*sin(t), y(t)=cy+uy*cos(t)+vy*sin(t).
 *
 * The raw binary AC1032 geometry remains the sole responsibility of the
 * shared independent C11 core (Android / iOS / Windows). This helper does
 * not affect actual arc drawing or project storage.
 */
final class NativeArcCullingBounds {
    private static final double TAU=2.0*Math.PI;
    private static final double MEMBERSHIP_TOLERANCE=1e-6;
    private NativeArcCullingBounds() {}

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
    static boolean compute(float cx,float cy,float ux,float uy,
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
