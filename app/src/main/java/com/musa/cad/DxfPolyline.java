package com.musa.cad;

import java.util.*;

/** 2D LWPOLYLINE vertices and stable cubic approximations of bulge arcs. */
public final class DxfPolyline {
    public static final class Vertex {
        public final float x,y;public final double bulge;
        Vertex(float x,float y,double bulge){this.x=x;this.y=y;this.bulge=bulge;}
    }
    public final List<Vertex> vertices;
    public final boolean closed;
    private DxfPolyline(List<Vertex> vertices,boolean closed){this.vertices=vertices;this.closed=closed;}
    public static DxfPolyline parse(List<String> tags,int from,int to){
        List<Vertex> vertices=new ArrayList<>();Float x=null,y=null;double bulge=0;boolean closed=false;
        for(int i=from;i+1<to;i+=2){
            int code=Integer.parseInt(tags.get(i).trim());String value=tags.get(i+1).trim();
            if(code==70)closed=(Integer.parseInt(value)&1)!=0;
            else if(code==10){
                if(x!=null){if(y==null)throw new IllegalArgumentException("Missing vertex Y");vertices.add(new Vertex(x,y,bulge));}
                x=number(value);y=null;bulge=0;
            }else if(code==20&&x!=null)y=number(value);
            else if(code==42&&x!=null){bulge=Double.parseDouble(value);if(!Double.isFinite(bulge))throw new IllegalArgumentException("Invalid bulge");}
        }
        if(x!=null){if(y==null)throw new IllegalArgumentException("Missing vertex Y");vertices.add(new Vertex(x,y,bulge));}
        return new DxfPolyline(vertices,closed);
    }
    private static float number(String value){float n=Float.parseFloat(value);if(!Float.isFinite(n))throw new IllegalArgumentException("Invalid vertex");return n;}
    public int segmentCount(){return Math.max(0,vertices.size()-(closed?0:1));}
    /** Each entry is control1, control2, end. At most eight pieces per circular arc. */
    public List<float[]> curve(int segment){
        Vertex a=vertices.get(segment),b=vertices.get((segment+1)%vertices.size());
        List<float[]> result=new ArrayList<>();
        if(a.bulge!=0&&(a.x!=b.x||a.y!=b.y))append(a.x,a.y,b.x,b.y,a.bulge,result,0);
        return result;
    }
    private static void append(double x0,double y0,double x1,double y1,double b,List<float[]> out,int depth){
        double dx=x1-x0,dy=y1-y0;
        if(Math.abs(b)>Math.tan(Math.PI/16)*(1+1e-14)){
            if(depth>=4)throw new IllegalArgumentException("Invalid arc subdivision");
            double mx=x0+dx*.5+dy*b*.5,my=y0+dy*.5-dx*b*.5;
            double half=b/(Math.hypot(1,b)+1);
            if(!Double.isFinite(mx)||!Double.isFinite(my))throw new IllegalArgumentException("Arc outside range");
            append(x0,y0,mx,my,half,out,depth+1);append(mx,my,x1,y1,half,out,depth+1);return;
        }
        double tangent=(1-b*b)/3,normal=2*b/3;
        double[] values={x0+dx*tangent+dy*normal,y0+dy*tangent-dx*normal,
            x1-dx*tangent+dy*normal,y1-dy*tangent-dx*normal,x1,y1};
        float[] piece=new float[6];
        for(int i=0;i<6;i++){piece[i]=(float)values[i];if(!Float.isFinite(piece[i]))throw new IllegalArgumentException("Arc outside range");}
        out.add(piece);
    }
}
