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
        private final RectF contentBounds;
        private final float worldToContentScale;
        private final double millimetersPerUnit;
        private final String drawingUnitName;

        Result(Bitmap b,int e,int skipped,float[] points,List<Entity> document,Matrix view,Set<String> all,Set<String> visible,
               RectF contentBounds,float worldToContentScale,double millimetersPerUnit,String drawingUnitName){
            bitmap=b;entityCount=e;skippedCount=skipped;snapPoints=points;
            this.document=document;this.view=new Matrix(view);this.contentBounds=new RectF(contentBounds);
            this.worldToContentScale=worldToContentScale;this.millimetersPerUnit=millimetersPerUnit;this.drawingUnitName=drawingUnitName;
            layerNames=Collections.unmodifiableSet(new TreeSet<>(all));
            visibleLayers=Collections.unmodifiableSet(new TreeSet<>(visible));layerCount=all.size();
        }

        public Result withVisibleLayers(Set<String> selected)throws IOException{
            Set<String> visible=new HashSet<>(selected);visible.retainAll(layerNames);
            Result result=renderLayers(document,view,layerNames,visible,skippedCount,contentBounds,worldToContentScale,millimetersPerUnit,drawingUnitName);
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

        /** Draws a paper-friendly vector copy. White CAD geometry becomes black on white paper. */
        public void drawVectorForPrint(Canvas canvas, Matrix contentToPage, boolean monochrome){
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(.8f);
            Matrix combined=new Matrix();combined.setConcat(contentToPage,view);
            for(Entity entity:document){
                LayerEntity layer=(LayerEntity)entity;
                if(!visibleLayers.contains(layer.layer))continue;
                paint.setColor(monochrome?Color.BLACK:paperColor(layer.color));
                layer.entity.draw(canvas,paint,combined);
            }
        }

        /** Content-space to physical print-page matrix. denominator=0 means fit to page. */
        public Matrix printMatrix(RectF target,int denominator){
            Matrix matrix=new Matrix();
            if(target==null||target.width()<=0||target.height()<=0)return matrix;
            double pointsPerWorld=CadPrintMath.pointsPerDrawingUnit(millimetersPerUnit,denominator);
            if(denominator>0&&Double.isFinite(pointsPerWorld)&&worldToContentScale>0){
                float scale=(float)(pointsPerWorld/worldToContentScale);
                matrix.setScale(scale,scale);
                RectF mapped=new RectF(contentBounds);matrix.mapRect(mapped);
                matrix.postTranslate(target.centerX()-mapped.centerX(),target.centerY()-mapped.centerY());
            }else{
                matrix.setRectToRect(contentBounds,target,Matrix.ScaleToFit.CENTER);
            }
            return matrix;
        }

        public boolean hasPhysicalUnits(){return Double.isFinite(millimetersPerUnit)&&millimetersPerUnit>0;}
        public String drawingUnitName(){return drawingUnitName;}
        public float drawingAspectRatio(){return contentBounds.height()>0?contentBounds.width()/contentBounds.height():1f;}
        public int contentWidth(){return SIZE;}
        public int contentHeight(){return SIZE;}
    }

    /** Retains the source DXF color already resolved from ACI/TrueColor/BYLAYER/BYBLOCK. */
    private static final class LayerEntity implements Entity {
        final Entity entity; final String layer; final int color;
        LayerEntity(Entity e,String l,int color){entity=e;layer=l;this.color=color;}
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){p.setColor(color);entity.draw(c,p,m);}
    }

    private static int paperColor(int color){
        int r=Color.red(color),g=Color.green(color),b=Color.blue(color);
        return r>=245&&g>=245&&b>=245?Color.BLACK:Color.rgb(r,g,b);
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

    public static Result render(File file)throws IOException{
        List<String> lines=readLines(file);
        int insUnits=headerInt(lines,"$INSUNITS",0);
        double millimetersPerUnit=CadPrintMath.millimetersPerUnit(insUnits);
        String drawingUnitName=CadPrintMath.unitName(insUnits);
        DxfBlocks.Result expanded=DxfBlocks.expand(lines);
        ArrayList<Entity> entities=new ArrayList<>();Set<String> layers=new HashSet<>();
        int skipped=expanded.skipped;
        List<DxfBlocks.Placement> placements=expanded.placements;
        for(int index=0;index<placements.size();index++){
            FileTransfer.checkCancelled();
            DxfBlocks.Placement item=placements.get(index);
            Entity entity;
            if("POLYLINE".equals(item.record.type)){
                ArrayList<PointF> points=new ArrayList<>();int j=index+1;
                while(j<placements.size()&&"VERTEX".equals(placements.get(j).record.type)){
                    DxfBlocks.Record vertex=placements.get(j).record;
                    points.add(new PointF(f(lines,vertex.from,vertex.to,10),f(lines,vertex.from,vertex.to,20)));j++;
                }
                if(points.size()<2){skipped++;continue;}
                entity=new Poly(points,(((int)item.record.number(70,0))&1)!=0);index=j-1;
            }else if("VERTEX".equals(item.record.type)){
                skipped++;continue;
            }else{
                entity=parse(item.record.type,lines,item.record.from,item.record.to);
                if(entity==null){skipped++;continue;}
            }
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,item.color));layers.add(item.layer);
        }
        if(entities.isEmpty())return null;
        RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
        for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}
        if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;
        if(b.width()==0){b.left-=.5f;b.right+=.5f;}
        if(b.height()==0){b.top-=.5f;b.bottom+=.5f;}
        float s=Math.min((SIZE-2f*MARGIN)/b.width(),(SIZE-2f*MARGIN)/b.height());
        Matrix m=new Matrix();m.postTranslate(-b.left,-b.bottom);m.postScale(s,-s);
        m.postTranslate(MARGIN+(SIZE-2*MARGIN-b.width()*s)/2f,MARGIN+(SIZE-2*MARGIN-b.height()*s)/2f);
        RectF contentBounds=new RectF(b);m.mapRect(contentBounds);
        return renderLayers(entities,m,layers,layers,skipped,contentBounds,s,millimetersPerUnit,drawingUnitName);
    }

    private static Result renderLayers(List<Entity> document,Matrix view,Set<String> all,Set<String> visible,int skipped,
                                       RectF contentBounds,float worldToContentScale,double millimetersPerUnit,String drawingUnitName)throws IOException{
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
            return new Result(bitmap,shown.size(),skipped,snapPoints(shown,view),document,view,all,visible,
                contentBounds,worldToContentScale,millimetersPerUnit,drawingUnitName);
        }catch(IOException|RuntimeException|OutOfMemoryError e){bitmap.recycle();throw e;}
    }

    private static float[] snapPoints(List<Entity> entities,Matrix matrix){
        ArrayList<PointF> points=new ArrayList<>();
        for(Entity wrapped:entities){
            Entity entity=wrapped instanceof LayerEntity?((LayerEntity)wrapped).entity:wrapped;
            Matrix transform=new Matrix();
            if(entity instanceof Transformed){transform=((Transformed)entity).matrix;entity=((Transformed)entity).entity;}
            ArrayList<PointF> local=new ArrayList<>();
            if(entity instanceof Line){
                Line line=(Line)entity;
                local.add(new PointF(line.x1,line.y1));local.add(new PointF(line.x2,line.y2));
            }else if(entity instanceof Poly){local.addAll(((Poly)entity).pts);}
            for(PointF point:local){
                float[] xy={point.x,point.y};transform.mapPoints(xy);points.add(new PointF(xy[0],xy[1]));
            }
        }
        float[] result=new float[points.size()*2];
        for(int i=0;i<points.size();i++){result[i*2]=points.get(i).x;result[i*2+1]=points.get(i).y;}
        matrix.mapPoints(result);return result;
    }

    private static Entity parse(String type,List<String>a,int from,int to){
        if("TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)){
            StringBuilder text=new StringBuilder();
            for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==1||("MTEXT".equals(type)&&code==3))text.append(a.get(i+1));}
            String plain=DxfText.plain(text.toString());
            if(plain.trim().isEmpty())return null;
            float angle=f(a,from,to,50);
            if("MTEXT".equals(type)){
                angle=(float)Math.toDegrees(angle);
                if(!str(a,from,to,11,"").isEmpty())angle=(float)Math.toDegrees(Math.atan2(f(a,from,to,21),f(a,from,to,11)));
            }
            return new Label(f(a,from,to,10),f(a,from,to,20),Math.max(.01f,fv(a,from,to,40,1f)),angle,plain);
        }
        if("LINE".equals(type))return new Line(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21));
        if("POINT".equals(type))return new Marker(f(a,from,to,10),f(a,from,to,20),Math.max(.1f,fv(a,from,to,40,.5f)));
        if("CIRCLE".equals(type))return new Circle(f(a,from,to,10),f(a,from,to,20),Math.abs(f(a,from,to,40)),0,360);
        if("ARC".equals(type)){
            float start=f(a,from,to,50),end=f(a,from,to,51),sweep=end-start;if(sweep<0)sweep+=360;
            return new Circle(f(a,from,to,10),f(a,from,to,20),Math.abs(f(a,from,to,40)),start,sweep);
        }
        if("ELLIPSE".equals(type)){
            float mx=f(a,from,to,11),my=f(a,from,to,21);if(Math.hypot(mx,my)<1e-6)return null;
            return new EllipseCurve(f(a,from,to,10),f(a,from,to,20),mx,my,fv(a,from,to,40,1f),
                fv(a,from,to,41,0f),fv(a,from,to,42,(float)(Math.PI*2)));
        }
        if("LWPOLYLINE".equals(type)){
            ArrayList<PointF>p=repeatedPoints(a,from,to,10,20);
            return p.size()<2?null:new Poly(p,(((int)f(a,from,to,70))&1)!=0);
        }
        if("SPLINE".equals(type)){
            ArrayList<PointF>p=repeatedPoints(a,from,to,11,21);if(p.size()<2)p=repeatedPoints(a,from,to,10,20);
            return p.size()<2?null:new Poly(p,false);
        }
        if("LEADER".equals(type)){
            ArrayList<PointF>p=repeatedPoints(a,from,to,10,20);return p.size()<2?null:new Poly(p,false);
        }
        if("HATCH".equals(type)){
            ArrayList<PointF>p=repeatedPoints(a,from,to,10,20);return p.size()<2?null:new Poly(p,true);
        }
        if("SOLID".equals(type)||"TRACE".equals(type)||"3DFACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<2?null:new Poly(p,true);
        }
        if("DIMENSION".equals(type)){
            ArrayList<PointF>p=new ArrayList<>();
            addPointIfPresent(p,a,from,to,13,23);addPointIfPresent(p,a,from,to,14,24);addPointIfPresent(p,a,from,to,10,20);
            return p.size()<2?null:new Poly(p,false);
        }
        return null;
    }

    private static ArrayList<PointF> repeatedPoints(List<String>a,int from,int to,int xCode,int yCode){
        ArrayList<PointF>p=new ArrayList<>();Float x=null;
        for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==xCode)x=floatOf(a.get(i+1));else if(code==yCode&&x!=null){p.add(new PointF(x,floatOf(a.get(i+1))));x=null;}}
        return p;
    }

    private static ArrayList<PointF> numberedPoints(List<String>a,int from,int to,int xBase,int yBase,int count){
        ArrayList<PointF>p=new ArrayList<>();for(int i=0;i<count;i++)addPointIfPresent(p,a,from,to,xBase+i,yBase+i);return p;
    }

    private static void addPointIfPresent(ArrayList<PointF> p,List<String>a,int from,int to,int xCode,int yCode){
        if(has(a,from,to,xCode)&&has(a,from,to,yCode))p.add(new PointF(f(a,from,to,xCode),f(a,from,to,yCode)));
    }

    private static boolean has(List<String>a,int from,int to,int wanted){
        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted)return true;return false;
    }

    private static int headerInt(List<String>a,String variable,int fallback){
        for(int i=0;i+1<a.size();i+=2){
            if(intOf(a.get(i))!=9||!variable.equals(a.get(i+1).trim()))continue;
            for(int j=i+2;j+1<a.size();j+=2){
                int code=intOf(a.get(j));if(code==9||code==0)break;
                if(code==70)return intOf(a.get(j+1));
            }
        }
        return fallback;
    }

    private static List<String>readLines(File f)throws IOException{
        ArrayList<String>r=new ArrayList<>();
        try(BufferedReader b=new BufferedReader(new InputStreamReader(new FileInputStream(f),charset(f)))){
            String s;while((s=b.readLine())!=null){FileTransfer.checkCancelled();if(r.size()>=600000)throw new IOException("DXF etiket sınırı aşıldı");r.add(s);}
        }
        return r;
    }
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return-1;}}
    private static float floatOf(String s){try{return Float.parseFloat(s.trim());}catch(Exception e){return 0;}}
    private static float f(List<String>a,int from,int to,int wanted){for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted)return floatOf(a.get(i+1));return 0;}
    private static float fv(List<String>a,int from,int to,int wanted,float fallback){for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted)return floatOf(a.get(i+1));return fallback;}
    private static void add(RectF b,float x,float y){b.left=Math.min(b.left,x);b.top=Math.min(b.top,y);b.right=Math.max(b.right,x);b.bottom=Math.max(b.bottom,y);}

    private static java.nio.charset.Charset charset(File file)throws IOException {
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
