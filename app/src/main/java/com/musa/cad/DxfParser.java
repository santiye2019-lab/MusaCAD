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
        final float x1,y1,x2,y2;
        Line(float a,float b,float c,float d){x1=a;y1=b;x2=c;y2=d;}
        public void bounds(RectF b){add(b,x1,y1);add(b,x2,y2);}
        public void draw(Canvas c,Paint p,Matrix m){
            float[]v={x1,y1,x2,y2};m.mapPoints(v);c.drawLine(v[0],v[1],v[2],v[3],p);
        }
    }

    private static final class Poly implements Entity{
        final ArrayList<PointF> pts;final boolean closed;
        Poly(ArrayList<PointF>p,boolean c){pts=p;closed=c;}
        public void bounds(RectF b){for(PointF p:pts)add(b,p.x,p.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(pts.size()<2)return;
            Path q=new Path();float[]v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);q.moveTo(v[0],v[1]);
            for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);q.lineTo(v[0],v[1]);}
            if(closed)q.close();c.drawPath(q,p);
        }
    }

    private static final class Circle implements Entity{
        final float x,y,r,start,sweep;
        Circle(float a,float b,float c,float d,float e){x=a;y=b;r=c;start=d;sweep=e;}
        public void bounds(RectF b){add(b,x-r,y-r);add(b,x+r,y+r);}
        public void draw(Canvas c,Paint p,Matrix m){
            Path path=new Path();path.addArc(new RectF(x-r,y-r,x+r,y+r),start,sweep);path.transform(m);c.drawPath(path,p);
        }
    }

    private static final class Marker implements Entity{
        final float x,y,size;
        Marker(float x,float y,float size){this.x=x;this.y=y;this.size=size;}
        public void bounds(RectF b){add(b,x-size,y-size);add(b,x+size,y+size);}
        public void draw(Canvas c,Paint p,Matrix m){
            float[] v={x-size,y,x+size,y,x,y-size,x,y+size};m.mapPoints(v);
            c.drawLine(v[0],v[1],v[2],v[3],p);c.drawLine(v[4],v[5],v[6],v[7],p);
        }
    }

    private static final class EllipseCurve implements Entity{
        final float cx,cy,mx,my,ratio,start,end;
        EllipseCurve(float cx,float cy,float mx,float my,float ratio,float start,float end){
            this.cx=cx;this.cy=cy;this.mx=mx;this.my=my;this.ratio=Math.abs(ratio);this.start=start;this.end=end;
        }
        private PointF at(double t){
            double co=Math.cos(t),si=Math.sin(t);
            return new PointF((float)(cx+mx*co-my*ratio*si),(float)(cy+my*co+mx*ratio*si));
        }
        private double sweep(){double s=end-start;while(s<=0)s+=Math.PI*2;return Math.min(s,Math.PI*2);}
        public void bounds(RectF b){
            double sw=sweep();for(int i=0;i<=96;i++){PointF q=at(start+sw*i/96d);add(b,q.x,q.y);}
        }
        public void draw(Canvas c,Paint p,Matrix m){
            double sw=sweep();Path path=new Path();
            for(int i=0;i<=96;i++){
                PointF q=at(start+sw*i/96d);float[]v={q.x,q.y};m.mapPoints(v);
                if(i==0)path.moveTo(v[0],v[1]);else path.lineTo(v[0],v[1]);
            }
            c.drawPath(path,p);
        }
    }

    private static final class Composite implements Entity {
        final ArrayList<Entity> items;
        Composite(ArrayList<Entity> items){this.items=items;}
        public void bounds(RectF b){for(Entity item:items)item.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){for(Entity item:items)item.draw(c,p,m);}
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
            Result result=renderLayers(document,view,layerNames,visible,skippedCount);
            result.conversionWarnings=conversionWarnings;
            return result;
        }

        /** Draw retained DXF geometry directly to the target canvas so zoom stays sharp. */
        public void drawVector(Canvas canvas, Matrix imageMatrix){
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(1.5f);
            Matrix combined=new Matrix();
            combined.setConcat(imageMatrix,view);
            for(Entity entity:document){
                LayerEntity layer=(LayerEntity)entity;
                if(visibleLayers.contains(layer.layer))layer.draw(canvas,paint,combined);
            }
        }

        public int contentWidth(){return SIZE;}
        public int contentHeight(){return SIZE;}
    }

    private static final class LayerEntity implements Entity {
        final Entity entity; final String layer; final int color;
        LayerEntity(Entity e,String l,int color){entity=e;layer=l;this.color=color;}
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){p.setColor(color);entity.draw(c,p,m);}
    }

    private static final class Label implements Entity {
        final float x,y,height,angle; final String[] rows;
        Label(float x,float y,float h,float angle,String text){
            this.x=x;this.y=y;this.height=h;this.angle=angle;rows=text.split("\n",-1);
        }
        private Path shape(){
            Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);p.setTextSize(height);
            Path shape=new Path();
            for(int i=0;i<rows.length;i++){
                Path line=new Path();p.getTextPath(rows[i],0,rows[i].length(),0,i*height*1.3f,line);shape.addPath(line);
            }
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

    private static Map<String,Integer> readLayerColors(List<String> tags){
        HashMap<String,Integer> colors=new HashMap<>();String section="";
        for(int i=0;i<tags.size();){
            if(intOf(tags.get(i))!=0){i+=2;continue;}
            String type=tags.get(i+1).trim();int from=i+2;i=from;
            while(i<tags.size()&&intOf(tags.get(i))!=0)i+=2;
            if("SECTION".equals(type)){section=str(tags,from,i,2,"").trim();continue;}
            if("ENDSEC".equals(type)){section="";continue;}
            if("TABLES".equals(section)&&"LAYER".equals(type)){
                String name=str(tags,from,i,2,"0").trim();
                int aci=(int)fv(tags,from,i,62,7f);int trueColor=DxfColor.NO_TRUE_COLOR;
                String raw=str(tags,from,i,420,"").trim();
                if(!raw.isEmpty())try{trueColor=(int)Long.parseLong(raw);}catch(NumberFormatException ignored){}
                colors.put(DxfColor.key(name),DxfColor.layerArgb(aci,trueColor));
            }
        }
        colors.putIfAbsent(DxfColor.key("0"),DxfColor.aciArgb(7));return colors;
    }

    public static Result render(File file)throws IOException{
        // LibreDWG can expand a modest DWG into a very large ASCII DXF.
        // Avoid retaining millions of DXF text lines on mobile devices.
        if(file.length()>8L*