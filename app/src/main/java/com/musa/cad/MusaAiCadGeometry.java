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

    /**
     * Returns two finite WCS LINE segments after a single-point BREAK:
     * [start.x,start.y,split.x,split.y,split.x,split.y,end.x,end.y].
     * The picked point is projected onto the line's supporting segment.
     * Rejects near-endpoint splits and float-collapsed endpoints before a
     * source edit can be committed. Does not model a two-point BREAK gap.
     */
    public static double[] breakLineAt(double[] target,double pickX,double pickY){
        if(!line(target)||!finite(pickX,pickY))return null;
        double dx=target[2]-target[0],dy=target[3]-target[1];
        double length=Math.hypot(dx,dy);
        if(!(length>1e-12)||!Double.isFinite(length))return null;
        double t=projection(pickX,pickY,target);
        if(!Double.isFinite(t)||t<=0.001d||t>=0.999d)return null;
        double x=target[0]+dx*t,y=target[1]+dy*t;
        if(!finite(x,y)||Math.hypot(x-target[0],y-target[1])<=1e-12||
                Math.hypot(x-target[2],y-target[3])<=1e-12)return null;
        // CadView edits currently serialize 2D source coordinates as floats;
        // reject cases where either half would collapse after this conversion.
        float fx=(float)x,fy=(float)y;
        if(!Float.isFinite(fx)||!Float.isFinite(fy)||
                (fx==(float)target[0]&&fy==(float)target[1])||
                (fx==(float)target[2]&&fy==(float)target[3]))return null;
        return new double[]{target[0],target[1],x,y,x,y,target[2],target[3]};
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

    /**
     * Conservative finite-segment 2D corner. Used for source LINE FILLET
     * (fillet=true, value=radius) and CHAMFER (value=symmetric distance).
     * Points are in source drawing coordinates, not screen pixels.
     *
     * Return [farA.xy, tangentA.xy, farB.xy, tangentB.xy, arcMid.xy].
     * For chamfer the last point is just the connector midpoint.
     * Only the picked sides of intersecting finite LINEs are supported:
     * external intersection extensions, arc/polyline corners, asymmetric
     * chamfers, tangent-only/no-trim options await separate acceptance.
     * Fail closed before the caller starts an edit transaction.
     */
    public static double[] cornerLines(double[] first,double[] second,
                                       double pickAx,double pickAy,
                                       double pickBx,double pickBy,
                                       double value,boolean fillet){
        if(!line(first)||!line(second)||
                !finite(pickAx,pickAy,pickBx,pickBy,value)||
                !(value>1e-12))return null;
        double[] hit=intersection(first,second);
        if(hit==null)return null;
        double ix=hit[0],iy=hit[1],ta=hit[2],tb=hit[3];
        // This first implementation trims two existing finite segments;
        // extending to reach a far-away theoretical corner is not supported.
        if(ta<0d||ta>1d||tb<0d||tb>1d)return null;
        double pa=projection(pickAx,pickAy,first);
        double pb=projection(pickBx,pickBy,second);
        if(!Double.isFinite(pa)||!Double.isFinite(pb)||
                Math.abs(pa-ta)<=1e-10||Math.abs(pb-tb)<=1e-10)return null;
        int ea=pa<ta?0:2,eb=pb<tb?0:2;
        double ax=first[ea],ay=first[ea+1];
        double bx=second[eb],by=second[eb+1];
        double la=Math.hypot(ax-ix,ay-iy),lb=Math.hypot(bx-ix,by-iy);
        if(!(la>1e-12)||!(lb>1e-12)||!Double.isFinite(la)||!Double.isFinite(lb))
            return null;
        double uax=(ax-ix)/la,uay=(ay-iy)/la;
        double ubx=(bx-ix)/lb,uby=(by-iy)/lb;
        double cosine=Math.max(-1d,Math.min(1d,uax*ubx+uay*uby));
        double theta=Math.acos(cosine);
        if(!(theta>Math.toRadians(1d))||!(theta<Math.toRadians(179d)))return null;
        double distance=fillet?value/Math.tan(theta/2d):value;
        if(!(distance>1e-12)||!Double.isFinite(distance))return null;
        // Never silently extend a source LINE or collapse a kept section.
        double remainA=la-distance,remainB=lb-distance;
        if(!(remainA>Math.max(1e-12,la*1e-10))||
                !(remainB>Math.max(1e-12,lb*1e-10)))return null;
        double tax=ix+uax*distance,tay=iy+uay*distance;
        double tbx=ix+ubx*distance,tby=iy+uby*distance;
        double mx=(tax+tbx)*0.5,my=(tay+tby)*0.5;
        if(fillet){
            double bdx=uax+ubx,bdy=uay+uby,bl=Math.hypot(bdx,bdy);
            double sin=Math.sin(theta/2d);
            if(!(bl>1e-12)||!(sin>1e-12))return null;
            double centerDistance=value/sin;
            double cx=ix+bdx/bl*centerDistance,cy=iy+bdy/bl*centerDistance;
            double back=Math.hypot(ix-cx,iy-cy);
            if(!(back>1e-12)||!Double.isFinite(back))return null;
            mx=cx+(ix-cx)/back*value;
            my=cy+(iy-cy)/back*value;
        }
        double[] result={ax,ay,tax,tay,bx,by,tbx,tby,mx,my};
        return finite(result)?result:null;
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
