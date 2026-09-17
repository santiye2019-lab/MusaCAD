package com.musa.cad;

import java.util.*;

/** Pure-Java parser for rectangular 2D paper-space VIEWPORT entities. */
public final class DxfViewport {
    public static final int FLAG_PERSPECTIVE=1;
    public static final int FLAG_NON_RECTANGULAR=65536;
    public static final int FLAG_OFF=131072;

    public static final class View {
        public final double paperCenterX,paperCenterY,paperWidth,paperHeight;
        public final double viewCenterX,viewCenterY,viewHeight,twistDegrees,targetX,targetY;
        public final double directionX,directionY,directionZ;
        public final int status,id,flags;
        View(double pcx,double pcy,double pw,double ph,double vcx,double vcy,double vh,double twist,double tx,double ty,double dx,double dy,double dz,int status,int id,int flags){
            paperCenterX=pcx;paperCenterY=pcy;paperWidth=Math.abs(pw);paperHeight=Math.abs(ph);viewCenterX=vcx;viewCenterY=vcy;viewHeight=Math.abs(vh);twistDegrees=twist;targetX=tx;targetY=ty;directionX=dx;directionY=dy;directionZ=dz;this.status=status;this.id=id;this.flags=flags;
        }
        public boolean active2d(){return id>1&&status>0&&(flags&FLAG_OFF)==0&&(flags&FLAG_PERSPECTIVE)==0&&paperWidth>0d&&paperHeight>0d&&viewHeight>0d&&Math.abs(directionX)<1e-7&&Math.abs(directionY)<1e-7&&directionZ>0d;}
        public boolean rectangular(){return (flags&FLAG_NON_RECTANGULAR)==0;}
        public double scale(){return viewHeight>0d?paperHeight/viewHeight:0d;}
        /** World-space model center for a top-view viewport. */
        public double modelCenterX(){double r=Math.toRadians(twistDegrees),co=Math.cos(r),si=Math.sin(r);return targetX+viewCenterX*co-viewCenterY*si;}
        public double modelCenterY(){double r=Math.toRadians(twistDegrees),co=Math.cos(r),si=Math.sin(r);return targetY+viewCenterX*si+viewCenterY*co;}
        public double left(){return paperCenterX-paperWidth/2d;}public double right(){return paperCenterX+paperWidth/2d;}
        public double bottom(){return paperCenterY-paperHeight/2d;}public double top(){return paperCenterY+paperHeight/2d;}
    }

    public static View parse(List<String>a,int from,int to){
        return new View(number(a,from,to,10,0d),number(a,from,to,20,0d),number(a,from,to,40,0d),number(a,from,to,41,0d),
            number(a,from,to,12,0d),number(a,from,to,22,0d),number(a,from,to,45,0d),number(a,from,to,51,0d),
            number(a,from,to,17,0d),number(a,from,to,27,0d),number(a,from,to,16,0d),number(a,from,to,26,0d),number(a,from,to,36,1d),
            integer(a,from,to,68,0),integer(a,from,to,69,0),integer(a,from,to,90,0));
    }

    private static int code(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double number(List<String>a,int from,int to,int wanted,double fallback){for(int i=from;i+1<to;i+=2)if(code(a.get(i))==wanted)try{double v=Double.parseDouble(a.get(i+1).trim());return Double.isFinite(v)?v:fallback;}catch(Exception ignored){return fallback;}return fallback;}
    private static int integer(List<String>a,int from,int to,int wanted,int fallback){for(int i=from;i+1<to;i+=2)if(code(a.get(i))==wanted)try{return Integer.parseInt(a.get(i+1).trim());}catch(Exception ignored){return fallback;}return fallback;}
    private DxfViewport(){}
}
