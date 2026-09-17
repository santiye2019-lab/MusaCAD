package com.musa.cad;

import java.util.*;

/** Converts DXF polyline bulge segments into a smooth point chain for rendering. */
public final class DxfBulge {
    public static final class Vertex {
        public final double x,y,bulge;
        public Vertex(double x,double y,double bulge){this.x=x;this.y=y;this.bulge=Double.isFinite(bulge)?bulge:0d;}
    }
    public static final class Point {
        public final double x,y;
        Point(double x,double y){this.x=x;this.y=y;}
    }

    public static List<Point> expand(List<Vertex> vertices,boolean closed){
        ArrayList<Point> out=new ArrayList<>();if(vertices==null||vertices.isEmpty())return out;
        int n=vertices.size();out.add(new Point(vertices.get(0).x,vertices.get(0).y));
        int segments=closed?n:n-1;
        for(int i=0;i<segments;i++)appendSegment(out,vertices.get(i),vertices.get((i+1)%n));
        return out;
    }

    private static void appendSegment(ArrayList<Point> out,Vertex a,Vertex b){
        double dx=b.x-a.x,dy=b.y-a.y,chord=Math.hypot(dx,dy),bulge=a.bulge;
        if(chord<1e-12||Math.abs(bulge)<1e-12){append(out,b.x,b.y);return;}
        double theta=4d*Math.atan(bulge);
        if(!Double.isFinite(theta)||Math.abs(theta)<1e-10){append(out,b.x,b.y);return;}
        // Signed offset from chord midpoint to arc center, positive to the chord's left.
        double offset=chord*(1d-bulge*bulge)/(4d*bulge);
        double ux=dx/chord,uy=dy/chord;
        double cx=(a.x+b.x)*.5d-uy*offset,cy=(a.y+b.y)*.5d+ux*offset;
        double radius=Math.hypot(a.x-cx,a.y-cy),start=Math.atan2(a.y-cy,a.x-cx);
        if(!Double.isFinite(radius)||radius<1e-12){append(out,b.x,b.y);return;}
        // Max angular step ~2.8 degrees. This stays visually smooth even at deep CAD zoom.
        int steps=Math.max(2,Math.min(256,(int)Math.ceil(Math.abs(theta)/(Math.PI/64d))));
        for(int s=1;s<steps;s++){double t=(double)s/steps,ang=start+theta*t;append(out,cx+radius*Math.cos(ang),cy+radius*Math.sin(ang));}
        append(out,b.x,b.y);
    }

    private static void append(ArrayList<Point> out,double x,double y){
        if(!Double.isFinite(x)||!Double.isFinite(y))return;
        if(!out.isEmpty()){Point last=out.get(out.size()-1);if(Math.hypot(last.x-x,last.y-y)<1e-10)return;}
        out.add(new Point(x,y));
    }
    private DxfBulge(){}
}
