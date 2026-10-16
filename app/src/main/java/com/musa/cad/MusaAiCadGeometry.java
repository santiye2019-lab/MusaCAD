package com.musa.cad;

import java.util.*;

/** Pure deterministic geometry used by approved Gandalf CAD edits. */
public final class MusaAiCadGeometry {
    private static final double EPS=1e-7;

    public static double[] trimLine(double[] target,double[] boundary,double pickX,double pickY){
        if(!line(target)||!line(boundary)||!finite(pickX,pickY))return null;
        double[] hit=intersection(target,boundary);
        if(hit==null)return null;
        double ix=hit[0],iy=hit[1],t=hit[2],u=hit[3];
        if(t<=1e-4||t>=.9999||u<-1e-4||u>1.0001)return null;
        double pickT=projection(pickX,pickY,target);
        if(pickT<t)return new double[]{ix,iy,target[2],target[3]};
        return new double[]{target[0],target[1],ix,iy};
    }

    public static double[] extendLine(double[] target,double[] boundary){
        if(!line(target)||!line(boundary))return null;
        double[] hit=intersection(target,boundary);
        if(hit==null)return null;
        double ix=hit[0],iy=hit[1],t=hit[2],u=hit[3];
        if(u<-1e-4||u>1.0001)return null;
        if(t<0d)return new double[]{ix,iy,target[2],target[3]};
        if(t>1d)return new double[]{target[0],target[1],ix,iy};
        return null;
    }

    /**
     * Extends an open LINE/POLYLINE from one endpoint. newPoints are ordered
     * outward from the chosen existing endpoint and do not need to repeat it.
     */
    public static double[] continuePath(double[] existing,boolean fromStart,double[] newPoints){
        if(!path(existing)||!pathPoints(newPoints))return null;
        ArrayList<Double> out=new ArrayList<>();
        if(fromStart){
            double ex=existing[0],ey=existing[1];
            for(int i=newPoints.length-2;i>=0;i-=2){
                double x=newPoints[i],y=newPoints[i+1];
                if(i==0&&same(x,y,ex,ey))continue;
                append(out,x,y);
            }
            for(int i=0;i<existing.length;i+=2)append(out,existing[i],existing[i+1]);
        }else{
            for(int i=0;i<existing.length;i+=2)append(out,existing[i],existing[i+1]);
            double ex=existing[existing.length-2],ey=existing[existing.length-1];
            for(int i=0;i<newPoints.length;i+=2){
                double x=newPoints[i],y=newPoints[i+1];
                if(i==0&&same(x,y,ex,ey))continue;
                append(out,x,y);
            }
        }
        if(out.size()<4)return null;
        double[] result=new double[out.size()];
        for(int i=0;i<out.size();i++)result[i]=out.get(i);
        return result;
    }

    public static boolean validPolyline(double[] xy,boolean closed){
        if(!path(xy))return false;
        int points=xy.length/2;
        return closed?points>=3:points>=2;
    }

    /** Shared tested intersection for ordinary on-screen and AI source edits. */
    public static double[] intersectLines(double[] target,double[] boundary){
        return line(target)&&line(boundary)?intersection(target,boundary):null;
    }

    /** Relative picked-point fraction on a WCS segment; NaN for invalid lines. */
    public static double projectOnLine(double px,double py,double[] target){
        if(!line(target)||!finite(px,py))return Double.NaN;
        return projection(px,py,target);
    }

    private static double[] intersection(double[] a,double[] b){
        // Match musacad_line2.h's WCS supporting-line kernel. Comparing the
        // unnormalized cross product to a fixed 1e-7 incorrectly rejects
        // small (e.g. 0.1 mm) perpendicular source DWG lines.
        double x1=a[0],y1=a[1],x2=a[2],y2=a[3],x3=b[0],y3=b[1],x4=b[2],y4=b[3];
        double rx=x2-x1,ry=y2-y1,sx=x4-x3,sy=y4-y3;
        double lengthA=Math.hypot(rx,ry),lengthB=Math.hypot(sx,sy);
        if(!(lengthA>1e-12)||!(lengthB>1e-12)||
                !Double.isFinite(lengthA)||!Double.isFinite(lengthB))return null;
        double ux=rx/lengthA,uy=ry/lengthA,vx=sx/lengthB,vy=sy/lengthB;
        double den=cross(ux,uy,vx,vy);
        if(!Double.isFinite(den)||Math.abs(den)<=1e-12)return null;
        double qx=x3-x1,qy=y3-y1;
        double da=cross(qx,qy,vx,vy)/den;
        double db=cross(qx,qy,ux,uy)/den;
        double ix=x1+da*ux,iy=y1+da*uy,t=da/lengthA,u=db/lengthB;
        // Reject impossible out-of-budget intersections instead of passing
        // infinity or falsely clipping to a source endpoint.
        return finite(ix,iy,t,u)?new double[]{ix,iy,t,u}:null;
    }

    private static double projection(double x,double y,double[] line){
        // A fixed absolute length-squared EPS makes every segment shorter
        // than sqrt(1e-7) project to zero, reversing short-line TRIM picks.
        double dx=line[2]-line[0],dy=line[3]-line[1];
        double len=Math.hypot(dx,dy);
        if(!(len>1e-12)||!Double.isFinite(len))return Double.NaN;
        return ((x-line[0])*(dx/len)+(y-line[1])*(dy/len))/len;
    }

    private static double cross(double ax,double ay,double bx,double by){return ax*by-ay*bx;}
    private static boolean line(double[] v){return v!=null&&v.length==4&&finite(v);}
    private static boolean path(double[] v){return v!=null&&v.length>=4&&v.length%2==0&&finite(v);}
    private static boolean pathPoints(double[] v){return v!=null&&v.length>=2&&v.length%2==0&&finite(v);}
    private static boolean finite(double...v){if(v==null)return false;for(double x:v)if(!Double.isFinite(x)||Math.abs(x)>1e12)return false;return true;}
    private static boolean same(double x1,double y1,double x2,double y2){return Math.hypot(x1-x2,y1-y2)<=EPS;}
    private static void append(List<Double>out,double x,double y){
        int n=out.size();
        if(n>=2&&same(out.get(n-2),out.get(n-1),x,y))return;
        out.add(x);out.add(y);
    }

    private MusaAiCadGeometry(){}
}
