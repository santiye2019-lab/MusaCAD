package com.musa.cad;

import java.util.ArrayList;

/** Pure geometry helpers for CAD snap candidates (no Android dependency). */
public final class DxfSnapGeometry {
    public static double[] line(double x1,double y1,double x2,double y2){
        return new double[]{x1,y1,x2,y2,(x1+x2)*.5,(y1+y2)*.5};
    }

    public static double[] poly(double[] xy,boolean closed){
        if(xy==null||xy.length<4||xy.length%2!=0)return xy==null?new double[0]:xy.clone();
        ArrayList<Double> out=new ArrayList<>();
        int n=xy.length/2;
        for(int i=0;i<n;i++)add(out,xy[i*2],xy[i*2+1]);
        for(int i=1;i<n;i++)mid(out,xy[(i-1)*2],xy[(i-1)*2+1],xy[i*2],xy[i*2+1]);
        if(closed&&n>2)mid(out,xy[(n-1)*2],xy[(n-1)*2+1],xy[0],xy[1]);
        return pack(out);
    }

    public static double[] circle(double cx,double cy,double radius,double startDeg,double sweepDeg){
        ArrayList<Double> out=new ArrayList<>();add(out,cx,cy);
        double r=Math.abs(radius);if(!(r>0)||!Double.isFinite(r))return pack(out);
        if(Math.abs(sweepDeg)>=359.999){
            for(int i=0;i<4;i++)polar(out,cx,cy,r,i*90d);
        }else{
            polar(out,cx,cy,r,startDeg);polar(out,cx,cy,r,startDeg+sweepDeg);
            polar(out,cx,cy,r,startDeg+sweepDeg*.5);
        }
        return pack(out);
    }

    public static double[] ellipse(double cx,double cy,double mx,double my,double ratio,double start,double end){
        ArrayList<Double> out=new ArrayList<>();add(out,cx,cy);
        if(!Double.isFinite(mx)||!Double.isFinite(my)||Math.hypot(mx,my)<1e-12)return pack(out);
        double sw=end-start;while(sw<=0)sw+=Math.PI*2;sw=Math.min(sw,Math.PI*2);
        if(sw>=Math.PI*2-1e-6){for(int i=0;i<4;i++)ellipseAt(out,cx,cy,mx,my,ratio,i*Math.PI/2);}
        else{ellipseAt(out,cx,cy,mx,my,ratio,start);ellipseAt(out,cx,cy,mx,my,ratio,start+sw);ellipseAt(out,cx,cy,mx,my,ratio,start+sw*.5);}
        return pack(out);
    }

    private static void mid(ArrayList<Double> out,double x1,double y1,double x2,double y2){
        if(Math.hypot(x2-x1,y2-y1)>1e-12)add(out,(x1+x2)*.5,(y1+y2)*.5);
    }
    private static void polar(ArrayList<Double> out,double cx,double cy,double r,double degrees){
        double q=Math.toRadians(degrees);add(out,cx+r*Math.cos(q),cy+r*Math.sin(q));
    }
    private static void ellipseAt(ArrayList<Double> out,double cx,double cy,double mx,double my,double ratio,double t){
        double co=Math.cos(t),si=Math.sin(t),rr=Math.abs(ratio);
        add(out,cx+mx*co-my*rr*si,cy+my*co+mx*rr*si);
    }
    private static void add(ArrayList<Double> out,double x,double y){if(Double.isFinite(x)&&Double.isFinite(y)){out.add(x);out.add(y);}}
    private static double[] pack(ArrayList<Double> values){double[] r=new double[values.size()];for(int i=0;i<r.length;i++)r[i]=values.get(i);return r;}
    private DxfSnapGeometry(){}
}
