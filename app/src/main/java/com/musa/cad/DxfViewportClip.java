package com.musa.cad;

import java.util.ArrayList;

/** Pure geometry helpers for non-rectangular paper-space VIEWPORT clipping. */
public final class DxfViewportClip {
    private static final double EPS=1e-9;

    /**
     * Sanitizes packed paper-space polygon x/y pairs. Consecutive duplicates and an
     * explicit duplicate closing point are removed. Invalid/degenerate input becomes empty.
     */
    public static double[] polygon(double[] packed){
        if(packed==null||packed.length<6)return new double[0];
        ArrayList<Double> out=new ArrayList<>();double px=Double.NaN,py=Double.NaN;
        for(int i=0;i+1<packed.length;i+=2){
            double x=packed[i],y=packed[i+1];if(!Double.isFinite(x)||!Double.isFinite(y))return new double[0];
            if(!out.isEmpty()&&Math.hypot(x-px,y-py)<=EPS)continue;
            out.add(x);out.add(y);px=x;py=y;
        }
        if(out.size()>=8){double x0=out.get(0),y0=out.get(1),xn=out.get(out.size()-2),yn=out.get(out.size()-1);
            if(Math.hypot(xn-x0,yn-y0)<=EPS){out.remove(out.size()-1);out.remove(out.size()-1);}}
        if(out.size()<6)return new double[0];
        double area=0;int n=out.size()/2;
        for(int i=0;i<n;i++){int j=(i+1)%n;area+=out.get(i*2)*out.get(j*2+1)-out.get(j*2)*out.get(i*2+1);}
        if(Math.abs(area)<=EPS)return new double[0];
        double[] result=new double[out.size()];for(int i=0;i<result.length;i++)result[i]=out.get(i);return result;
    }

    /** Point-in-polygon with edge tolerance in paper-space drawing units. */
    public static boolean contains(double[] raw,double x,double y,double tolerance){
        double[] p=polygon(raw);if(p.length<6||!Double.isFinite(x)||!Double.isFinite(y))return false;
        double tol=Math.max(0,Double.isFinite(tolerance)?tolerance:0);int n=p.length/2;
        for(int i=0;i<n;i++){int j=(i+1)%n;if(distanceToSegment(x,y,p[i*2],p[i*2+1],p[j*2],p[j*2+1])<=tol)return true;}
        boolean inside=false;
        for(int i=0,j=n-1;i<n;j=i++){
            double xi=p[i*2],yi=p[i*2+1],xj=p[j*2],yj=p[j*2+1];
            boolean crosses=((yi>y)!=(yj>y))&&x<(xj-xi)*(y-yi)/(yj-yi)+xi;
            if(crosses)inside=!inside;
        }
        return inside;
    }

    /** Returns [left,bottom,right,top], or an empty array for invalid polygons. */
    public static double[] bounds(double[] raw){
        double[] p=polygon(raw);if(p.length<6)return new double[0];
        double l=Double.POSITIVE_INFINITY,b=Double.POSITIVE_INFINITY,r=Double.NEGATIVE_INFINITY,t=Double.NEGATIVE_INFINITY;
        for(int i=0;i<p.length;i+=2){l=Math.min(l,p[i]);r=Math.max(r,p[i]);b=Math.min(b,p[i+1]);t=Math.max(t,p[i+1]);}
        return new double[]{l,b,r,t};
    }

    private static double distanceToSegment(double x,double y,double x1,double y1,double x2,double y2){
        double dx=x2-x1,dy=y2-y1,d2=dx*dx+dy*dy;if(d2<=EPS*EPS)return Math.hypot(x-x1,y-y1);
        double u=((x-x1)*dx+(y-y1)*dy)/d2;u=Math.max(0,Math.min(1,u));
        return Math.hypot(x-(x1+u*dx),y-(y1+u*dy));
    }

    private DxfViewportClip(){}
}
