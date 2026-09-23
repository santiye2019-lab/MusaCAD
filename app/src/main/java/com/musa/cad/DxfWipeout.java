package com.musa.cad;

import java.util.*;

/** Decodes DXF WIPEOUT/RasterImage-style clipping geometry into drawing coordinates. */
public final class DxfWipeout {
    public static final class Point {
        public final double x,y;
        Point(double x,double y){this.x=x;this.y=y;}
    }
    public static final class Result {
        public final List<Point> boundary;
        Result(List<Point> boundary){this.boundary=Collections.unmodifiableList(boundary);}
    }
    private DxfWipeout(){}

    public static Result parse(List<String>a,int from,int to){
        double ox=0d,oy=0d,ux=0d,uy=0d,vx=0d,vy=0d,width=1d,height=1d;
        boolean hasOx=false,hasOy=false,hasU=false,hasV=false;
        ArrayList<Point> clip=new ArrayList<>();Double clipX=null;
        for(int i=from;i+1<to;i+=2){
            int code=intOf(a.get(i));String raw=a.get(i+1);
            switch(code){
                case 10:ox=d(raw);hasOx=true;break;
                case 20:oy=d(raw);hasOy=true;break;
                case 11:ux=d(raw);hasU=true;break;
                case 21:uy=d(raw);break;
                case 12:vx=d(raw);hasV=true;break;
                case 22:vy=d(raw);break;
                case 13:width=Math.max(1d,Math.abs(d(raw)));break;
                case 23:height=Math.max(1d,Math.abs(d(raw)));break;
                case 14:clipX=d(raw);break;
                case 24:if(clipX!=null){clip.add(new Point(clipX,d(raw)));clipX=null;}break;
                default:break;
            }
        }
        if(!hasOx||!hasOy||!hasU||!hasV)return new Result(Collections.emptyList());
        if(clip.size()<3){
            clip.clear();clip.add(new Point(-.5,-.5));clip.add(new Point(width-.5,-.5));
            clip.add(new Point(width-.5,height-.5));clip.add(new Point(-.5,height-.5));
        }
        ArrayList<Point> world=new ArrayList<>(clip.size());
        for(Point p:clip){
            double px=p.x+.5,py=p.y+.5;
            world.add(new Point(ox+ux*px+vx*py,oy+uy*px+vy*py));
        }
        return new Result(world);
    }
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double d(String s){try{return Double.parseDouble(s.trim());}catch(Exception e){return 0d;}}
}
