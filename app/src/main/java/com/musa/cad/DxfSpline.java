package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Pure-Java DXF SPLINE parser/evaluator. Uses rational de Boor when control/knot data is available. */
public final class DxfSpline {
    public static final int FLAG_CLOSED=1,FLAG_PERIODIC=2,FLAG_RATIONAL=4,FLAG_LINEAR=16;
    public static final class Point {
        public final double x,y;
        public Point(double x,double y){this.x=x;this.y=y;}
    }
    public static final class Result {
        public final List<Point> points,controlPoints,fitPoints;public final int flags,degree;public final boolean exactNurbs;
        Result(List<Point>points,List<Point>controls,List<Point>fits,int flags,int degree,boolean exact){this.points=Collections.unmodifiableList(points);controlPoints=Collections.unmodifiableList(controls);fitPoints=Collections.unmodifiableList(fits);this.flags=flags;this.degree=degree;exactNurbs=exact;}
        public boolean closed(){return (flags&FLAG_CLOSED)!=0;}
    }

    public static Result parse(List<String>a,int from,int to)throws IOException{
        int flags=(int)number(a,from,to,70,0),degree=Math.max(1,(int)number(a,from,to,71,3));
        List<Point>controls=points(a,from,to,10,20),fits=points(a,from,to,11,21);List<Double>knots=values(a,from,to,40),weights=values(a,from,to,41);
        boolean linear=(flags&FLAG_LINEAR)!=0;
        if(!linear&&controls.size()>=degree+1&&knots.size()>=controls.size()+degree+1){
            ArrayList<Point>sampled=sampleNurbs(controls,weights,knots,degree,(flags&FLAG_CLOSED)!=0);if(sampled.size()>=2)return new Result(sampled,new ArrayList<>(controls),new ArrayList<>(fits),flags,degree,true);
        }
        List<Point>seed=fits.size()>=2?fits:controls;ArrayList<Point>fallback=linear?new ArrayList<>(seed):catmullRom(seed,(flags&FLAG_CLOSED)!=0);
        return new Result(fallback,new ArrayList<>(controls),new ArrayList<>(fits),flags,degree,false);
    }

    public static Point evaluate(List<Point>controls,List<Double>weights,List<Double>knots,int degree,double u){
        if(controls==null||knots==null||controls.size()<degree+1||knots.size()<controls.size()+degree+1)return null;
        int n=controls.size()-1,p=degree,span=findSpan(n,p,u,knots);double[][]d=new double[p+1][3];
        for(int j=0;j<=p;j++){int index=span-p+j;double w=weights!=null&&weights.size()==controls.size()?weights.get(index):1d;if(!Double.isFinite(w)||Math.abs(w)<1e-12)w=1d;Point q=controls.get(index);d[j][0]=q.x*w;d[j][1]=q.y*w;d[j][2]=w;}
        for(int r=1;r<=p;r++)for(int j=p;j>=r;j--){int i=span-p+j;double den=knots.get(i+p-r+1)-knots.get(i);double alpha=Math.abs(den)<1e-15?0d:(u-knots.get(i))/den;alpha=Math.max(0d,Math.min(1d,alpha));for(int k=0;k<3;k++)d[j][k]=(1d-alpha)*d[j-1][k]+alpha*d[j][k];}
        double w=d[p][2];if(Math.abs(w)<1e-15||!Double.isFinite(w))return null;return new Point(d[p][0]/w,d[p][1]/w);
    }

    private static ArrayList<Point> sampleNurbs(List<Point>controls,List<Double>weights,List<Double>knots,int degree,boolean closed){
        ArrayList<Point>out=new ArrayList<>();int n=controls.size()-1;double start=knots.get(degree),end=knots.get(n+1);if(!Double.isFinite(start)||!Double.isFinite(end)||end<=start)return out;
        int steps=Math.max(96,Math.min(1024,controls.size()*32));for(int i=0;i<=steps;i++){double u=i==steps?Math.nextAfter(end,start):start+(end-start)*i/steps;Point p=evaluate(controls,weights,knots,degree,u);append(out,p);}
        if(!closed){Point endPoint=evaluate(controls,weights,knots,degree,end);if(endPoint!=null){if(!out.isEmpty())out.remove(out.size()-1);append(out,endPoint);}}else if(!out.isEmpty())append(out,new Point(out.get(0).x,out.get(0).y));return out;
    }

    private static int findSpan(int n,int p,double u,List<Double>U){if(u>=U.get(n+1))return n;if(u<=U.get(p))return p;int low=p,high=n+1,mid=(low+high)/2;while(u<U.get(mid)||u>=U.get(mid+1)){if(u<U.get(mid))high=mid;else low=mid;mid=(low+high)/2;}return mid;}

    /** Centripetal-looking uniform Catmull-Rom fallback for fit-only SPLINE records. */
    private static ArrayList<Point> catmullRom(List<Point>seed,boolean closed){
        ArrayList<Point>out=new ArrayList<>();if(seed==null||seed.isEmpty())return out;if(seed.size()==1){out.add(seed.get(0));return out;}if(seed.size()==2){out.add(seed.get(0));out.add(seed.get(1));return out;}
        int segments=closed?seed.size():seed.size()-1;for(int i=0;i<segments;i++){Point p0=get(seed,i-1,closed),p1=get(seed,i,closed),p2=get(seed,i+1,closed),p3=get(seed,i+2,closed);int steps=24;for(int s=0;s<steps;s++){double t=(double)s/steps,t2=t*t,t3=t2*t;double x=.5*((2*p1.x)+(-p0.x+p2.x)*t+(2*p0.x-5*p1.x+4*p2.x-p3.x)*t2+(-p0.x+3*p1.x-3*p2.x+p3.x)*t3);double y=.5*((2*p1.y)+(-p0.y+p2.y)*t+(2*p0.y-5*p1.y+4*p2.y-p3.y)*t2+(-p0.y+3*p1.y-3*p2.y+p3.y)*t3);append(out,new Point(x,y));}}
        if(closed)append(out,new Point(out.get(0).x,out.get(0).y));else append(out,seed.get(seed.size()-1));return out;
    }
    private static Point get(List<Point>p,int i,boolean closed){int n=p.size();if(closed){int k=((i%n)+n)%n;return p.get(k);}return p.get(Math.max(0,Math.min(n-1,i)));}
    private static void append(ArrayList<Point>out,Point p){if(p==null||!Double.isFinite(p.x)||!Double.isFinite(p.y))return;if(!out.isEmpty()){Point q=out.get(out.size()-1);if(Math.hypot(q.x-p.x,q.y-p.y)<1e-10)return;}out.add(p);}

    private static List<Double>values(List<String>a,int from,int to,int wanted){ArrayList<Double>out=new ArrayList<>();for(int i=from;i+1<to;i+=2)if(integer(a.get(i))==wanted){try{double v=Double.parseDouble(a.get(i+1).trim());if(Double.isFinite(v))out.add(v);}catch(Exception ignored){}}return out;}
    private static List<Point>points(List<String>a,int from,int to,int xc,int yc){ArrayList<Point>out=new ArrayList<>();Double x=null;for(int i=from;i+1<to;i+=2){int code=integer(a.get(i));if(code==xc)x=numberValue(a.get(i+1));else if(code==yc&&x!=null){Double y=numberValue(a.get(i+1));if(y!=null)out.add(new Point(x,y));x=null;}}return out;}
    private static double number(List<String>a,int from,int to,int code,double fallback){for(int i=from;i+1<to;i+=2)if(integer(a.get(i))==code){Double v=numberValue(a.get(i+1));return v==null?fallback:v;}return fallback;}
    private static Double numberValue(String s){try{double v=Double.parseDouble(s.trim());return Double.isFinite(v)?v:null;}catch(Exception e){return null;}}
    private static int integer(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return-1;}}
    private DxfSpline(){}
}
