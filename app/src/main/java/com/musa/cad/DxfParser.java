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
        public void bounds(RectF b){add(b,x-r,y-r);add(b,x+r,y+r);}public void draw(Canvas c,Paint p,Matrix m){Path path=new Path();path.addArc(new RectF(x-r,y-r,x+r,y+r),start,sweep);path.transform(m);c.drawPath(path,p);}
    }
    private static final class Transformed implements Entity {
        final Entity entity;final Matrix matrix;
        Transformed(Entity entity,DxfBlocks.Transform t)throws IOException {
            this.entity=entity;
            float[] v={(float)t.a,(float)t.c,(float)t.x,(float)t.b,(float)t.d,(float)t.y,0,0,1};
            for(float n:v)if(!Float.isFinite(n))throw new IOException("DXF blok dönüşümü sınır dışında");
            matrix=new Matrix();matrix.setValues(v);
        }
        public void bounds(RectF b){
            RectF local=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            entity.bounds(local);
            if(local.left>local.right||local.top>local.bottom)return;
            matrix.mapRect(local);add(b,local.left,local.top);add(b,local.right,local.bottom);
        }
        public void draw(Canvas c,Paint p,Matrix view){
            Matrix combined=new Matrix();combined.setConcat(view,matrix);entity.draw(c,p,combined);
        }
    }
    public static final class Result {
        public final Bitmap bitmap;
        public final float[] snapPoints;
        public final int entityCount, layerCount, skippedCount;
        public int conversionWarnings;
        public final Set<String> layerNames,visibleLayers;
        private final List<Entity> document;
        private final Matrix view;
        Result(Bitmap b,int e,int skipped,float[] points,List<Entity> document,Matrix view,Set<String> all,Set<String> visible){
            bitmap=b;entityCount=e;skippedCount=skipped;snapPoints=points;
            this.document=document;this.view=new Matrix(view);
            layerNames=Collections.unmodifiableSet(new TreeSet<>(all));
            visibleLayers=Collections.unmodifiableSet(new TreeSet<>(visible));layerCount=all.size();
        }
        public Result withVisibleLayers(Set<String> selected)throws IOException{
            Set<String> visible=new HashSet<>(selected);visible.retainAll(layerNames);
            Result result=renderLayers(document,view,layerNames,visible,skippedCount);result.conversionWarnings=conversionWarnings;return result;
        }
    }
    // Display colors distinguish layers; these are not the source file's ACI colors.
    private static final class LayerEntity implements Entity {
        final Entity entity; final String layer;
        LayerEntity(Entity e,String l){entity=e;layer=l;}
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){
            p.setColor(Color.HSVToColor(new float[]{Math.floorMod(layer.hashCode(),360),.45f,.95f}));
            entity.draw(c,p,m);
        }
    }
    private static final class Label implements Entity {
        final float x,y,height,angle; final String[] rows;
        Label(float x,float y,float h,float angle,String text){
            this.x=x;this.y=y;this.height=h;this.angle=angle;rows=text.split("\n",-1);
        }
        private Path shape(){
            Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setTextSize(height);
            Path shape=new Path();
            for(int i=0;i<rows.length;i++){Path line=new Path();p.getTextPath(rows[i],0,rows[i].length(),0,i*height*1.3f,line);shape.addPath(line);}
            Matrix placement=new Matrix();placement.setScale(1,-1);placement.postRotate(angle);placement.postTranslate(x,y);
            shape.transform(placement);return shape;
        }
        public void bounds(RectF b){RectF r=new RectF();shape().computeBounds(r,true);add(b,r.left,r.top);add(b,r.right,r.bottom);}
        public void draw(Canvas c,Paint p,Matrix m){
            Path path=shape();path.transform(m);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);p.setStyle(Paint.Style.STROKE);
        }
    }
    private static String str(List<String>a,int from,int to,int code,String fallback){
        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==code)return a.get(i+1);
        return fallback;
    }
    public static Result render(File file)throws IOException{
        ArrayList<Entity> entities=new ArrayList<>();Set<String> layers=new HashSet<>();
        final int[] unsupported={0};
        DxfBlocks.Result expanded=DxfBlocks.expand(file,charset(file),item->{
            FileTransfer.checkCancelled();
            Entity entity=parse(item.record.type,item.record.tags,item.record.from,item.record.to);
            if(entity==null){unsupported[0]++;return;}
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer));layers.add(item.layer);
        });
        int skipped=expanded.skipped+unsupported[0];
        if(entities.isEmpty())return null;RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;if(b.width()==0){b.left-=.5f;b.right+=.5f;}if(b.height()==0){b.top-=.5f;b.bottom+=.5f;}float s=Math.min((SIZE-2f*MARGIN)/b.width(),(SIZE-2f*MARGIN)/b.height());Matrix m=new Matrix();m.postTranslate(-b.left,-b.bottom);m.postScale(s,-s);m.postTranslate(MARGIN+(SIZE-2*MARGIN-b.width()*s)/2f,MARGIN+(SIZE-2*MARGIN-b.height()*s)/2f);return renderLayers(entities,m,layers,layers,skipped);
    }
    private static Result renderLayers(List<Entity> document,Matrix view,Set<String> all,Set<String> visible,int skipped)throws IOException{
        List<Entity> shown=new ArrayList<>();
        for(Entity entity:document){
            FileTransfer.checkCancelled();
            if(visible.contains(((LayerEntity)entity).layer))shown.add(entity);
        }
        Bitmap bitmap=Bitmap.createBitmap(SIZE,SIZE,Bitmap.Config.ARGB_8888);
        try{
            Canvas canvas=new Canvas(bitmap);canvas.drawColor(Color.rgb(18,24,30));
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2f);
            for(Entity entity:shown){FileTransfer.checkCancelled();entity.draw(canvas,paint,view);}
            FileTransfer.checkCancelled();
            return new Result(bitmap,shown.size(),skipped,snapPoints(shown,view),document,view,all,visible);
        }catch(IOException|RuntimeException|OutOfMemoryError e){bitmap.recycle();throw e;}
    }

    private static float[] snapPoints(List<Entity> entities,Matrix matrix)throws IOException {
        long count=0;
        for(Entity wrapped:entities){
            FileTransfer.checkCancelled();
            Entity entity=((LayerEntity)wrapped).entity;
            if(entity instanceof Transformed)entity=((Transformed)entity).entity;
            if(entity instanceof Line)count+=2;
            else if(entity instanceof Poly)count+=((Poly)entity).pts.size();
            if(count>8000000)throw new IOException("DXF yakalama noktası sınırı aşıldı");
        }
        float[] result=new float[(int)count*2];int at=0;
        for(Entity wrapped:entities){
            FileTransfer.checkCancelled();
            Entity entity=((LayerEntity)wrapped).entity;
            Matrix transform=null;
            if(entity instanceof Transformed){transform=((Transformed)entity).matrix;entity=((Transformed)entity).entity;}
            int start=at;
            if(entity instanceof Line){
                Line line=(Line)entity;
                result[at++]=line.x1;result[at++]=line.y1;result[at++]=line.x2;result[at++]=line.y2;
            }else if(entity instanceof Poly){
                for(PointF point:((Poly)entity).pts){result[at++]=point.x;result[at++]=point.y;}
            }
            if(transform!=null&&at>start)transform.mapPoints(result,start,result,start,(at-start)/2);
        }
        matrix.mapPoints(result);return result;
    }
    private static Entity parse(String type,List<String>a,int from,int to){
        if("TEXT".equals(type)||"MTEXT".equals(type)){
            StringBuilder text=new StringBuilder();
            for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==1||("MTEXT".equals(type)&&code==3))text.append(a.get(i+1));}
            String plain=DxfText.plain(text.toString());
            if(plain.trim().isEmpty())return null;
            float angle=f(a,from,to,50);
            if("MTEXT".equals(type)){
                angle=(float)Math.toDegrees(angle);
                if(!str(a,from,to,11,"").isEmpty())angle=(float)Math.toDegrees(Math.atan2(f(a,from,to,21),f(a,from,to,11)));
            }
            return new Label(f(a,from,to,10),f(a,from,to,20),Math.max(.01f,f(a,from,to,40)),angle,plain);
        }
if("LINE".equals(type))return new Line(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21));if("CIRCLE".equals(type))return new Circle(f(a,from,to,10),f(a,from,to,20),f(a,from,to,40),0,360);if("ARC".equals(type)){float start=f(a,from,to,50),end=f(a,from,to,51),sweep=end-start;if(sweep<0)sweep+=360;return new Circle(f(a,from,to,10),f(a,from,to,20),f(a,from,to,40),start,sweep);}if("LWPOLYLINE".equals(type)){ArrayList<PointF>p=new ArrayList<>();Float x=null;for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==10)x=floatOf(a.get(i+1));else if(code==20&&x!=null){p.add(new PointF(x,floatOf(a.get(i+1))));x=null;}}return new Poly(p,(((int)f(a,from,to,70))&1)!=0);}return null;}
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return-1;}}private static float floatOf(String s){try{return Float.parseFloat(s.trim());}catch(Exception e){return 0;}}private static float f(List<String>a,int from,int to,int wanted){for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted)return floatOf(a.get(i+1));return 0;}private static void add(RectF b,float x,float y){b.left=Math.min(b.left,x);b.top=Math.min(b.top,y);b.right=Math.max(b.right,x);b.bottom=Math.max(b.bottom,y);}private static java.nio.charset.Charset charset(File file)throws IOException {
        String header;
        try(InputStream in=new FileInputStream(file)){byte[] bytes=new byte[65536];int n=in.read(bytes);header=new String(bytes,0,Math.max(0,n),StandardCharsets.ISO_8859_1);}
        java.util.regex.Matcher v=java.util.regex.Pattern.compile("AC10([0-9]{2})").matcher(header);
        if(v.find()&&Integer.parseInt(v.group(1))>=21)return StandardCharsets.UTF_8;
        java.util.regex.Matcher cp=java.util.regex.Pattern.compile("ANSI_([0-9]+)").matcher(header);
        try{if(cp.find())return java.nio.charset.Charset.forName("windows-"+cp.group(1));}catch(Exception ignored){}
        return java.nio.charset.Charset.forName("windows-1252");
    }
    private DxfParser(){}
}
