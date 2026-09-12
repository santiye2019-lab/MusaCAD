package com.musa.cad;

import android.graphics.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Lightweight ASCII DXF renderer for common 2D entities. */
public final class DxfParser {
    private static final int SIZE=2400,MARGIN=80;
    private interface Entity{void bounds(RectF b);void draw(Canvas c,Paint p,Matrix m);}
    private static final class Line implements Entity{
        final float x1,y1,x2,y2;Line(float a,float b,float c,float d){x1=a;y1=b;x2=c;y2=d;}
        public void bounds(RectF b){add(b,x1,y1);add(b,x2,y2);}public void draw(Canvas c,Paint p,Matrix m){float[]v={x1,y1,x2,y2};m.mapPoints(v);c.drawLine(v[0],v[1],v[2],v[3],p);}
    }
    private static final class Poly implements Entity{
        final ArrayList<PointF> pts;final boolean closed;Poly(ArrayList<PointF>p,boolean c){pts=p;closed=c;}
        public void bounds(RectF b){for(PointF p:pts)add(b,p.x,p.y);}public void draw(Canvas c,Paint p,Matrix m){if(pts.size()<2)return;Path q=new Path();float[]v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);q.moveTo(v[0],v[1]);for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);q.lineTo(v[0],v[1]);}if(closed)q.close();c.drawPath(q,p);}
    }
    private static final class Circle implements Entity{
        final float x,y,r,start,sweep;Circle(float a,float b,float c,float d,float e){x=a;y=b;r=c;start=d;sweep=e;}
        public void bounds(RectF b){add(b,x-r,y-r);add(b,x+r,y+r);}public void draw(Canvas c,Paint p,Matrix m){float[]o={x,y},e={x+r,y};m.mapPoints(o);m.mapPoints(e);float rr=Math.abs(e[0]-o[0]);c.drawArc(new RectF(o[0]-rr,o[1]-rr,o[0]+rr,o[1]+rr),-start,-sweep,false,p);}
    }
    public static Bitmap render(File file)throws IOException{
        List<String> lines=readLines(file);if(lines.size()<4)return null;ArrayList<Entity> entities=new ArrayList<>();boolean section=false;
        for(int i=0;i+1<lines.size();){int code=intOf(lines.get(i));String value=lines.get(i+1).trim();i+=2;if(code==0&&"SECTION".equals(value)&&i+1<lines.size()&&"2".equals(lines.get(i).trim())&&"ENTITIES".equals(lines.get(i+1).trim())){section=true;i+=2;continue;}if(code==0&&"ENDSEC".equals(value)){section=false;continue;}if(!section||code!=0)continue;int end=i;while(end+1<lines.size()&&intOf(lines.get(end))!=0)end+=2;Entity e=parse(value,lines,i,end);if(e!=null)entities.add(e);i=end;}
        if(entities.isEmpty())return null;RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);for(Entity e:entities)e.bounds(b);if(b.width()<=0||b.height()<=0)return null;float s=Math.min((SIZE-2f*MARGIN)/b.width(),(SIZE-2f*MARGIN)/b.height());Matrix m=new Matrix();m.postTranslate(-b.left,-b.bottom);m.postScale(s,-s);m.postTranslate(MARGIN+(SIZE-2*MARGIN-b.width()*s)/2f,MARGIN+(SIZE-2*MARGIN-b.height()*s)/2f);Bitmap out=Bitmap.createBitmap(SIZE,SIZE,Bitmap.Config.ARGB_8888);Canvas c=new Canvas(out);c.drawColor(Color.rgb(18,24,30));Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setColor(Color.rgb(225,235,241));p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(2f);for(Entity e:entities)e.draw(c,p,m);return out;
    }
    private static Entity parse(String type,List<String>a,int from,int to){if("LINE".equals(type))return new Line(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21));if("CIRCLE".equals(type))return new Circle(f(a,from,to,10),f(a,from,to,20),f(a,from,to,40),0,360);if("ARC".equals(type)){float start=f(a,from,to,50),end=f(a,from,to,51),sweep=end-start;if(sweep<0)sweep+=360;return new Circle(f(a,from,to,10),f(a,from,to,20),f(a,from,to,40),start,sweep);}if("LWPOLYLINE".equals(type)){ArrayList<PointF>p=new ArrayList<>();Float x=null;for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==10)x=floatOf(a.get(i+1));else if(code==20&&x!=null){p.add(new PointF(x,floatOf(a.get(i+1))));x=null;}}return new Poly(p,(((int)f(a,from,to,70))&1)!=0);}return null;}
    private static List<String>readLines(File f)throws IOException{ArrayList<String>r=new ArrayList<>();try(BufferedReader b=new BufferedReader(new InputStreamReader(new FileInputStream(f),StandardCharsets.ISO_8859_1))){String s;while((s=b.readLine())!=null)r.add(s);}return r;}private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return-1;}}private static float floatOf(String s){try{return Float.parseFloat(s.trim());}catch(Exception e){return 0;}}private static float f(List<String>a,int from,int to,int wanted){for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted)return floatOf(a.get(i+1));return 0;}private static void add(RectF b,float x,float y){b.left=Math.min(b.left,x);b.top=Math.min(b.top,y);b.right=Math.max(b.right,x);b.bottom=Math.max(b.bottom,y);}private DxfParser(){}
}
