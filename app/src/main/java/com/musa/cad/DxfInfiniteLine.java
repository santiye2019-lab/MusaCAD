package com.musa.cad;

/** Pure clipping helpers for DXF RAY and XLINE infinite entities. */
public final class DxfInfiniteLine {
    /**
     * Clips p(t)=origin+t*direction to an axis-aligned rectangle. For a RAY t>=0;
     * for an XLINE t is unbounded in both directions. Returns two endpoints or [].
     */
    public static double[] clip(double x,double y,double dx,double dy,boolean ray,
                                double left,double top,double right,double bottom){
        if(!finite(x,y,dx,dy,left,top,right,bottom))return new double[0];
        if(right<left){double q=left;left=right;right=q;}if(bottom<top){double q=top;top=bottom;bottom=q;}
        double norm=Math.hypot(dx,dy);if(norm<1e-12)return new double[0];dx/=norm;dy/=norm;
        double tMin=ray?0d:Double.NEGATIVE_INFINITY,tMax=Double.POSITIVE_INFINITY;
        double[] r=slab(x,dx,left,right,tMin,tMax);if(r==null)return new double[0];tMin=r[0];tMax=r[1];
        r=slab(y,dy,top,bottom,tMin,tMax);if(r==null)return new double[0];tMin=r[0];tMax=r[1];
        if(!Double.isFinite(tMin)||!Double.isFinite(tMax)||tMax<tMin)return new double[0];
        return new double[]{x+dx*tMin,y+dy*tMin,x+dx*tMax,y+dy*tMax};
    }

    private static double[] slab(double p,double d,double lo,double hi,double tMin,double tMax){
        if(Math.abs(d)<1e-12)return p>=lo&&p<=hi?new double[]{tMin,tMax}:null;
        double a=(lo-p)/d,b=(hi-p)/d;if(a>b){double q=a;a=b;b=q;}
        tMin=Math.max(tMin,a);tMax=Math.min(tMax,b);return tMax>=tMin?new double[]{tMin,tMax}:null;
    }
    private static boolean finite(double... v){for(double q:v)if(!Double.isFinite(q))return false;return true;}
    private DxfInfiniteLine(){}
}
