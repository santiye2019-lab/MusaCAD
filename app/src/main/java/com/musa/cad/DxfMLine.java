package com.musa.cad;

import java.util.*;

/** Pure-Java decoder for visible 2D DXF MLINE element geometry. */
public final class DxfMLine {
    public static final class Segment {
        public final double x1,y1,x2,y2;
        Segment(double x1,double y1,double x2,double y2){this.x1=x1;this.y1=y1;this.x2=x2;this.y2=y2;}
    }
    public static final class Result {
        public final List<Segment> segments;
        public final int vertexCount,elementCount;
        public final boolean closed;
        Result(List<Segment> segments,int vertexCount,int elementCount,boolean closed){
            this.segments=Collections.unmodifiableList(new ArrayList<>(segments));
            this.vertexCount=vertexCount;this.elementCount=elementCount;this.closed=closed;
        }
    }
    private static final class Vertex {
        double x,y,dx,dy,mx,my;
        final List<double[]> params=new ArrayList<>();
    }
    private static final class Tag {
        final int code;final String value;
        Tag(int code,String value){this.code=code;this.value=value;}
    }

    public static Result parse(List<String> raw,int from,int to){
        ArrayList<Tag> tags=read(raw,from,to);if(tags==null)return empty();
        int vertexCount=intValue(tags,72,0),elementCount=intValue(tags,73,0),flags=intValue(tags,71,0);
        boolean closed=(flags&2)!=0;
        if(vertexCount<2||elementCount<1||vertexCount>100000||elementCount>128)return new Result(Collections.emptyList(),Math.max(0,vertexCount),Math.max(0,elementCount),closed);
        ArrayList<Vertex> vertices=new ArrayList<>(vertexCount);
        int cursor=0;
        for(int v=0;v<vertexCount;v++){
            cursor=find(tags,cursor,11);if(cursor<0)break;
            Vertex q=new Vertex();q.x=d(tags.get(cursor++).value,Double.NaN);
            int iy=findBefore(tags,cursor,21,11);if(iy<0)break;q.y=d(tags.get(iy).value,Double.NaN);cursor=iy+1;
            int idx=findBefore(tags,cursor,12,11);if(idx<0)break;q.dx=d(tags.get(idx).value,Double.NaN);cursor=idx+1;
            int idy=findBefore(tags,cursor,22,11);if(idy<0)break;q.dy=d(tags.get(idy).value,Double.NaN);cursor=idy+1;
            int imx=findBefore(tags,cursor,13,11);if(imx<0)break;q.mx=d(tags.get(imx).value,Double.NaN);cursor=imx+1;
            int imy=findBefore(tags,cursor,23,11);if(imy<0)break;q.my=d(tags.get(imy).value,Double.NaN);cursor=imy+1;
            if(!normalizeDirection(q))break;
            for(int e=0;e<elementCount;e++){
                int i74=findBefore(tags,cursor,74,11);if(i74<0){q.params.add(new double[0]);continue;}
                int count=Math.max(0,Math.min(2048,i(tags.get(i74).value,0)));cursor=i74+1;
                double[] p=new double[count];int got=0;
                while(cursor<tags.size()&&got<count&&tags.get(cursor).code!=11){
                    Tag t=tags.get(cursor++);if(t.code==41)p[got++]=d(t.value,0d);else if(t.code==74||t.code==75)break;
                }
                if(got<count)p=Arrays.copyOf(p,got);q.params.add(p);
                // Advance over the area-fill parameterization for this element.
                int i75=findBefore(tags,cursor,75,11,74);
                if(i75>=0){
                    int fill=Math.max(0,Math.min(2048,i(tags.get(i75).value,0)));cursor=i75+1;int seen=0;
                    while(cursor<tags.size()&&seen<fill&&tags.get(cursor).code!=11&&tags.get(cursor).code!=74){
                        if(tags.get(cursor).code==42)seen++;cursor++;
                    }
                }
            }
            if(Double.isFinite(q.x)&&Double.isFinite(q.y))vertices.add(q);else break;
        }
        if(vertices.size()<2)return new Result(Collections.emptyList(),vertices.size(),elementCount,closed);
        int usable=Math.min(vertexCount,vertices.size()),segCount=closed?usable:usable-1;
        ArrayList<Segment> out=new ArrayList<>();
        for(int v=0;v<segCount;v++){
            Vertex a=vertices.get(v),b=vertices.get((v+1)%usable);
            for(int e=0;e<elementCount;e++)appendElement(out,a,b,e);
        }
        return new Result(out,usable,elementCount,closed);
    }

    private static void appendElement(List<Segment> out,Vertex a,Vertex b,int e){
        double[] ap=e<a.params.size()?a.params.get(e):new double[0],bp=e<b.params.size()?b.params.get(e):new double[0];
        double ao=ap.length>0?ap[0]:0d,bo=bp.length>0?bp[0]:ao;
        double ax=a.x+a.mx*ao,ay=a.y+a.my*ao,bx=b.x+b.mx*bo,by=b.y+b.my*bo;
        double dx=a.dx,dy=a.dy,startShift=ap.length>1?ap[1]:0d;
        double sx=ax+dx*startShift,sy=ay+dy*startShift;
        double total=(bx-sx)*dx+(by-sy)*dy;
        if(!Double.isFinite(total)||total<=1e-9){
            double vx=bx-sx,vy=by-sy,len=Math.hypot(vx,vy);if(len<=1e-9)return;dx=vx/len;dy=vy/len;total=len;
        }
        if(ap.length<=2){add(out,sx,sy,sx+dx*total,sy+dy*total);return;}
        double cursor=0d;
        for(int k=2;k<ap.length;k+=2){
            double cutStart=clamp(ap[k],0d,total);
            if(cutStart>cursor+1e-9)add(out,sx+dx*cursor,sy+dy*cursor,sx+dx*cutStart,sy+dy*cutStart);
            if(k+1<ap.length)cursor=clamp(ap[k+1],cutStart,total);else{cursor=cutStart;break;}
        }
        if(cursor<total-1e-9)add(out,sx+dx*cursor,sy+dy*cursor,sx+dx*total,sy+dy*total);
    }
    private static void add(List<Segment> out,double x1,double y1,double x2,double y2){
        if(Double.isFinite(x1)&&Double.isFinite(y1)&&Double.isFinite(x2)&&Double.isFinite(y2)&&Math.hypot(x2-x1,y2-y1)>1e-9)out.add(new Segment(x1,y1,x2,y2));
    }
    private static boolean normalizeDirection(Vertex q){
        double dl=Math.hypot(q.dx,q.dy),ml=Math.hypot(q.mx,q.my);
        if(!Double.isFinite(dl)||!Double.isFinite(ml)||dl<1e-12||ml<1e-12)return false;
        q.dx/=dl;q.dy/=dl;q.mx/=ml;q.my/=ml;return true;
    }
    private static ArrayList<Tag> read(List<String> raw,int from,int to){
        if(raw==null||from<0||to>raw.size()||from>to)return null;ArrayList<Tag> out=new ArrayList<>();
        try{for(int p=from;p+1<to;p+=2)out.add(new Tag(Integer.parseInt(raw.get(p).trim()),raw.get(p+1).trim()));return out;}catch(Exception ex){return null;}
    }
    private static int find(List<Tag> tags,int from,int wanted){for(int p=Math.max(0,from);p<tags.size();p++)if(tags.get(p).code==wanted)return p;return-1;}
    private static int findBefore(List<Tag> tags,int from,int wanted,int... stops){
        outer:for(int p=Math.max(0,from);p<tags.size();p++){int c=tags.get(p).code;if(c==wanted)return p;for(int stop:stops)if(c==stop)break outer;}return-1;
    }
    private static int intValue(List<Tag> tags,int code,int fallback){for(Tag t:tags)if(t.code==code)return i(t.value,fallback);return fallback;}
    private static int i(String v,int fallback){try{return Integer.parseInt(v.trim());}catch(Exception ex){return fallback;}}
    private static double d(String v,double fallback){try{double n=Double.parseDouble(v.trim());return Double.isFinite(n)?n:fallback;}catch(Exception ex){return fallback;}}
    private static double clamp(double v,double lo,double hi){return Math.max(lo,Math.min(hi,v));}
    private static Result empty(){return new Result(Collections.emptyList(),0,0,false);}
    private DxfMLine(){}
}
