package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Pure-Java parser for DXF HATCH boundary loops and embedded pattern-line definitions. */
public final class DxfHatch {
    public static final class Point {
        public final double x,y;
        Point(double x,double y){this.x=x;this.y=y;}
    }
    public static final class Loop {
        public final List<Point> points;
        public final boolean closed;
        public final int flags;
        Loop(List<Point> points,boolean closed,int flags){this.points=Collections.unmodifiableList(new ArrayList<>(points));this.closed=closed;this.flags=flags;}
    }
    public static final class PatternLine {
        public final double angleDegrees,baseX,baseY,offsetX,offsetY;
        public final double[] dashes;
        PatternLine(double angle,double bx,double by,double ox,double oy,double[] dashes){
            angleDegrees=angle;baseX=bx;baseY=by;offsetX=ox;offsetY=oy;this.dashes=dashes==null?new double[0]:dashes.clone();
        }
    }
    public static final class Result {
        public final boolean solid;
        public final String patternName;
        public final double patternAngleDegrees,patternScale;
        public final int hatchStyle;
        public final List<Loop> loops;
        public final List<PatternLine> patternLines;
        Result(boolean solid,String patternName,double angle,double scale,int style,List<Loop> loops,List<PatternLine> patternLines){
            this.solid=solid;this.patternName=patternName;patternAngleDegrees=angle;patternScale=scale;hatchStyle=style;
            this.loops=Collections.unmodifiableList(new ArrayList<>(loops));this.patternLines=Collections.unmodifiableList(new ArrayList<>(patternLines));
        }
    }

    private static final class Tag {
        final int code;final String value;
        Tag(int code,String value){this.code=code;this.value=value;}
    }
    private static final class Vertex {
        final double x,y,bulge;
        Vertex(double x,double y,double bulge){this.x=x;this.y=y;this.bulge=bulge;}
    }
    private static final class ParseLoop {
        final Loop loop;final int next;
        ParseLoop(Loop loop,int next){this.loop=loop;this.next=next;}
    }

    public static Result parse(List<String> tags,int from,int to)throws IOException{
        List<Tag> list=read(tags,from,to);
        boolean solid=intValue(list,70,0)==1;
        String name=textValue(list,2,"SOLID");
        double angle=numberValue(list,52,0d);
        double scale=numberValue(list,41,1d);if(!Double.isFinite(scale)||scale<=0d)scale=1d;
        int style=intValue(list,75,0);
        ArrayList<Loop> loops=new ArrayList<>();
        int pathCount=intValue(list,91,0);
        int cursor=indexOf(list,91,0);cursor=cursor<0?0:cursor+1;
        for(int p=0;p<pathCount;p++){
            cursor=findCode(list,cursor,92);if(cursor<0)break;
            int flags=parseInt(list.get(cursor).value,0);cursor++;
            ParseLoop parsed=(flags&2)!=0?parsePolylineLoop(list,cursor,flags):parseEdgeLoop(list,cursor,flags);
            if(parsed.loop!=null&&parsed.loop.points.size()>=2)loops.add(parsed.loop);
            cursor=Math.max(cursor,parsed.next);
        }
        return new Result(solid,name,angle,scale,style,loops,parsePatternLines(list));
    }

    private static ParseLoop parsePolylineLoop(List<Tag> tags,int start,int flags)throws IOException{
        int cursor=start,hasBulge=0,closed=1,count=0;
        while(cursor<tags.size()&&tags.get(cursor).code!=93&&tags.get(cursor).code!=92){
            Tag t=tags.get(cursor++);if(t.code==72)hasBulge=parseInt(t.value,0);else if(t.code==73)closed=parseInt(t.value,1);
        }
        if(cursor<tags.size()&&tags.get(cursor).code==93){count=parseInt(tags.get(cursor).value,0);cursor++;}
        ArrayList<Vertex> raw=new ArrayList<>();
        for(int i=0;i<count&&cursor<tags.size();i++){
            cursor=findCodeUntil(tags,cursor,10,92,97,75,76,78,98);if(cursor<0)break;
            double x=parseDouble(tags.get(cursor).value,0);cursor++;
            int yIndex=findCodeUntil(tags,cursor,20,10,92,97,75,76,78,98);if(yIndex<0)break;
            double y=parseDouble(tags.get(yIndex).value,0);cursor=yIndex+1;double bulge=0d;
            if(hasBulge!=0&&cursor<tags.size()&&tags.get(cursor).code==42){bulge=parseDouble(tags.get(cursor).value,0);cursor++;}
            raw.add(new Vertex(x,y,bulge));
        }
        ArrayList<Point> points=expandBulges(raw,closed!=0);cursor=skipSourceHandles(tags,cursor);
        return new ParseLoop(new Loop(points,closed!=0,flags),cursor);
    }

    private static ParseLoop parseEdgeLoop(List<Tag> tags,int start,int flags)throws IOException{
        int cursor=findCodeUntil(tags,start,93,92,75,76,78,98);if(cursor<0)return new ParseLoop(null,start);
        int edgeCount=parseInt(tags.get(cursor).value,0);cursor++;ArrayList<Point> points=new ArrayList<>();
        for(int e=0;e<edgeCount&&cursor<tags.size();e++){
            cursor=findCodeUntil(tags,cursor,72,92,97,75,76,78,98);if(cursor<0)break;
            int type=parseInt(tags.get(cursor).value,0);cursor++;int end=findNextEdgeBoundary(tags,cursor);
            if(type==1)appendLineEdge(points,tags,cursor,end);
            else if(type==2)appendArcEdge(points,tags,cursor,end);
            else if(type==3)appendEllipseEdge(points,tags,cursor,end);
            else if(type==4)appendSplineEdge(points,tags,cursor,end);
            cursor=end;
        }
        cursor=skipSourceHandles(tags,cursor);return new ParseLoop(new Loop(points,true,flags),cursor);
    }

    private static void appendLineEdge(List<Point> out,List<Tag> tags,int from,int to){
        Point a=point(tags,from,to,10,20),b=point(tags,from,to,11,21);if(a!=null)addDistinct(out,a);if(b!=null)addDistinct(out,b);
    }
    private static void appendArcEdge(List<Point> out,List<Tag> tags,int from,int to){
        Point c=point(tags,from,to,10,20);if(c==null)return;double r=Math.abs(numberValue(tags,from,to,40,0));if(r<=1e-12)return;
        double a0=numberValue(tags,from,to,50,0),a1=numberValue(tags,from,to,51,0);boolean ccw=intValue(tags,from,to,73,1)!=0;
        appendArc(out,c.x,c.y,r,r,0,a0,a1,ccw);
    }
    private static void appendEllipseEdge(List<Point> out,List<Tag> tags,int from,int to){
        Point c=point(tags,from,to,10,20),major=point(tags,from,to,11,21);if(c==null||major==null)return;
        double majorLen=Math.hypot(major.x,major.y),ratio=Math.abs(numberValue(tags,from,to,40,1));if(majorLen<=1e-12||ratio<=1e-12)return;
        double rotation=Math.toDegrees(Math.atan2(major.y,major.x));double a0=numberValue(tags,from,to,50,0),a1=numberValue(tags,from,to,51,360);boolean ccw=intValue(tags,from,to,73,1)!=0;
        appendArc(out,c.x,c.y,majorLen,majorLen*ratio,rotation,a0,a1,ccw);
    }
    private static void appendSplineEdge(List<Point> out,List<Tag> tags,int from,int to){
        ArrayList<Point> control=repeatedPoints(tags,from,to,10,20);if(control.size()<2)control=repeatedPoints(tags,from,to,11,21);for(Point p:control)addDistinct(out,p);
    }

    private static void appendArc(List<Point> out,double cx,double cy,double rx,double ry,double rotationDeg,double startDeg,double endDeg,boolean ccw){
        double sweep=endDeg-startDeg;if(ccw){while(sweep<=0)sweep+=360;}else{while(sweep>=0)sweep-=360;}if(Math.abs(sweep)>360)sweep=Math.copySign(360,sweep);
        int steps=Math.max(4,(int)Math.ceil(Math.abs(sweep)/12d));double rot=Math.toRadians(rotationDeg),co=Math.cos(rot),si=Math.sin(rot);
        for(int i=0;i<=steps;i++){double a=Math.toRadians(startDeg+sweep*i/steps),x=rx*Math.cos(a),y=ry*Math.sin(a);addDistinct(out,new Point(cx+x*co-y*si,cy+x*si+y*co));}
    }

    private static ArrayList<Point> expandBulges(List<Vertex> raw,boolean closed){
        ArrayList<Point> out=new ArrayList<>();if(raw.isEmpty())return out;int segments=closed?raw.size():raw.size()-1;
        for(int i=0;i<segments;i++){Vertex a=raw.get(i),b=raw.get((i+1)%raw.size());addDistinct(out,new Point(a.x,a.y));if(Math.abs(a.bulge)>1e-9)appendBulgeInterior(out,a,b);}
        if(!closed)addDistinct(out,new Point(raw.get(raw.size()-1).x,raw.get(raw.size()-1).y));return out;
    }
    private static void appendBulgeInterior(List<Point> out,Vertex a,Vertex b){
        double dx=b.x-a.x,dy=b.y-a.y,chord=Math.hypot(dx,dy);if(chord<=1e-12)return;double theta=4d*Math.atan(a.bulge),half=theta/2d,sin=Math.sin(Math.abs(half));if(Math.abs(sin)<1e-12)return;
        double r=chord/(2d*sin),midX=(a.x+b.x)/2d,midY=(a.y+b.y)/2d,h=Math.sqrt(Math.max(0,r*r-chord*chord/4d));double nx=-dy/chord,ny=dx/chord,side=Math.signum(a.bulge);
        double cx=midX+nx*h*side,cy=midY+ny*h*side,start=Math.atan2(a.y-cy,a.x-cx);int steps=Math.max(2,(int)Math.ceil(Math.abs(theta)/Math.toRadians(12)));
        for(int i=1;i<steps;i++){double t=start+theta*i/steps;addDistinct(out,new Point(cx+r*Math.cos(t),cy+r*Math.sin(t)));}
    }

    private static ArrayList<PatternLine> parsePatternLines(List<Tag> tags){
        ArrayList<PatternLine> out=new ArrayList<>();int count=intValue(tags,78,0),cursor=indexOf(tags,78,0);if(cursor<0)return out;cursor++;
        for(int n=0;n<count&&cursor<tags.size();n++){
            cursor=findCode(tags,cursor,53);if(cursor<0)break;double angle=parseDouble(tags.get(cursor++).value,0),bx=0,by=0,ox=0,oy=0;int dashCount=0;ArrayList<Double>dashes=new ArrayList<>();
            while(cursor<tags.size()){
                Tag t=tags.get(cursor);if(t.code==53&&n+1<count)break;if(t.code==98||t.code==450)break;
                if(t.code==43)bx=parseDouble(t.value,0);else if(t.code==44)by=parseDouble(t.value,0);else if(t.code==45)ox=parseDouble(t.value,0);else if(t.code==46)oy=parseDouble(t.value,0);else if(t.code==79)dashCount=parseInt(t.value,0);else if(t.code==49&&dashes.size()<Math.max(0,dashCount))dashes.add(parseDouble(t.value,0));cursor++;
            }
            double[] ds=new double[dashes.size()];for(int i=0;i<ds.length;i++)ds[i]=dashes.get(i);out.add(new PatternLine(angle,bx,by,ox,oy,ds));
        }
        return out;
    }

    private static int skipSourceHandles(List<Tag> tags,int cursor){if(cursor<tags.size()&&tags.get(cursor).code==97){int n=parseInt(tags.get(cursor).value,0);cursor++;while(cursor<tags.size()&&n>0){if(tags.get(cursor).code==330)n--;cursor++;}}return cursor;}
    private static int findNextEdgeBoundary(List<Tag> tags,int cursor){for(int i=cursor;i<tags.size();i++){int c=tags.get(i).code;if(c==72||c==92||c==97||c==75||c==76||c==78||c==98)return i;}return tags.size();}
    private static int findCode(List<Tag> tags,int from,int code){for(int i=Math.max(0,from);i<tags.size();i++)if(tags.get(i).code==code)return i;return-1;}
    private static int indexOf(List<Tag> tags,int code,int occurrence){int seen=0;for(int i=0;i<tags.size();i++)if(tags.get(i).code==code){if(seen++==occurrence)return i;}return-1;}
    private static int findCodeUntil(List<Tag> tags,int from,int wanted,int... stop){outer:for(int i=Math.max(0,from);i<tags.size();i++){int c=tags.get(i).code;if(c==wanted)return i;for(int s:stop)if(c==s)break outer;}return-1;}
    private static Point point(List<Tag> tags,int from,int to,int xc,int yc){boolean hx=false,hy=false;double x=0,y=0;for(int i=from;i<to&&i<tags.size();i++){Tag t=tags.get(i);if(t.code==xc){x=parseDouble(t.value,0);hx=true;}else if(t.code==yc){y=parseDouble(t.value,0);hy=true;}}return hx&&hy?new Point(x,y):null;}
    private static ArrayList<Point> repeatedPoints(List<Tag> tags,int from,int to,int xc,int yc){ArrayList<Point> out=new ArrayList<>();Double x=null;for(int i=from;i<to&&i<tags.size();i++){Tag t=tags.get(i);if(t.code==xc)x=parseDouble(t.value,0);else if(t.code==yc&&x!=null){out.add(new Point(x,parseDouble(t.value,0)));x=null;}}return out;}
    private static void addDistinct(List<Point> out,Point p){if(out.isEmpty()){out.add(p);return;}Point q=out.get(out.size()-1);if(Math.hypot(q.x-p.x,q.y-p.y)>1e-9)out.add(p);}

    private static List<Tag> read(List<String> tags,int from,int to)throws IOException{
        ArrayList<Tag> out=new ArrayList<>();try{for(int i=from;i+1<to;i+=2)out.add(new Tag(Integer.parseInt(tags.get(i).trim()),tags.get(i+1).trim()));}catch(Exception e){throw new IOException("Geçersiz HATCH etiketi",e);}return out;
    }
    private static String textValue(List<Tag> tags,int code,String fallback){for(Tag t:tags)if(t.code==code)return t.value;return fallback;}
    private static int intValue(List<Tag> tags,int code,int fallback){for(Tag t:tags)if(t.code==code)return parseInt(t.value,fallback);return fallback;}
    private static int intValue(List<Tag> tags,int from,int to,int code,int fallback){for(int i=from;i<to&&i<tags.size();i++)if(tags.get(i).code==code)return parseInt(tags.get(i).value,fallback);return fallback;}
    private static double numberValue(List<Tag> tags,int code,double fallback){for(Tag t:tags)if(t.code==code)return parseDouble(t.value,fallback);return fallback;}
    private static double numberValue(List<Tag> tags,int from,int to,int code,double fallback){for(int i=from;i<to&&i<tags.size();i++)if(tags.get(i).code==code)return parseDouble(tags.get(i).value,fallback);return fallback;}
    private static int parseInt(String value,int fallback){try{return Integer.parseInt(value.trim());}catch(Exception e){return fallback;}}
    private static double parseDouble(String value,double fallback){try{double v=Double.parseDouble(value.trim());return Double.isFinite(v)?v:fallback;}catch(Exception e){return fallback;}}
    private DxfHatch(){}
}
