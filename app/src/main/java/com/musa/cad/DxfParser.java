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

    /** Flattened entity with its effective AutoCAD color already resolved. */
    private static final class LayerEntity implements Entity {
        final Entity entity; final String layer; final int color;
        LayerEntity(Entity e,String l,int color){entity=e;layer=l;this.color=color;}
        public void bounds(RectF b){entity.bounds(b);}
        public void draw(Canvas c,Paint p,Matrix m){
            p.setColor(color);
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
        // LibreDWG can expand a modest DWG into a very large ASCII DXF.
        // Avoid retaining millions of DXF text lines on mobile devices.
        if(file.length()>8L*1024*1024)return renderStreaming(file);
        try{return renderBuffered(file);}
        catch(IOException e){
            if(e.getMessage()!=null&&e.getMessage().contains("etiket sınırı"))return renderStreaming(file);
            throw e;
        }
    }

    private static Result renderBuffered(File file)throws IOException{
        List<String> lines=readLines(file);
        Map<String,Integer> layerColors=readLayerColors(lines);
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
            int color=DxfColor.argb(item.color,layerColors);
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,color));layers.add(item.layer);
        }
        return finishEntities(entities,layers,skipped);
    }

    private interface StreamNode {}
    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer;final int aci,trueColor;
        StreamShape(Entity entity,String layer,int aci,int trueColor){
            this.entity=entity;this.layer=layer;this.aci=aci;this.trueColor=trueColor;
        }
    }
    private static final class StreamInsert implements StreamNode {
        final String name,layer;final int aci,trueColor;
        final double x,y,z,sx,sy,rotation,ex,ey,ez;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            x=r.number(10,0);y=r.number(20,0);z=r.number(30,0);
            sx=r.number(41,1);sy=r.number(42,1);rotation=r.number(50,0);
            columns=(int)r.number(70,1);rows=(int)r.number(71,1);
            ex=r.number(210,0);ey=r.number(220,0);ez=r.number(230,1);
        }
    }
    private static final class StreamDimension implements StreamNode {
        final String name,layer;final int aci,trueColor;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
        }
    }
    private static final class StreamBlock {
        final String name;final double bx,by,bz;final int flags;final String xref;
        final ArrayList<StreamNode> members=new ArrayList<>();
        StreamBlock(StreamRecord r)throws IOException{
            name=key(r.text(2,""));bx=r.number(10,0);by=r.number(20,0);bz=r.number(30,0);
            flags=(int)r.number(70,0);xref=r.text(1,"");
        }
    }
    private static final class StreamRecord {
        final ArrayList<String> tags=new ArrayList<>();
        void add(int code,String value){tags.add(Integer.toString(code));tags.add(value);}
        String text(int code,String fallback){
            for(int i=0;i+1<tags.size();i+=2)if(intOf(tags.get(i))==code)return tags.get(i+1).trim();
            return fallback;
        }
        int integer(int code,int fallback)throws IOException{
            String value=text(code,Integer.toString(fallback));
            try{return Integer.parseInt(value);}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
        }
        int trueColor()throws IOException{
            String value=text(420,"").trim();if(value.isEmpty())return DxfColor.NO_TRUE_COLOR;
            try{return DxfColor.trueColor(Long.parseLong(value));}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF TrueColor değeri",e);}
        }
        double number(int code,double fallback)throws IOException{
            String value=text(code,Double.toString(fallback));
            try{double n=Double.parseDouble(value);if(!Double.isFinite(n))throw new NumberFormatException();return n;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}
        }
    }
    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer;final boolean closed;final int aci,trueColor;
        final ArrayList<PointF> points=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,boolean closed,int aci,int trueColor){
            this.target=target;this.layer=layer;this.closed=closed;this.aci=aci;this.trueColor=trueColor;
        }
    }
    private static final class StreamContext {
        String section="";StreamBlock activeBlock;PendingPoly pending;int skipped;
        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final HashMap<String,Integer> layerColors=new HashMap<>();
        final ArrayList<StreamNode> roots=new ArrayList<>();
    }
    private static final class StreamCounter {int visits;}

    /** Memory-bounded parser used for large ASCII DXF files produced by DWG conversion. */
    private static Result renderStreaming(File file)throws IOException{
        StreamContext context=new StreamContext();
        try(BufferedReader reader=new BufferedReader(new InputStreamReader(new FileInputStream(file),charset(file)),128*1024)){
            String type=null;StreamRecord record=null;
            while(true){
                FileTransfer.checkCancelled();
                String codeLine=reader.readLine();if(codeLine==null)break;
                String value=reader.readLine();if(value==null)throw new IOException("Eksik DXF etiketi");
                final int code;try{code=Integer.parseInt(codeLine.trim());}catch(NumberFormatException e){throw new IOException("Geçersiz ASCII DXF etiketi",e);}
                if(code==0){
                    if(type!=null)processStreamRecord(type,record,context);
                    type=value.trim();record=new StreamRecord();
                }else if(record!=null&&keepStreamCode(code)){
                    record.add(code,value);
                }
            }
            if(type!=null)processStreamRecord(type,record,context);
        }
        finishPending(context);
        ArrayList<Entity> entities=new ArrayList<>();Set<String> layers=new HashSet<>();
        StreamCounter counter=new StreamCounter();
        expandStream(context.roots,new DxfBlocks.Transform(),"0",null,context.blocks,new HashSet<>(),entities,layers,counter,context);
        return finishEntities(entities,layers,context.skipped);
    }

    private static boolean keepStreamCode(int code){
        return code==1||code==2||code==3||code==8||code==62||code==420||(code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||code==210||code==220||code==230;
    }

    private static ArrayList<StreamNode> streamTarget(StreamContext c){
        if("ENTITIES".equals(c.section))return c.roots;
        if("BLOCKS".equals(c.section)&&c.activeBlock!=null)return c.activeBlock.members;
        return null;
    }

    private static void processStreamRecord(String type,StreamRecord r,StreamContext c)throws IOException{
        if("SECTION".equals(type)){finishPending(c);c.section=r.text(2,"");c.activeBlock=null;return;}
        if("ENDSEC".equals(type)){finishPending(c);c.section="";c.activeBlock=null;return;}
        if("EOF".equals(type)){finishPending(c);return;}
        if("BLOCKS".equals(c.section)){
            if("BLOCK".equals(type)){
                finishPending(c);StreamBlock block=new StreamBlock(r);c.activeBlock=block;
                if(!block.name.isEmpty())c.blocks.put(block.name,block);return;
            }
            if("ENDBLK".equals(type)){finishPending(c);c.activeBlock=null;return;}
        }
        if("LAYER".equals(type)){
            String layerName=DxfColor.key(r.text(2,"0"));
            c.layerColors.put(layerName,DxfColor.layerArgb(r.integer(62,7),r.trueColor()));return;
        }
        ArrayList<StreamNode> target=streamTarget(c);if(target==null)return;
        if(c.pending!=null){
            if("VERTEX".equals(type)){c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));return;}
            if("SEQEND".equals(type)){finishPending(c);return;}
            finishPending(c);
        }
        if("POLYLINE".equals(type)){
            c.pending=new PendingPoly(target,r.text(8,"0"),(((int)r.number(70,0))&1)!=0,
                r.integer(62,DxfColor.BYLAYER),r.trueColor());return;
        }
        if("VERTEX".equals(type)||"SEQEND".equals(type))return;
        if("INSERT".equals(type)){target.add(new StreamInsert(r));return;}
        if("DIMENSION".equals(type)){target.add(new StreamDimension(r));return;}
        Entity entity=parse(type,r.tags,0,r.tags.size());
        if(entity==null)c.skipped++;else target.add(new StreamShape(entity,r.text(8,"0"),
            r.integer(62,DxfColor.BYLAYER),r.trueColor()));
    }

    private static void finishPending(StreamContext c){
        PendingPoly p=c.pending;if(p==null)return;c.pending=null;
        if(p.points.size()<2)c.skipped++;else p.target.add(new StreamShape(new Poly(p.points,p.closed),p.layer,p.aci,p.trueColor));
    }

    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,
                                     StreamCounter counter,StreamContext context)throws IOException{
        for(StreamNode node:nodes){
            FileTransfer.checkCancelled();
            if(++counter.visits>500000)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");
            if(node instanceof StreamShape){
                StreamShape shape=(StreamShape)node;String layer="0".equals(shape.layer)?parentLayer:shape.layer;
                DxfColor.Ref ref=DxfColor.resolve(shape.aci,shape.trueColor,layer,byBlockColor);
                int color=DxfColor.argb(ref,context.layerColors);
                entities.add(new LayerEntity(new Transformed(shape.entity,parent),layer,color));layers.add(layer);continue;
            }
            if(node instanceof StreamDimension){
                StreamDimension dimension=(StreamDimension)node;String layer="0".equals(dimension.layer)?parentLayer:dimension.layer;
                StreamBlock block=blocks.get(dimension.name);
                if(block==null||stack.contains(dimension.name)||stack.size()>=32||block.bz!=0||(block.flags&12)!=0||!block.xref.isEmpty()){
                    context.skipped++;continue;
                }
                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                stack.add(dimension.name);
                expandStream(block.members,parent,layer,dimColor,blocks,stack,entities,layers,counter,context);
                stack.remove(dimension.name);continue;
            }
            StreamInsert insert=(StreamInsert)node;String layer="0".equals(insert.layer)?parentLayer:insert.layer;
            StreamBlock block=blocks.get(insert.name);
            if(block==null||stack.contains(insert.name)||stack.size()>=32||insert.columns!=1||insert.rows!=1||insert.z!=0||
                insert.ex!=0||insert.ey!=0||insert.ez!=1||block.bz!=0||(block.flags&12)!=0||!block.xref.isEmpty()||insert.sx==0||insert.sy==0){
                context.skipped++;continue;
            }
            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            DxfBlocks.Transform local=DxfBlocks.Transform.insert(block.bx,block.by,insert.sx,insert.sy,insert.rotation,insert.x,insert.y);
            DxfBlocks.Transform transform=parent.thenLocal(local);stack.add(insert.name);
            expandStream(block.members,transform,layer,insertColor,blocks,stack,entities,layers,counter,context);
            stack.remove(insert.name);
        }
    }

    private static String key(String value){return value.toUpperCase(Locale.ROOT);}

    private static Result finishEntities(ArrayList<Entity> entities,Set<String> layers,int skipped)throws IOException{
        if(entities.isEmpty())return null;
        RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
        for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}
        if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;
        if(b.width()==0){b.left-=.5f;b.right+=.5f;}
        if(b.height()==0){b.top-=.5f;b.bottom+=.5f;}
        float s=Math.min((SIZE-2f*MARGIN)/b.width(),(SIZE-2f*MARGIN)/b.height());
        Matrix m=new Matrix();m.postTranslate(-b.left,-b.bottom);m.postScale(s,-s);
        m.postTranslate(MARGIN+(SIZE-2*MARGIN-b.width()*s)/2f,MARGIN+(SIZE-2*MARGIN-b.height()*s)/2f);
        return renderLayers(entities,m,layers,layers,skipped);
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

    private static Map<String,Integer> readLayerColors(List<String>a)throws IOException{
        HashMap<String,Integer> colors=new HashMap<>();
        for(int i=0;i+1<a.size();i+=2){
            if(intOf(a.get(i))!=0||!"LAYER".equals(a.get(i+1).trim()))continue;
            int from=i+2,to=from;while(to<a.size()&&intOf(a.get(to))!=0)to+=2;
            String name=DxfColor.key(str(a,from,to,2,"0").trim());
            int aci=(int)fv(a,from,to,62,7f);int trueColor=DxfColor.NO_TRUE_COLOR;
            String raw=str(a,from,to,420,"").trim();
            if(!raw.isEmpty())try{trueColor=DxfColor.trueColor(Long.parseLong(raw));}catch(NumberFormatException ignored){}
            colors.put(name,DxfColor.layerArgb(aci,trueColor));
            i=to-2;
        }
        return colors;
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
        if("HATCH".equals(type))return parseHatch(a,from,to);
        if("SOLID".equals(type)||"TRACE".equals(type)||"3DFACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<2?null:new Poly(p,true);
        }
        // DIMENSION graphics are expanded from their anonymous *D blocks before parse().
        if("DIMENSION".equals(type))return null;
        return null;
    }

    /**
     * HATCH has an elevation point (10/20) before its boundary paths. Treating every
     * 10/20 pair as one polygon connects that elevation origin to unrelated loops and
     * creates the long "spider web" lines seen in some DWGs. Parse each boundary path
     * independently instead. Bulged polyline vertices are currently shown as straight
     * boundary chords; importantly, separate loops are never joined together.
     */
    private static Entity parseHatch(List<String>a,int from,int to){
        int pathCount=(int)fv(a,from,to,91,0f);if(pathCount<=0)return null;
        int i=from;ArrayList<Entity> paths=new ArrayList<>();
        for(int pathIndex=0;pathIndex<pathCount;pathIndex++){
            while(i+1<to&&intOf(a.get(i))!=92)i+=2;
            if(i+1>=to)break;
            int flags=(int)floatOf(a.get(i+1));i+=2;
            if((flags&2)!=0){
                boolean closed=true;int vertexCount=-1;
                while(i+1<to){
                    int code=intOf(a.get(i));
                    if(code==73)closed=((int)floatOf(a.get(i+1)))!=0;
                    if(code==93){vertexCount=(int)floatOf(a.get(i+1));i+=2;break;}
                    if(code==92)break;i+=2;
                }
                ArrayList<PointF> points=new ArrayList<>();
                for(int v=0;v<vertexCount&&i+1<to;v++){
                    while(i+1<to&&intOf(a.get(i))!=10){
                        int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;
                    }
                    if(i+1>=to||intOf(a.get(i))!=10)break;
                    float x=floatOf(a.get(i+1));i+=2;
                    while(i+1<to&&intOf(a.get(i))!=20){
                        int code=intOf(a.get(i));if(code==92||code==97||code==10)break;i+=2;
                    }
                    if(i+1>=to||intOf(a.get(i))!=20)break;
                    float y=floatOf(a.get(i+1));i+=2;
                    if(Float.isFinite(x)&&Float.isFinite(y))points.add(new PointF(x,y));
                    if(i+1<to&&intOf(a.get(i))==42)i+=2;
                }
                if(points.size()>=2)paths.add(new Poly(points,closed));
            }else{
                int edgeCount=-1;
                while(i+1<to){
                    int code=intOf(a.get(i));
                    if(code==93){edgeCount=(int)floatOf(a.get(i+1));i+=2;break;}
                    if(code==92)break;i+=2;
                }
                for(int edge=0;edge<edgeCount&&i+1<to;edge++){
                    while(i+1<to&&intOf(a.get(i))!=72){
                        int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;
                    }
                    if(i+1>=to||intOf(a.get(i))!=72)break;
                    int edgeType=(int)floatOf(a.get(i+1));int edgeFrom=i+2;i=edgeFrom;
                    while(i+1<to){
                        int code=intOf(a.get(i));
                        if(code==72||code==92||code==97)break;i+=2;
                    }
                    int edgeTo=i;Entity edgeEntity=null;
                    if(edgeType==1&&has(a,edgeFrom,edgeTo,10)&&has(a,edgeFrom,edgeTo,20)&&has(a,edgeFrom,edgeTo,11)&&has(a,edgeFrom,edgeTo,21)){
                        edgeEntity=new Line(f(a,edgeFrom,edgeTo,10),f(a,edgeFrom,edgeTo,20),f(a,edgeFrom,edgeTo,11),f(a,edgeFrom,edgeTo,21));
                    }else if(edgeType==2&&has(a,edgeFrom,edgeTo,10)&&has(a,edgeFrom,edgeTo,20)&&has(a,edgeFrom,edgeTo,40)){
                        float start=f(a,edgeFrom,edgeTo,50),end=f(a,edgeFrom,edgeTo,51);boolean ccw=((int)fv(a,edgeFrom,edgeTo,73,1f))!=0;
                        float sweep=end-start;while(sweep<0)sweep+=360f;if(!ccw){float oldStart=start;start=end;sweep=oldStart-end;while(sweep<0)sweep+=360f;}
                        edgeEntity=new Circle(f(a,edgeFrom,edgeTo,10),f(a,edgeFrom,edgeTo,20),Math.abs(f(a,edgeFrom,edgeTo,40)),start,sweep);
                    }else if(edgeType==3&&has(a,edgeFrom,edgeTo,10)&&has(a,edgeFrom,edgeTo,20)&&has(a,edgeFrom,edgeTo,11)&&has(a,edgeFrom,edgeTo,21)){
                        float start=(float)Math.toRadians(f(a,edgeFrom,edgeTo,50)),end=(float)Math.toRadians(f(a,edgeFrom,edgeTo,51));
                        edgeEntity=new EllipseCurve(f(a,edgeFrom,edgeTo,10),f(a,edgeFrom,edgeTo,20),f(a,edgeFrom,edgeTo,11),f(a,edgeFrom,edgeTo,21),fv(a,edgeFrom,edgeTo,40,1f),start,end);
                    }else if(edgeType==4){
                        ArrayList<PointF> points=repeatedPoints(a,edgeFrom,edgeTo,10,20);if(points.size()>=2)edgeEntity=new Poly(points,false);
                    }
                    if(edgeEntity!=null)paths.add(edgeEntity);
                }
            }
        }
        if(paths.isEmpty())return null;
        return paths.size()==1?paths.get(0):new Composite(paths);
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
