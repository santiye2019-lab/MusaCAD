package com.musa.cad;

import android.graphics.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Lightweight ASCII DXF renderer for common 2D entities. */
public final class DxfParser {
    private static final int SIZE=2400,MARGIN=80;
    private static final int BACKGROUND=Color.rgb(18,24,30);
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

    /** Applies DXF geometric polyline width in drawing units without changing entity lineweight semantics. */
    private static final class GeometricWidth implements Entity{
        final Entity entity;final double width;
        GeometricWidth(Entity entity,double width){this.entity=entity;this.width=DxfPolylineWidth.constant(width);}
        public void bounds(RectF b){
            RectF local=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);entity.bounds(local);
            if(local.left>local.right||local.top>local.bottom)return;float h=(float)(width*.5);
            add(b,local.left-h,local.top-h);add(b,local.right+h,local.bottom+h);
        }
        public void draw(Canvas c,Paint p,Matrix m){
            float[] basis={1,0,0,1};m.mapVectors(basis);double sx=Math.hypot(basis[0],basis[1]),sy=Math.hypot(basis[2],basis[3]);
            double device=DxfPolylineWidth.device(width,sx,sy);if(device<=0){entity.draw(c,p,m);return;}
            float oldWidth=p.getStrokeWidth();Paint.Cap oldCap=p.getStrokeCap();Paint.Join oldJoin=p.getStrokeJoin();
            p.setStrokeWidth((float)device);p.setStrokeCap(Paint.Cap.BUTT);p.setStrokeJoin(Paint.Join.MITER);entity.draw(c,p,m);
            p.setStrokeWidth(oldWidth);p.setStrokeCap(oldCap);p.setStrokeJoin(oldJoin);
        }
    }

    /** Filled geometry for DXF per-segment start/end polyline widths. */
    private static final class VariableWidth implements Entity{
        final Entity centerline;final ArrayList<ArrayList<PointF>> outlines=new ArrayList<>();
        VariableWidth(Entity centerline,double[][] packed){
            this.centerline=centerline;
            if(packed!=null)for(double[] polygon:packed){ArrayList<PointF> pts=packedPoints(polygon);if(pts.size()>=3)outlines.add(pts);}
        }
        public void bounds(RectF b){
            if(outlines.isEmpty()){centerline.bounds(b);return;}
            for(ArrayList<PointF> polygon:outlines)for(PointF q:polygon)add(b,q.x,q.y);
        }
        public void draw(Canvas c,Paint p,Matrix m){
            if(outlines.isEmpty()){centerline.draw(c,p,m);return;}
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();p.setPathEffect(null);p.setStyle(Paint.Style.FILL);
            for(ArrayList<PointF> polygon:outlines){
                Path path=new Path();float[] v={polygon.get(0).x,polygon.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
                for(int i=1;i<polygon.size();i++){v[0]=polygon.get(i).x;v[1]=polygon.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();c.drawPath(path,p);
            }
            p.setStyle(oldStyle);p.setPathEffect(oldEffect);
        }
    }

    private static final class LeaderEntity implements Entity{
        final Poly line;final ArrayList<PointF> arrow;
        LeaderEntity(ArrayList<PointF> points,ArrayList<PointF> arrow){this.line=new Poly(points,false);this.arrow=arrow==null?new ArrayList<>():arrow;}
        public void bounds(RectF b){line.bounds(b);for(PointF q:arrow)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            line.draw(c,p,m);if(arrow.size()<3)return;
            Path path=new Path();float[] v={arrow.get(0).x,arrow.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
            for(int i=1;i<arrow.size();i++){v[0]=arrow.get(i).x;v[1]=arrow.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();p.setPathEffect(null);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);
            p.setStyle(oldStyle);p.setPathEffect(oldEffect);
        }
    }

    private static final class InfiniteEntity implements Entity{
        final float x,y,dx,dy;final boolean ray;
        InfiniteEntity(float x,float y,float dx,float dy,boolean ray){this.x=x;this.y=y;this.dx=dx;this.dy=dy;this.ray=ray;}
        public void bounds(RectF b){
            double len=Math.hypot(dx,dy);if(len<1e-12){add(b,x,y);return;}float ux=(float)(dx/len),uy=(float)(dy/len);
            add(b,x,y);add(b,x+ux,y+uy);if(!ray)add(b,x-ux,y-uy);
        }
        public void draw(Canvas c,Paint p,Matrix m){
            float[] v={x,y,x+dx,y+dy};m.mapPoints(v);double[] q=DxfInfiniteLine.clip(v[0],v[1],v[2]-v[0],v[3]-v[1],ray,0,0,c.getWidth(),c.getHeight());
            if(q.length==4)c.drawLine((float)q[0],(float)q[1],(float)q[2],(float)q[3],p);
        }
    }

    private static final class SegmentSet implements Entity{
        final float[] xy;
        SegmentSet(double[] packed){xy=new float[packed==null?0:packed.length];for(int i=0;i<xy.length;i++)xy[i]=(float)packed[i];}
        public void bounds(RectF b){for(int i=0;i+1<xy.length;i+=2)add(b,xy[i],xy[i+1]);}
        public void draw(Canvas c,Paint p,Matrix m){if(xy.length<4)return;float[] v=xy.clone();m.mapPoints(v);c.drawLines(v,p);}
    }

    private static final class FilledPoly implements Entity{
        final ArrayList<PointF> pts;
        FilledPoly(ArrayList<PointF> pts){this.pts=pts;}
        public void bounds(RectF b){for(PointF q:pts)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(pts.size()<3)return;Path path=new Path();float[] v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
            for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();
            Paint.Style old=p.getStyle();PathEffect effect=p.getPathEffect();p.setPathEffect(null);p.setStyle(Paint.Style.FILL);c.drawPath(path,p);
            p.setStyle(old);p.setPathEffect(effect);
        }
    }

    private static final class Wipeout implements Entity{
        final ArrayList<PointF> pts;
        Wipeout(ArrayList<PointF> pts){this.pts=pts;}
        public void bounds(RectF b){for(PointF q:pts)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(pts.size()<3)return;Path path=new Path();float[] v={pts.get(0).x,pts.get(0).y};m.mapPoints(v);path.moveTo(v[0],v[1]);
            for(int i=1;i<pts.size();i++){v[0]=pts.get(i).x;v[1]=pts.get(i).y;m.mapPoints(v);path.lineTo(v[0],v[1]);}path.close();
            Paint.Style oldStyle=p.getStyle();int oldColor=p.getColor();float oldWidth=p.getStrokeWidth();PathEffect oldEffect=p.getPathEffect();
            p.setStyle(Paint.Style.FILL);p.setColor(BACKGROUND);p.setPathEffect(null);c.drawPath(path,p);
            p.setStyle(oldStyle);p.setColor(oldColor);p.setStrokeWidth(oldWidth);p.setPathEffect(oldEffect);
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
        final float x,y;final DxfPointStyle.Style style;
        Marker(float x,float y,DxfPointStyle.Style style){this.x=x;this.y=y;this.style=style==null?new DxfPointStyle.Style(0,0):style;}
        public void bounds(RectF b){add(b,x,y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(style.hidden())return;float[] center={x,y};m.mapPoints(center);
            float[] vectors={1,0,0,1};m.mapVectors(vectors);double deviceScale=(Math.hypot(vectors[0],vectors[1])+Math.hypot(vectors[2],vectors[3]))*.5;
            float h=(float)DxfPointStyle.deviceHalfSize(style,deviceScale,Math.min(c.getWidth(),c.getHeight()));float cx=center[0],cy=center[1];
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();p.setPathEffect(null);int base=style.base();
            if(base==0){p.setStyle(Paint.Style.FILL);c.drawCircle(cx,cy,Math.max(1f,p.getStrokeWidth()*.75f),p);p.setStyle(Paint.Style.STROKE);}
            else{p.setStyle(Paint.Style.STROKE);if(base==2){c.drawLine(cx-h,cy,cx+h,cy,p);c.drawLine(cx,cy-h,cx,cy+h,p);}
                else if(base==3){c.drawLine(cx-h,cy-h,cx+h,cy+h,p);c.drawLine(cx-h,cy+h,cx+h,cy-h,p);}
                else if(base==4)c.drawLine(cx,cy-h,cx,cy+h,p);
                else if(base!=1){p.setStyle(Paint.Style.FILL);c.drawCircle(cx,cy,Math.max(1f,p.getStrokeWidth()*.75f),p);p.setStyle(Paint.Style.STROKE);}}
            if(style.circle()){p.setStyle(Paint.Style.STROKE);c.drawCircle(cx,cy,h,p);}
            if(style.square()){p.setStyle(Paint.Style.STROKE);c.drawRect(cx-h,cy-h,cx+h,cy+h,p);}
            p.setStyle(oldStyle);p.setPathEffect(oldEffect);
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

    private static final class HatchLoop {
        final int flags;final ArrayList<PointF> points;
        HatchLoop(int flags,ArrayList<PointF> points){this.flags=flags;this.points=points;}
    }

    /** Filled/patterned HATCH clipped to its independent boundary loops. */
    private static final class Hatch implements Entity {
        final ArrayList<HatchLoop> loops;final boolean solid;final int style,solidFillAci;
        final ArrayList<DxfHatchPattern.Line> pattern;final DxfHatchGradient.Data gradient;
        Hatch(ArrayList<HatchLoop> loops,boolean solid,int style,ArrayList<DxfHatchPattern.Line> pattern,DxfHatchGradient.Data gradient){
            this(loops,solid,style,pattern,gradient,-1);
        }
        Hatch(ArrayList<HatchLoop> loops,boolean solid,int style,ArrayList<DxfHatchPattern.Line> pattern,DxfHatchGradient.Data gradient,int solidFillAci){
            this.loops=loops;this.solid=solid;this.style=style;this.pattern=pattern;this.gradient=gradient==null?DxfHatchGradient.none():gradient;this.solidFillAci=solidFillAci;
        }
        private boolean preferred(HatchLoop loop){
            if(style==2)return (loop.flags&1)!=0;              // IGNORE: external boundary only.
            if(style==1)return (loop.flags&(1|16))!=0;       // OUTER: external + outermost islands.
            return true;                                     // NORMAL: even/odd all loops.
        }
        private int preferredCount(){int n=0;for(HatchLoop l:loops)if(preferred(l))n++;return n;}
        private Path boundary(Matrix m){
            boolean fallback=preferredCount()==0;Path path=new Path();path.setFillType(Path.FillType.EVEN_ODD);
            for(HatchLoop loop:loops){
                if(!fallback&&!preferred(loop))continue;if(loop.points.size()<2)continue;
                PointF first=loop.points.get(0);path.moveTo(first.x,first.y);
                for(int i=1;i<loop.points.size();i++){PointF q=loop.points.get(i);path.lineTo(q.x,q.y);}path.close();
            }
            path.transform(m);return path;
        }
        private RectF localBounds(){
            boolean fallback=preferredCount()==0;RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            for(HatchLoop loop:loops){if(!fallback&&!preferred(loop))continue;for(PointF q:loop.points)add(b,q.x,q.y);}
            return b;
        }
        public void bounds(RectF b){for(HatchLoop loop:loops)for(PointF q:loop.points)add(b,q.x,q.y);}
        public void draw(Canvas c,Paint p,Matrix m){
            if(loops.isEmpty())return;Path clip=boundary(m);
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();
            if(gradient.enabled){
                RectF box=localBounds();if(box.left<=box.right&&box.top<=box.bottom){
                    int oldColor=p.getColor(),alpha=Color.alpha(oldColor);Shader oldShader=p.getShader();
                    int c1=(alpha<<24)|(gradient.color1&0x00ffffff),c2=(alpha<<24)|(gradient.color2&0x00ffffff);
                    p.setPathEffect(null);p.setStyle(Paint.Style.FILL);p.setShader(null);p.setColor(c1);
                    if(gradient.linear()){
                        double[] axis=DxfHatchGradient.axis(box.left,box.top,box.right,box.bottom,gradient.rotation);
                        if(axis.length==4){float[] q={(float)axis[0],(float)axis[1],(float)axis[2],(float)axis[3]};m.mapPoints(q);
                            if(Math.hypot(q[2]-q[0],q[3]-q[1])>1e-4)p.setShader(new LinearGradient(q[0],q[1],q[2],q[3],c1,c2,Shader.TileMode.CLAMP));}
                    }
                    c.drawPath(clip,p);p.setShader(oldShader);p.setColor(oldColor);p.setStyle(oldStyle);p.setPathEffect(oldEffect);return;
                }
            }
            if(solid){
                int oldColor=p.getColor();p.setPathEffect(null);p.setStyle(Paint.Style.FILL);
                if(solidFillAci>=0)p.setColor(DxfMPolygon.solidFillArgb(solidFillAci,oldColor));c.drawPath(clip,p);p.setColor(oldColor);
                p.setStyle(oldStyle);p.setPathEffect(oldEffect);return;
            }
            if(pattern.isEmpty()){
                p.setPathEffect(null);p.setStyle(Paint.Style.STROKE);c.drawPath(clip,p);
                p.setStyle(oldStyle);p.setPathEffect(oldEffect);return;
            }
            RectF box=localBounds();if(box.left>box.right||box.top>box.bottom)return;
            int save=c.save();c.clipPath(clip);p.setPathEffect(null);p.setStyle(Paint.Style.STROKE);
            int segments=0;final int segmentLimit=25000;
            outer:for(DxfHatchPattern.Line line:pattern){
                double dx=line.dx(),dy=line.dy();if(!Double.isFinite(dx)||!Double.isFinite(dy))continue;
                int[] range=DxfHatchPattern.familyRange(line,box.left,box.top,box.right,box.bottom);
                long families=(long)range[1]-range[0]+1L;if(families<=0)continue;
                float[] vectors={(float)dx,(float)dy,(float)line.offsetX,(float)line.offsetY};m.mapVectors(vectors);
                double dirPx=Math.hypot(vectors[0],vectors[1]);
                double spacingPx=dirPx<1e-9?0:Math.abs(vectors[0]*vectors[3]-vectors[1]*vectors[2])/dirPx;
                int step=1;if(spacingPx>0&&spacingPx<1.2)step=Math.max(step,(int)Math.ceil(1.2/spacingPx));
                if(families/step>4000L)step=Math.max(step,(int)Math.ceil(families/4000d));
                double minT=Double.POSITIVE_INFINITY,maxT=Double.NEGATIVE_INFINITY;
                double[] cx={box.left,box.right,box.right,box.left},cy={box.top,box.top,box.bottom,box.bottom};
                for(int q=0;q<4;q++){double t=cx[q]*dx+cy[q]*dy;minT=Math.min(minT,t);maxT=Math.max(maxT,t);}
                Path lines=new Path();double cycle=DxfHatchPattern.dashCycle(line);
                for(long kk=range[0];kk<=range[1];kk+=step){
                    double px=line.baseX+kk*line.offsetX,py=line.baseY+kk*line.offsetY;
                    double baseT=px*dx+py*dy,t0=minT-baseT-1,t1=maxT-baseT+1;
                    if(line.dashes.length==0||cycle<1e-12){
                        lines.moveTo((float)(px+dx*t0),(float)(py+dy*t0));lines.lineTo((float)(px+dx*t1),(float)(py+dy*t1));
                        if(++segments>=segmentLimit)break outer;continue;
                    }
                    double cycleStart=Math.floor(t0/cycle)*cycle;
                    for(double cs=cycleStart;cs<=t1+cycle;cs+=cycle){
                        double pos=cs;
                        for(double raw:line.dashes){
                            double len=Math.abs(raw);
                            if(len<1e-12){
                                if(pos>=t0&&pos<=t1){double dot=dirPx>1e-9?.8/dirPx:.01;
                                    lines.moveTo((float)(px+dx*(pos-dot*.5)),(float)(py+dy*(pos-dot*.5)));
                                    lines.lineTo((float)(px+dx*(pos+dot*.5)),(float)(py+dy*(pos+dot*.5)));
                                    if(++segments>=segmentLimit)break outer;}
                            }else if(raw>0){
                                double a=Math.max(pos,t0),b=Math.min(pos+len,t1);
                                if(b>a){lines.moveTo((float)(px+dx*a),(float)(py+dy*a));lines.lineTo((float)(px+dx*b),(float)(py+dy*b));
                                    if(++segments>=segmentLimit)break outer;}
                            }
                            pos+=len;
                        }
                    }
                }
                lines.transform(m);c.drawPath(lines,p);
            }
            c.restoreToCount(save);p.setStyle(oldStyle);p.setPathEffect(oldEffect);
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
            this(entity,new double[]{t.a,t.b,t.c,t.d,t.x,t.y});
        }
        Transformed(Entity entity,double[] t)throws IOException {
            this.entity=entity;
            if(t==null||t.length!=6)throw new IOException("Geçersiz DXF dönüşümü");
            float[] v={(float)t[0],(float)t[2],(float)t[4],(float)t[1],(float)t[3],(float)t[5],0,0,1};
            for(float n:v)if(!Float.isFinite(n))throw new IOException("DXF dönüşümü sınır dışında");
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

    private static final class ViewportClip implements Entity {
        final Entity entity;final Matrix modelToPaper;final DxfViewport.Spec spec;final double[] polygon;
        ViewportClip(Entity entity,DxfViewport.Spec spec)throws IOException{this(entity,spec,null);}
        ViewportClip(Entity entity,DxfViewport.Spec spec,double[] polygon)throws IOException{
            this.entity=entity;this.spec=spec;this.polygon=DxfViewportClip.polygon(polygon);double[] t=spec.matrix();
            float[] v={(float)t[0],(float)t[2],(float)t[4],(float)t[1],(float)t[3],(float)t[5],0,0,1};
            for(float n:v)if(!Float.isFinite(n))throw new IOException("VIEWPORT dönüşümü sınır dışında");
            modelToPaper=new Matrix();modelToPaper.setValues(v);
        }
        boolean nonRectangular(){return polygon.length>=6;}
        boolean containsPaper(double x,double y,double tolerance){
            return nonRectangular()?DxfViewportClip.containsPrepared(polygon,x,y,tolerance):DxfViewport.contains(spec,x,y,tolerance);
        }
        Path clipPath(Matrix view){
            Path clip=new Path();
            if(nonRectangular()){
                float[] xy=new float[polygon.length];for(int i=0;i<polygon.length;i++)xy[i]=(float)polygon[i];view.mapPoints(xy);
                clip.moveTo(xy[0],xy[1]);for(int i=2;i+1<xy.length;i+=2)clip.lineTo(xy[i],xy[i+1]);clip.close();return clip;
            }
            float[] xy={(float)spec.left(),(float)spec.bottom(),(float)spec.right(),(float)spec.bottom(),
                (float)spec.right(),(float)spec.top(),(float)spec.left(),(float)spec.top()};view.mapPoints(xy);
            clip.moveTo(xy[0],xy[1]);clip.lineTo(xy[2],xy[3]);clip.lineTo(xy[4],xy[5]);clip.lineTo(xy[6],xy[7]);clip.close();return clip;
        }
        public void bounds(RectF b){
            if(nonRectangular()){double[] q=DxfViewportClip.boundsPrepared(polygon);add(b,(float)q[0],(float)q[1]);add(b,(float)q[2],(float)q[3]);}
            else{add(b,(float)spec.left(),(float)spec.bottom());add(b,(float)spec.right(),(float)spec.top());}
        }
        public void draw(Canvas c,Paint p,Matrix view){
            int save=c.save();c.clipPath(clipPath(view));Matrix combined=new Matrix();combined.setConcat(view,modelToPaper);entity.draw(c,p,combined);c.restoreToCount(save);
        }
    }

    public static final class Result {
        public final Bitmap bitmap;
        public final float[] snapPoints;
        public final int entityCount, layerCount, skippedCount;
        public int conversionWarnings,insUnits;
        public boolean automaticUnits;
        public double unitsPerImagePixel=1d;
        public String unitName="piksel";
        public final Set<String> layerNames,visibleLayers,layoutNames;
        public final String activeLayout;
        private final List<Entity> document;
        private final Matrix view;

        Result(Bitmap b,int e,int skipped,float[] points,List<Entity> document,Matrix view,Set<String> all,Set<String> visible,
               Set<String> layouts,String activeLayout){
            bitmap=b;entityCount=e;skippedCount=skipped;snapPoints=points;
            this.document=document;this.view=new Matrix(view);
            layerNames=Collections.unmodifiableSet(new TreeSet<>(all));
            visibleLayers=Collections.unmodifiableSet(new TreeSet<>(visible));layerCount=all.size();
            LinkedHashSet<String> names=new LinkedHashSet<>();if(layouts!=null)names.addAll(layouts);if(names.isEmpty())names.add(DxfSpace.MODEL);
            layoutNames=Collections.unmodifiableSet(names);this.activeLayout=DxfSpace.normalizeName(activeLayout);
        }

        public Result withVisibleLayers(Set<String> selected)throws IOException{
            Set<String> visible=new HashSet<>(selected);visible.retainAll(layerNames);
            Result result=renderLayers(document,view,layerNames,visible,skippedCount,layoutNames,activeLayout);
            result.conversionWarnings=conversionWarnings;result.insUnits=insUnits;result.automaticUnits=automaticUnits;
            result.unitsPerImagePixel=unitsPerImagePixel;result.unitName=unitName;
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

    /** Flattened entity with its effective AutoCAD display style resolved. */
    private static final class LayerEntity implements Entity {
        final Entity entity;final String layer;final int color,lineWeight;final DxfLineTypes.Pattern lineType;final double lineTypeScale;
        LayerEntity(Entity e,String l,int color,int lineWeight,DxfLineTypes.Pattern lineType,double lineTypeScale){
            entity=e;layer=l;this.color=color;this.lineWeight=lineWeight;this.lineType=lineType;this.lineTypeScale=lineTypeScale;
        }
        public void bounds(RectF b){entity.bounds(b);}
        LayerEntity inViewport(DxfViewport.Spec spec,double[] polygon)throws IOException{
            return new LayerEntity(new ViewportClip(entity,spec,polygon),layer,color,lineWeight,lineType,lineTypeScale);
        }
        public void draw(Canvas c,Paint p,Matrix m){
            p.setColor(color);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(DxfStyle.strokeWidthPx(lineWeight));
            float[] vectors={1,0,0,1};m.mapVectors(vectors);
            double deviceScale=(Math.hypot(vectors[0],vectors[1])+Math.hypot(vectors[2],vectors[3]))*.5;
            float[] dash=DxfLineTypes.dashIntervals(lineType,lineTypeScale,deviceScale);
            p.setPathEffect(dash==null?null:new DashPathEffect(dash,0));
            entity.draw(c,p,m);p.setPathEffect(null);
            if(lineType!=null&&lineType.complex())drawComplex(c,p,entity,m,new int[]{4096});
        }
        private void drawComplex(Canvas c,Paint p,Entity e,Matrix m,int[] budget){
            if(e==null||budget[0]<=0)return;
            if(e instanceof GeometricWidth){drawComplex(c,p,((GeometricWidth)e).entity,m,budget);return;}
            if(e instanceof VariableWidth){drawComplex(c,p,((VariableWidth)e).centerline,m,budget);return;}
            if(e instanceof LeaderEntity){drawComplex(c,p,((LeaderEntity)e).line,m,budget);return;}
            if(e instanceof Transformed){Transformed t=(Transformed)e;Matrix q=new Matrix();q.setConcat(m,t.matrix);drawComplex(c,p,t.entity,q,budget);return;}
            if(e instanceof ViewportClip){
                ViewportClip v=(ViewportClip)e;int save=c.save();c.clipPath(v.clipPath(m));Matrix q=new Matrix();q.setConcat(m,v.modelToPaper);
                drawComplex(c,p,v.entity,q,budget);c.restoreToCount(save);return;
            }
            if(e instanceof Composite){for(Entity item:((Composite)e).items)drawComplex(c,p,item,m,budget);return;}
            if(e instanceof Line){Line q=(Line)e;segment(c,p,m,q.x1,q.y1,q.x2,q.y2,budget);return;}
            if(e instanceof Poly){
                Poly q=(Poly)e;for(int i=1;i<q.pts.size()&&budget[0]>0;i++){PointF a=q.pts.get(i-1),b=q.pts.get(i);segment(c,p,m,a.x,a.y,b.x,b.y,budget);}
                if(q.closed&&q.pts.size()>2&&budget[0]>0){PointF a=q.pts.get(q.pts.size()-1),b=q.pts.get(0);segment(c,p,m,a.x,a.y,b.x,b.y,budget);}return;
            }
            if(e instanceof SegmentSet){SegmentSet q=(SegmentSet)e;for(int i=0;i+3<q.xy.length&&budget[0]>0;i+=4)segment(c,p,m,q.xy[i],q.xy[i+1],q.xy[i+2],q.xy[i+3],budget);return;}
            if(e instanceof Circle){
                Circle q=(Circle)e;int n=Math.max(12,Math.min(96,(int)Math.ceil(Math.abs(q.sweep)/6d)));double prev=Math.toRadians(q.start);
                float ax=(float)(q.x+q.r*Math.cos(prev)),ay=(float)(q.y+q.r*Math.sin(prev));
                for(int i=1;i<=n&&budget[0]>0;i++){double t=Math.toRadians(q.start+q.sweep*i/n);float bx=(float)(q.x+q.r*Math.cos(t)),by=(float)(q.y+q.r*Math.sin(t));segment(c,p,m,ax,ay,bx,by,budget);ax=bx;ay=by;}return;
            }
            if(e instanceof EllipseCurve){
                EllipseCurve q=(EllipseCurve)e;double sw=q.sweep();PointF a=q.at(q.start);
                for(int i=1;i<=72&&budget[0]>0;i++){PointF b=q.at(q.start+sw*i/72d);segment(c,p,m,a.x,a.y,b.x,b.y,budget);a=b;}
            }
        }
        private void segment(Canvas c,Paint p,Matrix m,float x1,float y1,float x2,float y2,int[] budget){
            float[] v={x1,y1,x2,y2};m.mapPoints(v);double dx=v[2]-v[0],dy=v[3]-v[1],len=Math.hypot(dx,dy);if(len<1e-5)return;
            float[] basis={1,0,0,1};m.mapVectors(basis);double deviceScale=(Math.hypot(basis[0],basis[1])+Math.hypot(basis[2],basis[3]))*.5;
            List<DxfLineTypes.Placement> placements=DxfLineTypes.decorations(lineType,lineTypeScale,deviceScale,len);if(placements.isEmpty())return;
            double ux=dx/len,uy=dy/len,nx=-uy,ny=ux,tangent=Math.toDegrees(Math.atan2(dy,dx));
            Paint.Style oldStyle=p.getStyle();PathEffect oldEffect=p.getPathEffect();Typeface oldTypeface=p.getTypeface();float oldText=p.getTextSize();Paint.Align oldAlign=p.getTextAlign();
            p.setStyle(Paint.Style.FILL);p.setPathEffect(null);p.setTypeface(Typeface.create("sans-serif",Typeface.NORMAL));p.setTextAlign(Paint.Align.CENTER);
            for(DxfLineTypes.Placement q:placements){
                if(budget[0]<=0)break;if(q.kind!=DxfLineTypes.KIND_TEXT||q.text.isEmpty())continue;
                double along=q.distance+q.xOffsetPixels,px=v[0]+ux*along+nx*q.yOffsetPixels,py=v[1]+uy*along+ny*q.yOffsetPixels;
                if(!Double.isFinite(px)||!Double.isFinite(py))continue;float size=(float)Math.max(5,Math.min(180,q.scalePixels));p.setTextSize(size);
                double angle=((q.flags&4)!=0?0:tangent)+q.rotation;if((q.flags&8)!=0){angle%=360;if(angle<0)angle+=360;if(angle>90&&angle<270)angle+=180;}
                int save=c.save();c.rotate((float)angle,(float)px,(float)py);c.drawText(q.text,(float)px,(float)(py-size*.18),p);c.restoreToCount(save);budget[0]--;
            }
            p.setStyle(oldStyle);p.setPathEffect(oldEffect);p.setTypeface(oldTypeface);p.setTextSize(oldText);p.setTextAlign(oldAlign);
        }
    }

    private static final class Label implements Entity {
        final float x,y,height,angle,widthFactor,oblique,boxWidth,targetWidth,backgroundScale;
        final String text,fontFamily;final int hAlign,vAlign,attachment,backgroundColor;
        final boolean mtext,backwards,upsideDown,aligned,backgroundEnabled,backgroundFrame;
        private Path cached,cachedBackground;
        Label(float x,float y,float h,float angle,String text){
            this(x,y,h,angle,text,1,0,"sans-serif",0,0,false,1,0,false,false,0,false,false,false,BACKGROUND,1.5f);
        }
        Label(float x,float y,float h,float angle,String text,float widthFactor,float oblique,String fontFamily,
              int hAlign,int vAlign,boolean mtext,int attachment,float boxWidth,boolean backwards,boolean upsideDown,
              float targetWidth,boolean aligned){
            this(x,y,h,angle,text,widthFactor,oblique,fontFamily,hAlign,vAlign,mtext,attachment,boxWidth,backwards,upsideDown,targetWidth,aligned,false,false,BACKGROUND,1.5f);
        }
        Label(float x,float y,float h,float angle,String text,float widthFactor,float oblique,String fontFamily,
              int hAlign,int vAlign,boolean mtext,int attachment,float boxWidth,boolean backwards,boolean upsideDown,
              float targetWidth,boolean aligned,boolean backgroundEnabled,boolean backgroundFrame,int backgroundColor,float backgroundScale){
            this.x=x;this.y=y;this.height=Math.max(.01f,h);this.angle=angle;this.text=text==null?"":text;
            this.widthFactor=Math.max(.01f,Math.abs(widthFactor));this.oblique=Float.isFinite(oblique)?oblique:0;
            this.fontFamily=fontFamily==null||fontFamily.isEmpty()?"sans-serif":fontFamily;this.hAlign=hAlign;this.vAlign=vAlign;
            this.mtext=mtext;this.attachment=attachment;this.boxWidth=Math.max(0,boxWidth);this.backwards=backwards;this.upsideDown=upsideDown;
            this.targetWidth=Math.max(0,targetWidth);this.aligned=aligned;this.backgroundEnabled=backgroundEnabled;this.backgroundFrame=backgroundFrame;this.backgroundColor=backgroundColor;
            this.backgroundScale=(float)DxfMTextBackground.boxScale(backgroundScale);
        }
        private ArrayList<String> rows(Paint paint){
            ArrayList<String> result=new ArrayList<>();String[] explicit=text.split("\n",-1);
            if(!mtext||boxWidth<=0){Collections.addAll(result,explicit);return result;}
            float limit=boxWidth/Math.max(.01f,widthFactor);
            for(String source:explicit){
                if(source.isEmpty()){result.add("");continue;}
                StringBuilder row=new StringBuilder();
                for(String word:source.split(" ",-1)){
                    String candidate=row.length()==0?word:row+" "+word;
                    if(row.length()>0&&paint.measureText(candidate)>limit){result.add(row.toString());row.setLength(0);row.append(word);}
                    else{if(row.length()>0)row.append(' ');row.append(word);}
                }
                result.add(row.toString());
            }
            return result;
        }
        private Path buildShape(){
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setTextSize(height);paint.setTypeface(Typeface.create(fontFamily,Typeface.NORMAL));
            ArrayList<String> rows=rows(paint);Path shape=new Path();
            for(int i=0;i<rows.size();i++){
                String row=rows.get(i);if(row.isEmpty())continue;Path line=new Path();
                paint.getTextPath(row,0,row.length(),0,i*height*1.3f,line);shape.addPath(line);
            }
            if(shape.isEmpty())return shape;
            Matrix glyph=new Matrix();float sx=widthFactor*(backwards?-1f:1f),sy=upsideDown?1f:-1f;
            glyph.setScale(sx,sy);if(Math.abs(oblique)>1e-4)glyph.postSkew((float)Math.tan(Math.toRadians(oblique)),0);shape.transform(glyph);
            RectF r=new RectF();shape.computeBounds(r,true);
            if(targetWidth>0&&r.width()>1e-6){float fit=targetWidth/r.width();Matrix f=new Matrix();f.setScale(fit,aligned?fit:1f);shape.transform(f);shape.computeBounds(r,true);}
            float dx,dy;
            if(targetWidth>0){dx=-r.left;dy=0;}
            else if(mtext){
                double hx=DxfTextLayout.mtextHorizontal(attachment);int vy=DxfTextLayout.mtextVertical(attachment);
                dx=(float)-(r.left+hx*r.width());dy=vy==0?-r.bottom:vy==1?-r.centerY():-r.top;
            }else{
                double hx=DxfTextLayout.textHorizontal(hAlign);int vy=DxfTextLayout.textVertical(vAlign);
                dx=(float)-(r.left+hx*r.width());dy=vy==0?0:vy==1?-r.top:vy==2?-r.centerY():-r.bottom;
            }
            Matrix placement=new Matrix();placement.setTranslate(dx,dy);placement.postRotate(angle);placement.postTranslate(x,y);
            if(backgroundEnabled||backgroundFrame){
                RectF box=new RectF(r);float pad=(float)DxfMTextBackground.padding(height,backgroundScale);box.inset(-pad,-pad);
                Path background=new Path();background.addRect(box,Path.Direction.CW);background.transform(placement);cachedBackground=background;
            }
            shape.transform(placement);return shape;
        }
        private Path shape(){if(cached==null)cached=buildShape();return new Path(cached);}
        private Path background(){shape();return cachedBackground==null?null:new Path(cachedBackground);}
        public void bounds(RectF b){
            RectF r=new RectF();shape().computeBounds(r,true);if(!r.isEmpty()){add(b,r.left,r.top);add(b,r.right,r.bottom);}
            Path bg=background();if(bg!=null){r.setEmpty();bg.computeBounds(r,true);if(!r.isEmpty()){add(b,r.left,r.top);add(b,r.right,r.bottom);}}
        }
        public void draw(Canvas c,Paint p,Matrix m){
            Path path=shape();path.transform(m);Paint.Style old=p.getStyle();PathEffect effect=p.getPathEffect();int oldColor=p.getColor();
            p.setPathEffect(null);Path bg=background();if(bg!=null)bg.transform(m);
            if(bg!=null&&backgroundEnabled){p.setStyle(Paint.Style.FILL);p.setColor(backgroundColor);c.drawPath(bg,p);}
            if(bg!=null&&backgroundFrame){p.setStyle(Paint.Style.STROKE);p.setColor(oldColor);c.drawPath(bg,p);}
            p.setStyle(Paint.Style.FILL);p.setColor(oldColor);c.drawPath(path,p);p.setColor(oldColor);p.setStyle(old);p.setPathEffect(effect);
        }
    }

    private static String str(List<String>a,int from,int to,int code,String fallback){
        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==code)return a.get(i+1);
        return fallback;
    }

    public static Result render(File file)throws IOException{return render(file,null);}

    public static Result render(File file,String preferredLayout)throws IOException{
        // LibreDWG can expand a modest DWG into a very large ASCII DXF.
        // Avoid retaining millions of DXF text lines on mobile devices.
        if(file.length()>8L*1024*1024)return renderStreaming(file,preferredLayout);
        try{return renderBuffered(file,preferredLayout);}
        catch(IOException e){
            if(e.getMessage()!=null&&e.getMessage().contains("etiket sınırı"))return renderStreaming(file,preferredLayout);
            throw e;
        }
    }

    private static DxfViewport.Spec viewportSpec(List<String>a,int from,int to){
        return DxfViewport.of(f(a,from,to,10),f(a,from,to,20),fv(a,from,to,40,0f),fv(a,from,to,41,0f),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,45,0f),fv(a,from,to,51,0f),
            (int)fv(a,from,to,69,0f),(int)fv(a,from,to,68,1f),
            fv(a,from,to,16,0f),fv(a,from,to,26,0f),fv(a,from,to,36,1f),
            fv(a,from,to,17,0f),fv(a,from,to,27,0f),fv(a,from,to,37,0f));
    }

    private static Entity viewportFrame(DxfViewport.Spec spec){return viewportFrame(spec,null);}

    private static Entity viewportFrame(DxfViewport.Spec spec,double[] polygon){
        double[] clip=DxfViewportClip.polygon(polygon);ArrayList<PointF> p=new ArrayList<>();
        if(clip.length>=6){for(int i=0;i+1<clip.length;i+=2)p.add(new PointF((float)clip[i],(float)clip[i+1]));return new Poly(p,true);}
        p.add(new PointF((float)spec.left(),(float)spec.bottom()));p.add(new PointF((float)spec.right(),(float)spec.bottom()));
        p.add(new PointF((float)spec.right(),(float)spec.top()));p.add(new PointF((float)spec.left(),(float)spec.top()));return new Poly(p,true);
    }

    private static double[] viewportClipPolygon(Entity entity){
        ArrayList<PointF> points=new ArrayList<>();if(!appendViewportClipPolygon(entity,new Matrix(),points))return new double[0];
        double[] packed=new double[points.size()*2];for(int i=0;i<points.size();i++){packed[i*2]=points.get(i).x;packed[i*2+1]=points.get(i).y;}
        return DxfViewportClip.polygon(packed);
    }

    private static boolean appendViewportClipPolygon(Entity entity,Matrix transform,ArrayList<PointF> out){
        if(entity==null)return false;
        if(entity instanceof GeometricWidth)return appendViewportClipPolygon(((GeometricWidth)entity).entity,transform,out);
        if(entity instanceof VariableWidth)return appendViewportClipPolygon(((VariableWidth)entity).centerline,transform,out);
        if(entity instanceof Transformed){Transformed t=(Transformed)entity;Matrix q=new Matrix();q.setConcat(transform,t.matrix);return appendViewportClipPolygon(t.entity,q,out);}
        if(entity instanceof Poly){Poly p=(Poly)entity;if(!p.closed||p.pts.size()<3)return false;for(PointF v:p.pts)addMapped(out,transform,v.x,v.y);return true;}
        if(entity instanceof FilledPoly){FilledPoly p=(FilledPoly)entity;if(p.pts.size()<3)return false;for(PointF v:p.pts)addMapped(out,transform,v.x,v.y);return true;}
        if(entity instanceof Circle){Circle q=(Circle)entity;if(Math.abs(q.sweep)<359.99||q.r<=0)return false;for(int i=0;i<96;i++){double a=Math.toRadians(q.start+q.sweep*i/96d);addMapped(out,transform,q.x+q.r*Math.cos(a),q.y+q.r*Math.sin(a));}return true;}
        if(entity instanceof EllipseCurve){EllipseCurve q=(EllipseCurve)entity;double sw=q.sweep();if(sw<Math.PI*2-1e-4)return false;for(int i=0;i<96;i++){PointF v=q.at(q.start+sw*i/96d);addMapped(out,transform,v.x,v.y);}return true;}
        return false;
    }

    private static void addMapped(ArrayList<PointF> out,Matrix transform,double x,double y){float[] q={(float)x,(float)y};transform.mapPoints(q);if(Float.isFinite(q[0])&&Float.isFinite(q[1]))out.add(new PointF(q[0],q[1]));}

    private static Map<String,double[]> bufferedViewportClipPolygons(List<String> lines,List<DxfBlocks.Placement> placements,DxfPointStyle.Style pointStyle,DxfDimStyles.Table dimStyles)throws IOException{
        HashSet<String> wanted=new HashSet<>();for(DxfBlocks.Placement p:placements)if("VIEWPORT".equals(p.record.type)){String h=p.record.text(340,"");if(!h.isEmpty())wanted.add(DxfColor.key(h));}
        HashMap<String,double[]> result=new HashMap<>();if(wanted.isEmpty())return result;
        for(int index=0;index<placements.size();index++){
            DxfBlocks.Placement item=placements.get(index);String handle=item.record.text(5,"");if(handle.isEmpty()||!wanted.contains(DxfColor.key(handle)))continue;Entity entity=null;
            if("POLYLINE".equals(item.record.type)){ArrayList<DxfBlocks.Record> vertices=new ArrayList<>();int j=index+1;while(j<placements.size()&&"VERTEX".equals(placements.get(j).record.type)){vertices.add(placements.get(j).record);j++;}entity=classicPolyline(item.record,vertices,lines);}
            else if("LWPOLYLINE".equals(item.record.type)||"CIRCLE".equals(item.record.type)||"ELLIPSE".equals(item.record.type)||"SPLINE".equals(item.record.type)||"SOLID".equals(item.record.type))
                entity=parse(item.record.type,lines,item.record.from,item.record.to,pointStyle,dimStyles);
            if(entity==null)continue;entity=projectOcs(item.record.type,entity,lines,item.record.from,item.record.to);entity=new Transformed(entity,item.transform);
            double[] polygon=viewportClipPolygon(entity);if(polygon.length>=6)result.put(DxfColor.key(handle),polygon);
        }
        return result;
    }

    private static Set<String> viewportFrozenLayers(List<String>a,int from,int to,DxfLayerTable.Table layerTable){
        HashSet<String> result=new HashSet<>();
        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==331){
            String name=layerTable.nameForHandle(a.get(i+1));if(name!=null)result.add(DxfColor.key(name));
        }
        return result;
    }

    private static Set<String> viewportFrozenLayers(Collection<String> handles,DxfLayerTable.Table layerTable){
        HashSet<String> result=new HashSet<>();if(handles==null)return result;
        for(String handle:handles){String name=layerTable.nameForHandle(handle);if(name!=null)result.add(DxfColor.key(name));}
        return result;
    }

    private static void appendViewportEntities(ArrayList<Entity> target,List<LayerEntity> model,DxfViewport.Spec spec,Set<String> frozenLayers,double[] polygon)throws IOException{
        if((long)target.size()+model.size()>500000L)throw new IOException("VIEWPORT açıldığında nesne sınırı aşıldı");
        for(LayerEntity item:model){
            FileTransfer.checkCancelled();if(frozenLayers!=null&&frozenLayers.contains(DxfColor.key(item.layer)))continue;target.add(item.inViewport(spec,polygon));
        }
    }

    private static int appendBufferedPlacements(List<String> lines,List<DxfBlocks.Placement> placements,
                                                 DxfLayerTable.Table layerTable,DxfLineTypes.Table lineTypes,DxfTextStyles.Table textStyles,DxfPointStyle.Style pointStyle,DxfDimStyles.Table dimStyles,
                                                 ArrayList<Entity> entities,Set<String> layers,Set<String> visibleLayers,
                                                 List<LayerEntity> viewportModel,Map<String,double[]> viewportClips)throws IOException{
        int skipped=0;Map<String,Integer> layerColors=layerTable.colors;
        for(int index=0;index<placements.size();index++){
            FileTransfer.checkCancelled();DxfBlocks.Placement item=placements.get(index);Entity entity;
            if("VIEWPORT".equals(item.record.type)){
                DxfViewport.Spec spec=viewportSpec(lines,item.record.from,item.record.to);
                if(viewportModel==null||!spec.supported()){skipped++;continue;}
                Set<String> frozen=viewportFrozenLayers(lines,item.record.from,item.record.to,layerTable);String clipHandle=item.record.text(340,"");
                double[] clip=viewportClips==null?null:viewportClips.get(DxfColor.key(clipHandle));
                appendViewportEntities(entities,viewportModel,spec,frozen,clip);entity=viewportFrame(spec,clip);
            }else if("POLYLINE".equals(item.record.type)){
                ArrayList<DxfBlocks.Record> vertices=new ArrayList<>();int j=index+1;
                while(j<placements.size()&&"VERTEX".equals(placements.get(j).record.type)){vertices.add(placements.get(j).record);j++;}
                entity=classicPolyline(item.record,vertices,lines);index=j-1;
                if(entity==null){skipped++;continue;}
            }else if("VERTEX".equals(item.record.type)){
                skipped++;continue;
            }else{
                entity=isTextType(item.record.type)?parseTextEntity(item.record.type,lines,item.record.from,item.record.to,textStyles):
                    parse(item.record.type,lines,item.record.from,item.record.to,pointStyle,dimStyles);
                if(entity==null){skipped++;continue;}
            }
            entity=projectOcs(item.record.type,entity,lines,item.record.from,item.record.to);
            int color=DxfTransparency.apply(DxfColor.argb(item.color,layerColors),DxfTransparency.opacity(item.transparency,layerTable.opacities));
            String effectiveType=DxfStyle.effectiveLineType(item.lineType,item.layer,layerTable.lineTypes);
            int effectiveWeight=DxfStyle.effectiveLineWeight(item.lineWeight,item.layer,layerTable.lineWeights);
            entities.add(new LayerEntity(new Transformed(entity,item.transform),item.layer,color,effectiveWeight,
                lineTypes.get(effectiveType),item.lineTypeScale*lineTypes.globalScale));
            if(layers.add(item.layer)&&!layerTable.contains(item.layer))visibleLayers.add(item.layer);
        }
        return skipped;
    }

    private static Result renderBuffered(File file,String preferredLayout)throws IOException{
        List<String> lines=readLines(file);
        DxfLayerTable.Table layerTable=DxfLayerTable.parse(lines);
        DxfLineTypes.Table lineTypes=DxfLineTypes.parse(lines);
        DxfTextStyles.Table textStyles=DxfTextStyles.parse(lines);
        DxfPointStyle.Style pointStyle=DxfPointStyle.parse(lines);
        DxfDimStyles.Table dimStyles=DxfDimStyles.parse(lines);
        int insUnits=DxfUnits.parse(lines);
        DxfBlocks.Result expanded=DxfBlocks.expand(lines,preferredLayout);
        ArrayList<Entity> entities=new ArrayList<>();Set<String> layers=new HashSet<>(layerTable.names);
        Set<String> visibleLayers=new HashSet<>(layerTable.visible);List<LayerEntity> viewportModel=null;
        if(!DxfSpace.isModel(expanded.activeLayout)){
            DxfBlocks.Result modelExpanded=DxfBlocks.expand(lines,DxfSpace.MODEL);ArrayList<Entity> modelRaw=new ArrayList<>();
            appendBufferedPlacements(lines,modelExpanded.placements,layerTable,lineTypes,textStyles,pointStyle,dimStyles,modelRaw,layers,visibleLayers,null,null);
            viewportModel=new ArrayList<>();for(Entity e:modelRaw)if(e instanceof LayerEntity)viewportModel.add((LayerEntity)e);
        }
        Map<String,double[]> viewportClips=viewportModel==null?Collections.emptyMap():bufferedViewportClipPolygons(lines,expanded.placements,pointStyle,dimStyles);
        int skipped=expanded.skipped+appendBufferedPlacements(lines,expanded.placements,layerTable,lineTypes,textStyles,pointStyle,dimStyles,
            entities,layers,visibleLayers,viewportModel,viewportClips);
        return applyUnits(finishEntities(entities,layers,visibleLayers,skipped,expanded.layouts,expanded.activeLayout),insUnits);
    }

    private interface StreamNode {}
    private static final class StreamShape implements StreamNode {
        final Entity entity;final String layer,handle;final int aci,trueColor,lineWeight;final String lineType;final double lineTypeScale;final long transparencyRaw;
        StreamShape(Entity entity,String layer,String handle,int aci,int trueColor,long transparencyRaw,String lineType,int lineWeight,double lineTypeScale){
            this.entity=entity;this.layer=layer;this.handle=handle;this.aci=aci;this.trueColor=trueColor;this.transparencyRaw=transparencyRaw;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;
        }
    }
    private static final class StreamViewport implements StreamNode {
        final DxfViewport.Spec spec;final String layer,lineType,handle,clipHandle;final int aci,trueColor,lineWeight;final long transparencyRaw;final double lineTypeScale;
        final Set<String> frozenHandles=new LinkedHashSet<>();
        StreamViewport(StreamRecord r)throws IOException{
            spec=DxfViewport.of(r.number(10,0),r.number(20,0),r.number(40,0),r.number(41,0),
                r.number(12,0),r.number(22,0),r.number(45,0),r.number(51,0),r.integer(69,0),r.integer(68,1),
                r.number(16,0),r.number(26,0),r.number(36,1),r.number(17,0),r.number(27,0),r.number(37,0));
            layer=r.text(8,"0");handle=r.text(5,"");clipHandle=r.text(340,"");aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();
            transparencyRaw=r.longInteger(440,DxfTransparency.UNSET);lineType=r.text(6,DxfStyle.BYLAYER);
            lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));
            for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==331)frozenHandles.add(r.tags.get(i+1).trim());
        }
    }

    private static final class StreamInsert implements StreamNode {
        final String name,layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;
        final double x,y,z,sx,sy,sz,rotation,ex,ey,ez,lineTypeScale,columnSpacing,rowSpacing;
        final int columns,rows;
        StreamInsert(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");handle=r.text(5,"");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();transparencyRaw=r.longInteger(440,DxfTransparency.UNSET);
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);lineTypeScale=DxfStyle.saneScale(r.number(48,1));
            x=r.number(10,0);y=r.number(20,0);z=r.number(30,0);
            sx=r.number(41,1);sy=r.number(42,1);sz=r.number(43,1);rotation=r.number(50,0);
            columns=(int)r.number(70,1);rows=(int)r.number(71,1);
            columnSpacing=r.number(44,0);rowSpacing=r.number(45,0);
            ex=r.number(210,0);ey=r.number(220,0);ez=r.number(230,1);
        }
    }
    private static final class StreamDimension implements StreamNode {
        final String name,layer,lineType,handle;final int aci,trueColor,lineWeight;final long transparencyRaw;
        StreamDimension(StreamRecord r)throws IOException{
            name=key(r.text(2,""));layer=r.text(8,"0");handle=r.text(5,"");
            aci=r.integer(62,DxfColor.BYLAYER);trueColor=r.trueColor();transparencyRaw=r.longInteger(440,DxfTransparency.UNSET);
            lineType=r.text(6,DxfStyle.BYLAYER);lineWeight=r.integer(370,DxfStyle.LW_BYLAYER);
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
        long longInteger(int code,long fallback)throws IOException{
            String value=text(code,Long.toString(fallback));
            try{return Long.parseLong(value);}catch(NumberFormatException e){throw new IOException("Geçersiz DXF tamsayı değeri",e);}
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
        double numberAt(int tagIndex,double fallback)throws IOException{
            if(tagIndex<0||tagIndex+1>=tags.size())return fallback;
            try{double n=Double.parseDouble(tags.get(tagIndex+1).trim());if(!Double.isFinite(n))throw new NumberFormatException();return n;}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF sayısal değeri",e);}
        }
    }
    private static final class PendingPoly {
        final ArrayList<StreamNode> target;final String layer,lineType,handle;final boolean closed;final int flags,mCount,nCount,aci,trueColor,lineWeight;final double lineTypeScale,geometricWidth,defaultStartWidth,defaultEndWidth;final long transparencyRaw;
        final double[] ocs;boolean variableWidth;
        final ArrayList<PointF> points=new ArrayList<>();final ArrayList<Double> bulges=new ArrayList<>(),startWidths=new ArrayList<>(),endWidths=new ArrayList<>();final ArrayList<int[]> faces=new ArrayList<>();
        PendingPoly(ArrayList<StreamNode> target,String layer,String handle,int flags,int mCount,int nCount,int aci,int trueColor,long transparencyRaw,String lineType,int lineWeight,double lineTypeScale,double defaultStartWidth,double defaultEndWidth,double[] ocs){
            this.target=target;this.layer=layer;this.handle=handle;this.flags=flags;this.mCount=mCount;this.nCount=nCount;this.closed=(flags&1)!=0;this.aci=aci;this.trueColor=trueColor;this.transparencyRaw=transparencyRaw;
            this.lineType=lineType;this.lineWeight=lineWeight;this.lineTypeScale=lineTypeScale;this.defaultStartWidth=defaultStartWidth;this.defaultEndWidth=defaultEndWidth;this.geometricWidth=DxfPolylineWidth.uniform(defaultStartWidth,defaultEndWidth);this.ocs=ocs;
        }
    }
    private static final class StreamContext {
        String section="",rootSequenceLayout;StreamBlock activeBlock;PendingPoly pending;int skipped,insUnits;
        final HashMap<String,StreamBlock> blocks=new HashMap<>();
        final DxfLayerTable.Table layerTable=new DxfLayerTable.Table();
        final DxfLineTypes.Table lineTypes=new DxfLineTypes.Table();
        final DxfTextStyles.Table textStyles=new DxfTextStyles.Table();
        final DxfDimStyles.Table dimStyles=new DxfDimStyles.Table();
        DxfPointStyle.Style pointStyle=new DxfPointStyle.Style(0,0);
        final Map<String,String> drawOrder=new HashMap<>();
        final Map<String,String> layoutByOwner=new HashMap<>();
        final Map<String,double[]> viewportClips=new HashMap<>();
        final LinkedHashSet<String> layouts=new LinkedHashSet<>();
        final LinkedHashMap<String,ArrayList<StreamNode>> rootsByLayout=new LinkedHashMap<>();
    }
    private static final class StreamCounter {int visits;}

    /** Memory-bounded parser used for large ASCII DXF files produced by DWG conversion. */
    private static Result renderStreaming(File file,String preferredLayout)throws IOException{
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
        finishPending(context);resolveStreamLayouts(context);buildStreamViewportClips(context);
        context.layerTable.ensureDefaultLayer();
        ArrayList<Entity> entities=new ArrayList<>();
        Set<String> layers=new HashSet<>(context.layerTable.names);
        Set<String> visibleLayers=new HashSet<>(context.layerTable.visible);
        if(context.layouts.isEmpty())context.layouts.add(DxfSpace.MODEL);
        String activeLayout=DxfSpace.chooseActive(context.rootsByLayout,preferredLayout);
        List<LayerEntity> viewportModel=null;
        if(!DxfSpace.isModel(activeLayout)){
            ArrayList<StreamNode> modelRoots=context.rootsByLayout.get(DxfSpace.MODEL);
            if(modelRoots!=null&&!modelRoots.isEmpty()){
                ArrayList<StreamNode> orderedModel=new ArrayList<>(modelRoots);
                if(!context.drawOrder.isEmpty())orderedModel.sort((a,b)->DxfDrawOrder.compare(streamHandle(a),streamHandle(b),context.drawOrder));
                ArrayList<Entity> modelRaw=new ArrayList<>();int previousSkipped=context.skipped;StreamCounter modelCounter=new StreamCounter();
                expandStream(orderedModel,new DxfBlocks.Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,
                    context.blocks,new HashSet<>(),modelRaw,layers,visibleLayers,modelCounter,context,null);
                context.skipped=previousSkipped;viewportModel=new ArrayList<>();
                for(Entity e:modelRaw)if(e instanceof LayerEntity)viewportModel.add((LayerEntity)e);
            }
        }
        ArrayList<StreamNode> roots=context.rootsByLayout.get(activeLayout);if(roots==null)roots=new ArrayList<>();
        if(!context.drawOrder.isEmpty())roots.sort((a,b)->DxfDrawOrder.compare(streamHandle(a),streamHandle(b),context.drawOrder));
        StreamCounter counter=new StreamCounter();
        expandStream(roots,new DxfBlocks.Transform(),"0",null,null,null,DxfStyle.LW_BYLAYER,
            context.blocks,new HashSet<>(),entities,layers,visibleLayers,counter,context,viewportModel);
        return applyUnits(finishEntities(entities,layers,visibleLayers,context.skipped,context.layouts,activeLayout),context.insUnits);
    }

    private static boolean keepStreamCode(int code){
        return code==1||code==2||code==3||code==4||code==5||code==6||code==7||code==8||code==9||code==60||code==62||code==63||code==66||code==67||code==68||code==69||
            code==330||code==331||code==340||code==370||code==410||code==420||code==421||code==440||
            (code>=10&&code<=59)||(code>=70&&code<=79)||(code>=90&&code<=99)||(code>=300&&code<=305)||(code>=450&&code<=470)||
            (code>=210&&code<=213)||(code>=220&&code<=223)||(code>=230&&code<=233);
    }

    private static boolean streamSequenceMember(String type){
        return "VERTEX".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)||"SEQEND".equals(type);
    }

    private static ArrayList<StreamNode> streamTarget(StreamContext c,String type,StreamRecord r)throws IOException{
        if("BLOCKS".equals(c.section)&&c.activeBlock!=null)return c.activeBlock.members;
        if(!"ENTITIES".equals(c.section))return null;
        boolean member=streamSequenceMember(type);
        if(c.rootSequenceLayout!=null&&!member)c.rootSequenceLayout=null;
        int paper=r.integer(67,0);String rawLayout=r.text(410,"");
        String layout=c.rootSequenceLayout!=null?c.rootSequenceLayout:
            (paper==1&&rawLayout.trim().isEmpty()?DxfSpace.unresolvedPaper(r.text(330,"")):DxfSpace.layout(paper,rawLayout));
        if(!DxfSpace.isUnresolvedPaper(layout))c.layouts.add(layout);
        ArrayList<StreamNode> target=c.rootsByLayout.computeIfAbsent(layout,k->new ArrayList<>());
        if("POLYLINE".equals(type)||("INSERT".equals(type)&&r.integer(66,0)!=0))c.rootSequenceLayout=layout;
        return target;
    }

    private static void processStreamRecord(String type,StreamRecord r,StreamContext c)throws IOException{
        if("SECTION".equals(type)){
            finishPending(c);c.section=r.text(2,"");c.activeBlock=null;
            if("HEADER".equals(c.section)){
                c.insUnits=DxfUnits.parseHeaderRecord(r.tags);c.pointStyle=DxfPointStyle.parseHeaderRecord(r.tags);
                for(int i=0;i+1<r.tags.size();i+=2)if(intOf(r.tags.get(i))==9&&"$LTSCALE".equalsIgnoreCase(r.tags.get(i+1).trim())){
                    for(int j=i+2;j+1<r.tags.size();j+=2){int code=intOf(r.tags.get(j));if(code==9)break;if(code==40){double v=r.numberAt(j,1);if(v>0)c.lineTypes.globalScale=v;break;}}
                }
            }
            return;
        }
        if("ENDSEC".equals(type)){finishPending(c);c.section="";c.activeBlock=null;return;}
        if("EOF".equals(type)){finishPending(c);return;}
        if("BLOCKS".equals(c.section)){
            if("BLOCK".equals(type)){
                finishPending(c);StreamBlock block=new StreamBlock(r);c.activeBlock=block;
                if(!block.name.isEmpty())c.blocks.put(block.name,block);return;
            }
            if("ENDBLK".equals(type)){finishPending(c);c.activeBlock=null;return;}
        }
        if("TABLES".equals(c.section)&&"LAYER".equals(type)){
            String layerName=r.text(2,"0");
            c.layerTable.add(layerName,r.integer(62,7),r.integer(70,0),r.trueColor(),r.text(6,DxfStyle.CONTINUOUS),r.integer(370,DxfStyle.LW_DEFAULT),
                r.longInteger(440,DxfTransparency.UNSET),r.text(5,""));return;
        }
        if("TABLES".equals(c.section)&&"LTYPE".equals(type)){c.lineTypes.addRecord(r.tags);return;}
        if("TABLES".equals(c.section)&&"STYLE".equals(type)){
            c.textStyles.add(r.text(2,DxfTextStyles.STANDARD),r.text(3,""),r.text(4,""),r.number(40,0),r.number(41,1),r.number(50,0),
                r.integer(70,0),r.integer(71,0));return;
        }
        if("TABLES".equals(c.section)&&"DIMSTYLE".equals(type)){c.dimStyles.addRecord(r.tags);return;}
        if("OBJECTS".equals(c.section)&&"LAYOUT".equals(type)){
            String name=DxfSpace.layoutObjectName(r.tags,0,r.tags.size());String owner=DxfSpace.layoutObjectOwner(r.tags,0,r.tags.size());
            c.layouts.add(name);if(!owner.isEmpty())c.layoutByOwner.put(owner,name);return;
        }
        if("OBJECTS".equals(c.section)&&"SORTENTSTABLE".equals(type)){
            DxfDrawOrder.addRecord(r.tags,0,r.tags.size(),c.drawOrder);return;
        }
        ArrayList<StreamNode> target=streamTarget(c,type,r);if(target==null)return;
        if(c.pending!=null){
            if("VERTEX".equals(type)){
                if(!DxfVisibility.invisible(type,r.integer(60,0),r.integer(70,0))){
                    int vertexFlags=r.integer(70,0);
                    if((c.pending.flags&64)!=0&&(vertexFlags&128)!=0&&(vertexFlags&64)==0){
                        c.pending.faces.add(new int[]{r.integer(71,0),r.integer(72,0),r.integer(73,0),r.integer(74,0)});
                    }else{
                        boolean hasStart=has(r.tags,0,r.tags.size(),40),hasEnd=has(r.tags,0,r.tags.size(),41);if(hasStart||hasEnd)c.pending.variableWidth=true;
                        c.pending.points.add(new PointF((float)r.number(10,0),(float)r.number(20,0)));c.pending.bulges.add(r.number(42,0));
                        c.pending.startWidths.add(r.number(40,c.pending.defaultStartWidth));c.pending.endWidths.add(r.number(41,c.pending.defaultEndWidth));
                    }
                }
                return;
            }
            if("SEQEND".equals(type)){finishPending(c);c.rootSequenceLayout=null;return;}
            finishPending(c);
        }
        if(DxfVisibility.invisible(type,r.integer(60,0),r.integer(70,0)))return;
        if("POLYLINE".equals(type)){
            int flags=r.integer(70,0);
            c.pending=new PendingPoly(target,r.text(8,"0"),r.text(5,""),flags,r.integer(71,0),r.integer(72,0),
                r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.longInteger(440,DxfTransparency.UNSET),r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1)),
                r.number(40,0),r.number(41,0),ocsMatrix("POLYLINE",r.tags,0,r.tags.size()));return;
        }
        if("VERTEX".equals(type))return;
        if("SEQEND".equals(type)){c.rootSequenceLayout=null;return;}
        if("VIEWPORT".equals(type)){target.add(new StreamViewport(r));return;}
        if("INSERT".equals(type)){target.add(new StreamInsert(r));return;}
        if("DIMENSION".equals(type)){target.add(new StreamDimension(r));return;}
        Entity entity=isTextType(type)?parseTextEntity(type,r.tags,0,r.tags.size(),c.textStyles):parse(type,r.tags,0,r.tags.size(),c.pointStyle,c.dimStyles);
        if(entity==null)c.skipped++;else{
            entity=projectOcs(type,entity,r.tags,0,r.tags.size());
            target.add(new StreamShape(entity,r.text(8,"0"),r.text(5,""),r.integer(62,DxfColor.BYLAYER),r.trueColor(),r.longInteger(440,DxfTransparency.UNSET),
                r.text(6,DxfStyle.BYLAYER),r.integer(370,DxfStyle.LW_BYLAYER),DxfStyle.saneScale(r.number(48,1))));
        }
    }

    private static void resolveStreamLayouts(StreamContext c){
        LinkedHashMap<String,ArrayList<StreamNode>> resolved=new LinkedHashMap<>();
        for(Map.Entry<String,ArrayList<StreamNode>> entry:c.rootsByLayout.entrySet()){
            String layout=DxfSpace.resolveUnresolved(entry.getKey(),c.layoutByOwner);c.layouts.add(layout);
            resolved.computeIfAbsent(layout,k->new ArrayList<>()).addAll(entry.getValue());
        }
        c.rootsByLayout.clear();c.rootsByLayout.putAll(resolved);c.layouts.addAll(c.layoutByOwner.values());
    }

    private static void buildStreamViewportClips(StreamContext c){
        HashSet<String> wanted=new HashSet<>();for(ArrayList<StreamNode> nodes:c.rootsByLayout.values())for(StreamNode node:nodes)if(node instanceof StreamViewport){
            String h=((StreamViewport)node).clipHandle;if(h!=null&&!h.isEmpty())wanted.add(DxfColor.key(h));}
        if(wanted.isEmpty())return;
        for(ArrayList<StreamNode> nodes:c.rootsByLayout.values())for(StreamNode node:nodes)if(node instanceof StreamShape){
            StreamShape shape=(StreamShape)node;if(shape.handle==null||shape.handle.isEmpty()||!wanted.contains(DxfColor.key(shape.handle)))continue;
            double[] polygon=viewportClipPolygon(shape.entity);if(polygon.length>=6)c.viewportClips.put(DxfColor.key(shape.handle),polygon);
        }
    }

    private static void finishPending(StreamContext c)throws IOException{
        PendingPoly p=c.pending;if(p==null)return;c.pending=null;
        Entity entity=streamPolyline(p);if(entity==null){c.skipped++;return;}
        if(p.ocs!=null)entity=new Transformed(entity,p.ocs);
        p.target.add(new StreamShape(entity,p.layer,p.handle,p.aci,p.trueColor,p.transparencyRaw,p.lineType,p.lineWeight,p.lineTypeScale));
    }

    private static void expandStream(List<StreamNode> nodes,DxfBlocks.Transform parent,String parentLayer,DxfColor.Ref byBlockColor,
                                     DxfTransparency.Ref byBlockTransparency,String byBlockLineType,int byBlockLineWeight,Map<String,StreamBlock> blocks,Set<String> stack,ArrayList<Entity> entities,Set<String> layers,
                                     Set<String> visibleLayers,StreamCounter counter,StreamContext context,List<LayerEntity> viewportModel)throws IOException{
        for(StreamNode node:nodes){
            FileTransfer.checkCancelled();
            if(++counter.visits>DxfBlocks.MAX_EXPANSION_VISITS)throw new IOException("DXF blokları açıldığında nesne sınırı aşıldı");
            if(node instanceof StreamShape){
                StreamShape shape=(StreamShape)node;String layer="0".equals(shape.layer)?parentLayer:shape.layer;
                DxfColor.Ref ref=DxfColor.resolve(shape.aci,shape.trueColor,layer,byBlockColor);
                DxfTransparency.Ref transparency=DxfTransparency.resolve(shape.transparencyRaw,layer,byBlockTransparency);
                int color=DxfTransparency.apply(DxfColor.argb(ref,context.layerTable.colors),DxfTransparency.opacity(transparency,context.layerTable.opacities));
                String semanticType=DxfStyle.resolveLineType(shape.lineType,byBlockLineType);
                int semanticWeight=DxfStyle.resolveLineWeight(shape.lineWeight,byBlockLineWeight);
                String effectiveType=DxfStyle.effectiveLineType(semanticType,layer,context.layerTable.lineTypes);
                int effectiveWeight=DxfStyle.effectiveLineWeight(semanticWeight,layer,context.layerTable.lineWeights);
                entities.add(new LayerEntity(new Transformed(shape.entity,parent),layer,color,effectiveWeight,
                    context.lineTypes.get(effectiveType),shape.lineTypeScale*context.lineTypes.globalScale));
                if(layers.add(layer)&&!context.layerTable.contains(layer))visibleLayers.add(layer);
                continue;
            }
            if(node instanceof StreamViewport){
                StreamViewport viewport=(StreamViewport)node;String layer="0".equals(viewport.layer)?parentLayer:viewport.layer;
                if(viewportModel==null||!viewport.spec.supported()){context.skipped++;continue;}
                Set<String> frozen=viewportFrozenLayers(viewport.frozenHandles,context.layerTable);double[] clip=context.viewportClips.get(DxfColor.key(viewport.clipHandle));
                appendViewportEntities(entities,viewportModel,viewport.spec,frozen,clip);
                DxfColor.Ref ref=DxfColor.resolve(viewport.aci,viewport.trueColor,layer,byBlockColor);
                DxfTransparency.Ref transparency=DxfTransparency.resolve(viewport.transparencyRaw,layer,byBlockTransparency);
                int color=DxfTransparency.apply(DxfColor.argb(ref,context.layerTable.colors),DxfTransparency.opacity(transparency,context.layerTable.opacities));
                String semanticType=DxfStyle.resolveLineType(viewport.lineType,byBlockLineType);
                int semanticWeight=DxfStyle.resolveLineWeight(viewport.lineWeight,byBlockLineWeight);
                String effectiveType=DxfStyle.effectiveLineType(semanticType,layer,context.layerTable.lineTypes);
                int effectiveWeight=DxfStyle.effectiveLineWeight(semanticWeight,layer,context.layerTable.lineWeights);
                entities.add(new LayerEntity(new Transformed(viewportFrame(viewport.spec,clip),parent),layer,color,effectiveWeight,
                    context.lineTypes.get(effectiveType),viewport.lineTypeScale*context.lineTypes.globalScale));
                if(layers.add(layer)&&!context.layerTable.contains(layer))visibleLayers.add(layer);
                continue;
            }
            if(node instanceof StreamDimension){
                StreamDimension dimension=(StreamDimension)node;String layer="0".equals(dimension.layer)?parentLayer:dimension.layer;
                StreamBlock block=blocks.get(dimension.name);
                if(block==null||stack.contains(dimension.name)||stack.size()>=32||(block.flags&12)!=0||!block.xref.isEmpty()){
                    context.skipped++;continue;
                }
                DxfColor.Ref dimColor=DxfColor.resolve(dimension.aci,dimension.trueColor,layer,byBlockColor);
                DxfTransparency.Ref dimTransparency=DxfTransparency.resolve(dimension.transparencyRaw,layer,byBlockTransparency);
                String dimType=DxfStyle.resolveLineType(dimension.lineType,byBlockLineType);
                int dimWeight=DxfStyle.resolveLineWeight(dimension.lineWeight,byBlockLineWeight);
                stack.add(dimension.name);
                expandStream(block.members,parent,layer,dimColor,dimTransparency,dimType,dimWeight,blocks,stack,entities,layers,visibleLayers,counter,context,viewportModel);
                stack.remove(dimension.name);continue;
            }
            StreamInsert insert=(StreamInsert)node;String layer="0".equals(insert.layer)?parentLayer:insert.layer;
            StreamBlock block=blocks.get(insert.name);
            if(block==null||stack.contains(insert.name)||stack.size()>=32||(block.flags&12)!=0||!block.xref.isEmpty()||
                insert.sx==0||insert.sy==0||insert.sz==0||insert.columns<1||insert.rows<1||insert.columns>1000||insert.rows>1000||
                (long)insert.columns*insert.rows>10000L){
                context.skipped++;continue;
            }
            DxfColor.Ref insertColor=DxfColor.resolve(insert.aci,insert.trueColor,layer,byBlockColor);
            DxfTransparency.Ref insertTransparency=DxfTransparency.resolve(insert.transparencyRaw,layer,byBlockTransparency);
            String insertType=DxfStyle.resolveLineType(insert.lineType,byBlockLineType);
            int insertWeight=DxfStyle.resolveLineWeight(insert.lineWeight,byBlockLineWeight);
            stack.add(insert.name);
            try{
                for(int row=0;row<insert.rows;row++)for(int column=0;column<insert.columns;column++){
                    final DxfBlocks.Transform local;
                    try{
                        local=DxfBlocks.Transform.insert(block.bx,block.by,block.bz,insert.sx,insert.sy,insert.sz,insert.rotation,
                            insert.x,insert.y,insert.z,insert.ex,insert.ey,insert.ez,column*insert.columnSpacing,row*insert.rowSpacing);
                    }catch(IllegalArgumentException invalid){context.skipped++;continue;}
                    DxfBlocks.Transform transform=parent.thenLocal(local);
                    expandStream(block.members,transform,layer,insertColor,insertTransparency,insertType,insertWeight,blocks,stack,entities,layers,visibleLayers,counter,context,viewportModel);
                }
            }finally{stack.remove(insert.name);}
        }
    }

    private static String streamHandle(StreamNode node){
        if(node instanceof StreamShape)return ((StreamShape)node).handle;
        if(node instanceof StreamViewport)return ((StreamViewport)node).handle;
        if(node instanceof StreamInsert)return ((StreamInsert)node).handle;
        if(node instanceof StreamDimension)return ((StreamDimension)node).handle;
        return "";
    }

    private static String key(String value){return value.toUpperCase(Locale.ROOT);}

    private static Result applyUnits(Result result,int insUnits){
        if(result==null)return null;result.insUnits=insUnits;
        if(!DxfSpace.isModel(result.activeLayout)||!DxfUnits.known(insUnits))return result;
        float[] vectors={1,0,0,1};result.view.mapVectors(vectors);
        double scale=(Math.hypot(vectors[0],vectors[1])+Math.hypot(vectors[2],vectors[3]))*.5;
        if(Double.isFinite(scale)&&scale>1e-12){
            result.unitsPerImagePixel=1d/scale;result.unitName=DxfUnits.symbol(insUnits);result.automaticUnits=true;
        }
        return result;
    }

    private static Result finishEntities(ArrayList<Entity> entities,Set<String> layers,Set<String> visibleLayers,int skipped,
                                         Set<String> layoutNames,String activeLayout)throws IOException{
        if(entities.isEmpty())return null;
        RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
        boolean hasVisibleBounds=false;
        for(Entity e:entities){
            FileTransfer.checkCancelled();LayerEntity layer=(LayerEntity)e;if(!visibleLayers.contains(layer.layer))continue;
            layer.bounds(b);hasVisibleBounds=true;
        }
        if(!hasVisibleBounds||!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top){
            b.set(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            for(Entity e:entities){FileTransfer.checkCancelled();e.bounds(b);}
        }
        if(!Float.isFinite(b.left)||!Float.isFinite(b.top)||b.right<b.left||b.bottom<b.top)return null;
        if(b.width()==0){b.left-=.5f;b.right+=.5f;}
        if(b.height()==0){b.top-=.5f;b.bottom+=.5f;}
        float s=Math.min((SIZE-2f*MARGIN)/b.width(),(SIZE-2f*MARGIN)/b.height());
        Matrix m=new Matrix();m.postTranslate(-b.left,-b.bottom);m.postScale(s,-s);
        m.postTranslate(MARGIN+(SIZE-2*MARGIN-b.width()*s)/2f,MARGIN+(SIZE-2*MARGIN-b.height()*s)/2f);
        return renderLayers(entities,m,layers,visibleLayers,skipped,layoutNames,activeLayout);
    }

    private static Result renderLayers(List<Entity> document,Matrix view,Set<String> all,Set<String> visible,int skipped,
                                       Set<String> layoutNames,String activeLayout)throws IOException{
        List<Entity> shown=new ArrayList<>();
        for(Entity entity:document){
            FileTransfer.checkCancelled();
            if(visible.contains(((LayerEntity)entity).layer))shown.add(entity);
        }
        Bitmap bitmap=Bitmap.createBitmap(SIZE,SIZE,Bitmap.Config.ARGB_8888);
        try{
            Canvas canvas=new Canvas(bitmap);canvas.drawColor(BACKGROUND);
            Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2f);
            for(Entity entity:shown){FileTransfer.checkCancelled();entity.draw(canvas,paint,view);}
            FileTransfer.checkCancelled();
            return new Result(bitmap,shown.size(),skipped,snapPoints(shown,view),document,view,all,visible,layoutNames,activeLayout);
        }catch(IOException|RuntimeException|OutOfMemoryError e){bitmap.recycle();throw e;}
    }

    private static float[] snapPoints(List<Entity> entities,Matrix matrix){
        ArrayList<PointF> points=new ArrayList<>();
        for(Entity wrapped:entities){
            Entity entity=wrapped instanceof LayerEntity?((LayerEntity)wrapped).entity:wrapped;
            Matrix transform=new Matrix();ArrayList<ViewportClip> clips=new ArrayList<>();
            while(true){
                if(entity instanceof GeometricWidth){entity=((GeometricWidth)entity).entity;continue;}
                if(entity instanceof VariableWidth){entity=((VariableWidth)entity).centerline;continue;}
                if(entity instanceof Transformed){
                    Transformed wrappedTransform=(Transformed)entity;Matrix combined=new Matrix();
                    combined.setConcat(transform,wrappedTransform.matrix);transform=combined;entity=wrappedTransform.entity;continue;
                }
                if(entity instanceof ViewportClip){
                    ViewportClip viewport=(ViewportClip)entity;Matrix combined=new Matrix();
                    combined.setConcat(transform,viewport.modelToPaper);transform=combined;clips.add(viewport);entity=viewport.entity;continue;
                }
                break;
            }
            double[] candidates=null;
            if(entity instanceof Line){
                Line line=(Line)entity;candidates=DxfSnapGeometry.line(line.x1,line.y1,line.x2,line.y2);
            }else if(entity instanceof InfiniteEntity){
                InfiniteEntity infinite=(InfiniteEntity)entity;candidates=new double[]{infinite.x,infinite.y};
            }else if(entity instanceof LeaderEntity){
                Poly poly=((LeaderEntity)entity).line;double[] xy=new double[poly.pts.size()*2];
                for(int i=0;i<poly.pts.size();i++){xy[i*2]=poly.pts.get(i).x;xy[i*2+1]=poly.pts.get(i).y;}
                candidates=DxfSnapGeometry.poly(xy,false);
            }else if(entity instanceof Poly){
                Poly poly=(Poly)entity;double[] xy=new double[poly.pts.size()*2];
                for(int i=0;i<poly.pts.size();i++){xy[i*2]=poly.pts.get(i).x;xy[i*2+1]=poly.pts.get(i).y;}
                candidates=DxfSnapGeometry.poly(xy,poly.closed);
            }else if(entity instanceof Circle){
                Circle circle=(Circle)entity;candidates=DxfSnapGeometry.circle(circle.x,circle.y,circle.r,circle.start,circle.sweep);
            }else if(entity instanceof EllipseCurve){
                EllipseCurve ellipse=(EllipseCurve)entity;candidates=DxfSnapGeometry.ellipse(ellipse.cx,ellipse.cy,ellipse.mx,ellipse.my,ellipse.ratio,ellipse.start,ellipse.end);
            }else if(entity instanceof Marker){
                Marker marker=(Marker)entity;candidates=new double[]{marker.x,marker.y};
            }else if(entity instanceof FilledPoly){
                FilledPoly poly=(FilledPoly)entity;double[] xy=new double[poly.pts.size()*2];
                for(int i=0;i<poly.pts.size();i++){xy[i*2]=poly.pts.get(i).x;xy[i*2+1]=poly.pts.get(i).y;}
                candidates=DxfSnapGeometry.poly(xy,true);
            }else if(entity instanceof SegmentSet){
                SegmentSet set=(SegmentSet)entity;int segmentCount=Math.min(set.xy.length/4,50000);candidates=new double[segmentCount*6];
                for(int i=0;i<segmentCount;i++){float x1=set.xy[i*4],y1=set.xy[i*4+1],x2=set.xy[i*4+2],y2=set.xy[i*4+3];int q=i*6;
                    candidates[q]=x1;candidates[q+1]=y1;candidates[q+2]=x2;candidates[q+3]=y2;candidates[q+4]=(x1+x2)*.5;candidates[q+5]=(y1+y2)*.5;}
            }
            if(candidates!=null)for(int i=0;i+1<candidates.length;i+=2){
                float[] xy={(float)candidates[i],(float)candidates[i+1]};transform.mapPoints(xy);boolean inside=true;
                for(ViewportClip clip:clips)if(!clip.containsPaper(xy[0],xy[1],1e-6)){inside=false;break;}
                if(inside)points.add(new PointF(xy[0],xy[1]));
                if(points.size()>=250000)break;
            }
            if(points.size()>=250000)break;
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

    private static boolean isTextType(String type){return "TEXT".equals(type)||"MTEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type);}

    private static Entity parseTextEntity(String type,List<String>a,int from,int to,DxfTextStyles.Table styles){
        boolean mtext="MTEXT".equals(type);StringBuilder raw=new StringBuilder();
        for(int i=from;i+1<to;i+=2){int code=intOf(a.get(i));if(code==1||(mtext&&code==3))raw.append(a.get(i+1));}
        String plain=DxfText.plain(raw.toString());if(plain.trim().isEmpty())return null;
        DxfTextStyles.Entry style=styles.get(str(a,from,to,7,DxfTextStyles.STANDARD));
        float height=fv(a,from,to,40,0f);if(height<=0)height=(float)(style.fixedHeight>0?style.fixedHeight:1d);
        float width=(float)style.widthFactor;if(!mtext)width*=Math.max(.01f,fv(a,from,to,41,1f));
        float oblique=has(a,from,to,51)?f(a,from,to,51):(float)style.oblique;
        boolean backwards=false,upsideDown=false;int hAlign=0,vAlign=0,attachment=1;float boxWidth=0,targetWidth=0;
        float x=f(a,from,to,10),y=f(a,from,to,20),angle=0;boolean aligned=false;
        boolean backgroundEnabled=false,backgroundFrame=false;int backgroundColor=BACKGROUND;float backgroundScale=1.5f;
        if(mtext){
            attachment=(int)fv(a,from,to,71,1f);boxWidth=Math.max(0,fv(a,from,to,41,0f));
            int backgroundFlags=(int)fv(a,from,to,90,0f);backgroundEnabled=DxfMTextBackground.enabled(backgroundFlags);backgroundFrame=DxfMTextBackground.frame(backgroundFlags);
            if(backgroundEnabled){
                long backgroundTrueColor=-1;String rawColor=str(a,from,to,421,"").trim();
                if(!rawColor.isEmpty())try{backgroundTrueColor=Long.parseLong(rawColor);}catch(NumberFormatException ignored){}
                backgroundColor=DxfMTextBackground.color(backgroundFlags,(int)fv(a,from,to,63,0f),backgroundTrueColor,BACKGROUND);
            }
            if(backgroundEnabled||backgroundFrame)backgroundScale=(float)DxfMTextBackground.boxScale(fv(a,from,to,45,1.5f));
            if(has(a,from,to,11)&&has(a,from,to,21))angle=(float)Math.toDegrees(Math.atan2(f(a,from,to,21),f(a,from,to,11)));
            else if(has(a,from,to,50))angle=(float)Math.toDegrees(f(a,from,to,50));
        }else{
            hAlign=(int)fv(a,from,to,72,0f);vAlign=(int)fv(a,from,to,73,0f);angle=fv(a,from,to,50,0f);
            int generation=has(a,from,to,71)?(int)fv(a,from,to,71,0f):style.generationFlags;
            backwards=(generation&2)!=0;upsideDown=(generation&4)!=0;
            if(DxfTextLayout.isAlignedOrFit(hAlign)&&has(a,from,to,11)&&has(a,from,to,21)){
                float x2=f(a,from,to,11),y2=f(a,from,to,21),dx=x2-x,dy=y2-y;targetWidth=(float)Math.hypot(dx,dy);
                if(targetWidth>1e-6){angle=(float)Math.toDegrees(Math.atan2(dy,dx));aligned=hAlign==3;hAlign=0;vAlign=0;}
            }else if(DxfTextLayout.usesAlignmentPoint(hAlign,vAlign)&&has(a,from,to,11)&&has(a,from,to,21)){
                x=f(a,from,to,11);y=f(a,from,to,21);
            }
        }
        return new Label(x,y,height,angle,plain,width,oblique,style.familyHint(),hAlign,vAlign,mtext,attachment,boxWidth,
            backwards,upsideDown,targetWidth,aligned,backgroundEnabled,backgroundFrame,backgroundColor,backgroundScale);
    }

    private static Entity projectOcs(String type,Entity entity,List<String>a,int from,int to)throws IOException{
        double[] matrix=ocsMatrix(type,a,from,to);
        return matrix==null?entity:new Transformed(entity,matrix);
    }

    /**
     * DXF uses OCS/ECS coordinates for selected planar entities. WCS-native entities
     * (LINE, POINT, MTEXT, ELLIPSE, SPLINE, LEADER and 3DFACE) must not be transformed here.
     */
    private static double[] ocsMatrix(String type,List<String>a,int from,int to){
        boolean supported="TEXT".equals(type)||"ATTRIB".equals(type)||"ATTDEF".equals(type)||
            "CIRCLE".equals(type)||"ARC".equals(type)||"LWPOLYLINE".equals(type)||"POLYLINE".equals(type)||
            "HATCH".equals(type)||"MPOLYGON".equals(type)||"SOLID".equals(type)||"TRACE".equals(type);
        if(!supported)return null;
        if("POLYLINE".equals(type)){
            int flags=(int)fv(a,from,to,70,0f);
            if((flags&(8|16|64))!=0)return null; // 3D/polyface/mesh coordinates are not planar OCS here.
        }
        double ex=fv(a,from,to,210,0f),ey=fv(a,from,to,220,0f),ez=fv(a,from,to,230,1f);
        if(Math.abs(ex)<1e-12&&Math.abs(ey)<1e-12&&ez>0)return null;
        double elevation="LWPOLYLINE".equals(type)?fv(a,from,to,38,0f):fv(a,from,to,30,0f);
        try{return DxfOcs.plane2d(ex,ey,ez,elevation);}
        catch(IllegalArgumentException invalid){return null;}
    }

    private static Entity parse(String type,List<String>a,int from,int to,DxfPointStyle.Style pointStyle,DxfDimStyles.Table dimStyles){
        if("POINT".equals(type))return new Marker(f(a,from,to,10),f(a,from,to,20),pointStyle);
        if("LEADER".equals(type))return leaderEntity(a,from,to,dimStyles);
        if("MULTILEADER".equals(type))return mLeaderEntity(a,from,to);
        return parse(type,a,from,to);
    }

    private static Entity mLeaderEntity(List<String>a,int from,int to){
        DxfMLeader.Data data=DxfMLeader.parse(a.subList(Math.max(0,from),Math.min(a.size(),to)));ArrayList<Entity> items=new ArrayList<>();
        String text=DxfText.plain(data.text);if(!text.trim().isEmpty()&&Double.isFinite(data.textX)&&Double.isFinite(data.textY)){
            float height=(float)(Double.isFinite(data.textHeight)&&data.textHeight>1e-9?data.textHeight:1d);
            items.add(new Label((float)data.textX,(float)data.textY,height,0,text));
        }
        for(DxfMLeader.Leader leader:data.leaders){
            ArrayList<PointF> points=packedPoints(leader.points);if(points.size()<2)continue;ArrayList<PointF> arrow=new ArrayList<>();
            PointF tip=points.get(points.size()-1),next=points.get(points.size()-2);double adjacent=Math.hypot(next.x-tip.x,next.y-tip.y);
            if(adjacent>1e-9)arrow=packedPoints(DxfLeader.arrow(tip.x,tip.y,next.x,next.y,leader.arrowSize));
            items.add(new LeaderEntity(points,arrow));
        }
        if(items.isEmpty())return null;return items.size()==1?items.get(0):new Composite(items);
    }

    private static Entity leaderEntity(List<String>a,int from,int to,DxfDimStyles.Table dimStyles){
        ArrayList<PointF> raw=repeatedPoints(a,from,to,10,20);if(raw.size()<2)return null;int n=raw.size();double[] xs=new double[n],ys=new double[n];
        for(int i=0;i<n;i++){xs[i]=raw.get(i).x;ys[i]=raw.get(i).y;}
        boolean spline=((int)fv(a,from,to,72,0f))==1;double endTx=Double.NaN,endTy=Double.NaN;int hookDirection=(int)fv(a,from,to,74,0f);
        if(spline&&has(a,from,to,211)&&has(a,from,to,221)){endTx=f(a,from,to,211);endTy=f(a,from,to,221);if(hookDirection==0){endTx=-endTx;endTy=-endTy;}}
        ArrayList<PointF> points=packedPoints(DxfLeader.path(xs,ys,spline,endTx,endTy));if(points.size()<2)points=new ArrayList<>(raw);
        String dimStyle=str(a,from,to,3,"");double styleSize=dimStyles==null?0d:dimStyles.arrowSize(dimStyle);
        ArrayList<PointF> arrow=new ArrayList<>();
        if(((int)fv(a,from,to,71,0f))!=0){
            PointF tip=raw.get(0),rawNext=raw.get(1),direction=points.size()>1?points.get(1):rawNext;double segment=Math.hypot(rawNext.x-tip.x,rawNext.y-tip.y);
            double size=DxfLeader.saneSize(styleSize,segment);arrow=packedPoints(DxfLeader.arrowSized(tip.x,tip.y,direction.x,direction.y,size));
        }
        int annotationType=(int)fv(a,from,to,73,3f);boolean hasHook=((int)fv(a,from,to,75,0f))!=0&&annotationType!=3;
        if(hasHook&&has(a,from,to,211)&&has(a,from,to,221)){
            PointF end=raw.get(n-1),before=raw.get(n-2);double adjacent=Math.hypot(end.x-before.x,end.y-before.y);double size=DxfLeader.saneSize(styleSize,adjacent);
            double[] hook=DxfLeader.hook(end.x,end.y,f(a,from,to,211),f(a,from,to,221),hookDirection!=0,size);
            if(hook.length==4)points.add(new PointF((float)hook[2],(float)hook[3]));
        }
        return new LeaderEntity(points,arrow);
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
        if("RAY".equals(type)||"XLINE".equals(type)){
            float dx=f(a,from,to,11),dy=f(a,from,to,21);if(Math.hypot(dx,dy)<1e-12)return null;
            return new InfiniteEntity(f(a,from,to,10),f(a,from,to,20),dx,dy,"RAY".equals(type));
        }
        if("POINT".equals(type))return new Marker(f(a,from,to,10),f(a,from,to,20),new DxfPointStyle.Style(0,0));
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
        if("LWPOLYLINE".equals(type))return lwPolyline(a,from,to);
        if("SPLINE".equals(type))return splineEntity(a,from,to);
        if("HATCH".equals(type))return parseHatch(a,from,to);
        if("MPOLYGON".equals(type))return parseMPolygon(a,from,to);
        if("WIPEOUT".equals(type))return parseWipeout(a,from,to);
        if("IMAGE".equals(type))return parseImageFrame(a,from,to);
        if("SOLID".equals(type)||"TRACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);return p.size()<3?null:new FilledPoly(p);
        }
        if("3DFACE".equals(type)){
            ArrayList<PointF>p=numberedPoints(a,from,to,10,20,4);
            if(p.size()<3)return null;
            return segmentSet(DxfFace.visibleEdges(pointArray(p),(int)fv(a,from,to,70,0f)));
        }
        // DIMENSION graphics are expanded from their anonymous *D blocks before parse().
        if("DIMENSION".equals(type))return null;
        return null;
    }

    /**
     * HATCH has an elevation point (10/20) before its boundary paths. Treating every
     * 10/20 pair as one polygon connects that elevation origin to unrelated loops and
     * creates the long "spider web" lines seen in some DWGs. Parse each boundary path
     * independently instead. Bulged polyline boundaries are sampled as their true arc geometry, while separate loops
     * are never joined together.
     */
    /** Parse independent HATCH loops, then render SOLID fill or explicit DXF pattern definitions. */
    private static Entity parseHatch(List<String>a,int from,int to){return parseHatchLike(a,from,to,false);}
    private static Entity parseMPolygon(List<String>a,int from,int to){return parseHatchLike(a,from,to,true);}

    private static Entity parseHatchLike(List<String>a,int from,int to,boolean mpolygon){
        int pathCount=(int)fv(a,from,to,91,0f);if(pathCount<=0)return null;
        int i=from;ArrayList<HatchLoop> loops=new ArrayList<>();
        for(int pathIndex=0;pathIndex<pathCount;pathIndex++){
            while(i+1<to&&intOf(a.get(i))!=92)i+=2;
            if(i+1>=to)break;
            int flags=(int)floatOf(a.get(i+1));i+=2;
            if((flags&2)!=0){
                boolean closed=true;int vertexCount=-1;
                while(i+1<to){int code=intOf(a.get(i));if(code==73)closed=((int)floatOf(a.get(i+1)))!=0;
                    if(code==93){vertexCount=(int)floatOf(a.get(i+1));i+=2;break;}if(code==92)break;i+=2;}
                ArrayList<PointF> points=new ArrayList<>();ArrayList<Double> bulges=new ArrayList<>();
                for(int v=0;v<vertexCount&&i+1<to;v++){
                    while(i+1<to&&intOf(a.get(i))!=10){int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=10)break;float x=floatOf(a.get(i+1));i+=2;
                    while(i+1<to&&intOf(a.get(i))!=20){int code=intOf(a.get(i));if(code==92||code==97||code==10)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=20)break;float y=floatOf(a.get(i+1));i+=2;double bulge=0d;
                    if(i+1<to&&intOf(a.get(i))==42){bulge=floatOf(a.get(i+1));i+=2;}
                    if(Float.isFinite(x)&&Float.isFinite(y)){points.add(new PointF(x,y));bulges.add(bulge);}
                }
                if(points.size()>=2){Poly sampled=(Poly)bulgedPoly(points,bulges,closed);loops.add(new HatchLoop(flags,new ArrayList<>(sampled.pts)));}
            }else{
                int edgeCount=-1;
                while(i+1<to){int code=intOf(a.get(i));if(code==93){edgeCount=(int)floatOf(a.get(i+1));i+=2;break;}if(code==92)break;i+=2;}
                ArrayList<PointF> loop=new ArrayList<>();
                for(int edge=0;edge<edgeCount&&i+1<to;edge++){
                    while(i+1<to&&intOf(a.get(i))!=72){int code=intOf(a.get(i));if(code==92||code==97)break;i+=2;}
                    if(i+1>=to||intOf(a.get(i))!=72)break;int edgeType=(int)floatOf(a.get(i+1));int edgeFrom=i+2;i=edgeFrom;
                    while(i+1<to){int code=intOf(a.get(i));if(code==72||code==92||code==97)break;i+=2;}
                    appendHatchEdge(loop,hatchEdgePoints(edgeType,a,edgeFrom,i));
                }
                if(loop.size()>=2)loops.add(new HatchLoop(flags,loop));
            }
        }
        if(loops.isEmpty())return null;
        if(mpolygon){
            double ox=fv(a,i,to,11,0f),oy=fv(a,i,to,21,0f);
            if(Double.isFinite(ox)&&Double.isFinite(oy)&&(Math.abs(ox)>1e-12||Math.abs(oy)>1e-12))
                for(HatchLoop loop:loops)for(PointF q:loop.points){q.x+=ox;q.y+=oy;}
        }
        boolean solid=mpolygon?DxfMPolygon.solid((int)fv(a,from,to,71,0f)):((int)fv(a,from,to,70,0f))!=0;
        int style=mpolygon?0:(int)fv(a,from,to,75,0f);DxfHatchGradient.Data gradient=DxfHatchGradient.parse(a,from,to);
        ArrayList<DxfHatchPattern.Line> pattern=(solid||gradient.enabled)?new ArrayList<>():parseHatchPatternLines(a,i,to);
        int fillAci=mpolygon?(int)fv(a,i,to,63,256f):-1;Hatch fill=new Hatch(loops,solid,style,pattern,gradient,fillAci);
        if(!mpolygon)return fill;
        ArrayList<Entity> items=new ArrayList<>();if(solid||gradient.enabled||!pattern.isEmpty())items.add(fill);
        for(HatchLoop loop:loops)if(loop.points.size()>=2)items.add(new Poly(new ArrayList<>(loop.points),true));
        if(items.isEmpty())return null;return items.size()==1?items.get(0):new Composite(items);
    }

    private static void appendHatchEdge(ArrayList<PointF> loop,ArrayList<PointF> edge){
        if(edge==null||edge.isEmpty())return;if(loop.isEmpty()){loop.addAll(edge);return;}
        PointF last=loop.get(loop.size()-1),first=edge.get(0);int start=Math.hypot(last.x-first.x,last.y-first.y)<1e-4?1:0;
        for(int i=start;i<edge.size();i++)loop.add(edge.get(i));
    }

    private static ArrayList<PointF> hatchEdgePoints(int type,List<String>a,int from,int to){
        ArrayList<PointF> result=new ArrayList<>();
        if(type==1&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,11)&&has(a,from,to,21)){
            result.add(new PointF(f(a,from,to,10),f(a,from,to,20)));result.add(new PointF(f(a,from,to,11),f(a,from,to,21)));return result;
        }
        if(type==2&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,40)){
            double start=f(a,from,to,50),end=f(a,from,to,51);boolean ccw=((int)fv(a,from,to,73,1f))!=0;
            double sweep=end-start;if(ccw){while(sweep<=0)sweep+=360;}else{while(sweep>=0)sweep-=360;}
            int n=Math.max(8,Math.min(128,(int)Math.ceil(Math.abs(sweep)/5d)));double cx=f(a,from,to,10),cy=f(a,from,to,20),r=Math.abs(f(a,from,to,40));
            for(int k=0;k<=n;k++){double q=Math.toRadians(start+sweep*k/n);result.add(new PointF((float)(cx+r*Math.cos(q)),(float)(cy+r*Math.sin(q))));}return result;
        }
        if(type==3&&has(a,from,to,10)&&has(a,from,to,20)&&has(a,from,to,11)&&has(a,from,to,21)){
            double start=f(a,from,to,50),end=f(a,from,to,51);boolean ccw=((int)fv(a,from,to,73,1f))!=0;
            double sweep=end-start;if(ccw){while(sweep<=0)sweep+=Math.PI*2;}else{while(sweep>=0)sweep-=Math.PI*2;}
            int n=Math.max(12,Math.min(128,(int)Math.ceil(Math.abs(sweep)*24/Math.PI)));
            double cx=f(a,from,to,10),cy=f(a,from,to,20),mx=f(a,from,to,11),my=f(a,from,to,21),ratio=Math.abs(fv(a,from,to,40,1f));
            for(int k=0;k<=n;k++){double q=start+sweep*k/n,co=Math.cos(q),si=Math.sin(q);result.add(new PointF((float)(cx+mx*co-my*ratio*si),(float)(cy+my*co+mx*ratio*si)));}return result;
        }
        if(type==4){
            int degree=(int)fv(a,from,to,94,3f);ArrayList<PointF> controls=repeatedPoints(a,from,to,10,20);
            if(controls.size()<2)return result;double[] xs=new double[controls.size()],ys=new double[controls.size()];
            for(int k=0;k<controls.size();k++){xs[k]=controls.get(k).x;ys[k]=controls.get(k).y;}
            double[] knots=repeatedValues(a,from,to,40),weights=repeatedValues(a,from,to,42);if(weights.length!=controls.size())weights=null;
            return packedPoints(DxfCurves.sampleNurbs(degree,knots,weights,xs,ys));
        }
        return result;
    }

    private static ArrayList<DxfHatchPattern.Line> parseHatchPatternLines(List<String>a,int from,int to){
        ArrayList<DxfHatchPattern.Line> lines=new ArrayList<>();int marker=-1,count=0;
        for(int i=Math.max(0,from);i+1<to;i+=2)if(intOf(a.get(i))==78){marker=i;count=(int)floatOf(a.get(i+1));break;}
        if(marker<0||count<=0)return lines;int i=marker+2;
        for(int n=0;n<count;n++){
            while(i+1<to&&intOf(a.get(i))!=53){if(intOf(a.get(i))==98)return lines;i+=2;}
            if(i+1>=to)break;double angle=floatOf(a.get(i+1));int lineFrom=i+2;i=lineFrom;
            while(i+1<to&&intOf(a.get(i))!=53&&intOf(a.get(i))!=98)i+=2;int lineTo=i;
            double bx=fv(a,lineFrom,lineTo,43,0f),by=fv(a,lineFrom,lineTo,44,0f),ox=fv(a,lineFrom,lineTo,45,0f),oy=fv(a,lineFrom,lineTo,46,0f);
            int dashCount=(int)fv(a,lineFrom,lineTo,79,0f);double[] raw=repeatedValues(a,lineFrom,lineTo,49);
            if(dashCount>=0&&raw.length>dashCount)raw=Arrays.copyOf(raw,dashCount);
            if(Double.isFinite(angle)&&Double.isFinite(bx)&&Double.isFinite(by)&&Double.isFinite(ox)&&Double.isFinite(oy))
                lines.add(new DxfHatchPattern.Line(angle,bx,by,ox,oy,raw));
        }
        return lines;
    }

    /** External raster bytes are normally not embedded in DWG/DXF; keep the IMAGE footprint visible as a safe fallback. */
    private static Entity parseImageFrame(List<String>a,int from,int to){
        int display=(int)fv(a,from,to,70,1f);if((display&1)==0)return null;
        ArrayList<double[]> raw=new ArrayList<>();
        if((display&4)!=0){ArrayList<PointF> clip=repeatedPoints(a,from,to,14,24);for(PointF q:clip)raw.add(new double[]{q.x,q.y});}
        double[] packed=DxfWipeout.boundary(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,13,1f),fv(a,from,to,23,1f),raw);
        ArrayList<PointF> points=packedPoints(packed);return points.size()<3?null:new Poly(points,true);
    }

    private static Entity parseWipeout(List<String>a,int from,int to){
        ArrayList<PointF> clip=repeatedPoints(a,from,to,14,24);ArrayList<double[]> raw=new ArrayList<>();
        for(PointF q:clip)raw.add(new double[]{q.x,q.y});
        double[] packed=DxfWipeout.boundary(f(a,from,to,10),f(a,from,to,20),f(a,from,to,11),f(a,from,to,21),
            f(a,from,to,12),f(a,from,to,22),fv(a,from,to,13,1f),fv(a,from,to,23,1f),raw);
        ArrayList<PointF> points=packedPoints(packed);return points.size()<3?null:new Wipeout(points);
    }

    private static Entity lwPolyline(List<String>a,int from,int to){
        ArrayList<PointF> points=new ArrayList<>();ArrayList<Double> bulges=new ArrayList<>(),startWidths=new ArrayList<>(),endWidths=new ArrayList<>();boolean perVertexWidth=false;
        for(int i=from;i+1<to;i+=2){
            if(intOf(a.get(i))!=10)continue;
            float x=floatOf(a.get(i+1));Float y=null;double bulge=0d,startWidth=0d,endWidth=0d;int j=i+2;
            while(j+1<to&&intOf(a.get(j))!=10){
                int code=intOf(a.get(j));
                if(code==20)y=floatOf(a.get(j+1));else if(code==42)bulge=floatOf(a.get(j+1));
                else if(code==40){startWidth=floatOf(a.get(j+1));perVertexWidth=true;}else if(code==41){endWidth=floatOf(a.get(j+1));perVertexWidth=true;}
                j+=2;
            }
            if(y!=null&&Float.isFinite(x)&&Float.isFinite(y)){points.add(new PointF(x,y));bulges.add(bulge);startWidths.add(startWidth);endWidths.add(endWidth);}
            i=j-2;
        }
        if(points.size()<2)return null;boolean closed=(((int)fv(a,from,to,70,0f))&1)!=0;Entity result=bulgedPoly(points,bulges,closed);
        if(perVertexWidth)return variableGeometricWidth(result,points,bulges,startWidths,endWidths,closed);
        return geometricWidth(result,DxfPolylineWidth.constant(fv(a,from,to,43,0f)));
    }

    private static Entity splineEntity(List<String>a,int from,int to){
        boolean closed=(((int)fv(a,from,to,70,0f))&1)!=0;
        int degree=(int)fv(a,from,to,71,3f);
        ArrayList<PointF> controls=repeatedPoints(a,from,to,10,20);
        if(controls.size()>=2){
            double[] xs=new double[controls.size()],ys=new double[controls.size()];
            for(int i=0;i<controls.size();i++){xs[i]=controls.get(i).x;ys[i]=controls.get(i).y;}
            double[] knots=repeatedValues(a,from,to,40),weights=repeatedValues(a,from,to,41);
            if(weights.length!=controls.size())weights=null;
            ArrayList<PointF> sampled=packedPoints(DxfCurves.sampleNurbs(degree,knots,weights,xs,ys));
            if(sampled.size()>=2)return new Poly(sampled,closed);
        }
        ArrayList<PointF> fit=repeatedPoints(a,from,to,11,21);
        if(fit.size()>=2){
            double[] xs=new double[fit.size()],ys=new double[fit.size()];for(int i=0;i<fit.size();i++){xs[i]=fit.get(i).x;ys[i]=fit.get(i).y;}
            double startTx=has(a,from,to,12)&&has(a,from,to,22)?f(a,from,to,12):Double.NaN;
            double startTy=has(a,from,to,12)&&has(a,from,to,22)?f(a,from,to,22):Double.NaN;
            double endTx=has(a,from,to,13)&&has(a,from,to,23)?f(a,from,to,13):Double.NaN;
            double endTy=has(a,from,to,13)&&has(a,from,to,23)?f(a,from,to,23):Double.NaN;
            ArrayList<PointF> sampled=packedPoints(DxfCurves.sampleFitSpline(xs,ys,closed,startTx,startTy,endTx,endTy));
            if(sampled.size()>=2)return new Poly(sampled,closed);
            return new Poly(fit,closed);
        }
        return controls.size()>=2?new Poly(controls,closed):null;
    }

    private static Entity classicPolyline(DxfBlocks.Record header,List<DxfBlocks.Record> records,List<String> tags)throws IOException{
        int flags=(int)header.number(70,0);ArrayList<PointF> points=new ArrayList<>();ArrayList<Double> bulges=new ArrayList<>(),startWidths=new ArrayList<>(),endWidths=new ArrayList<>();ArrayList<int[]> faces=new ArrayList<>();
        double defaultStart=header.number(40,0),defaultEnd=header.number(41,0);boolean vertexWidthOverride=false;
        for(DxfBlocks.Record vertex:records){
            int vf=(int)vertex.number(70,0);
            if((flags&64)!=0&&(vf&128)!=0&&(vf&64)==0){faces.add(new int[]{(int)vertex.number(71,0),(int)vertex.number(72,0),(int)vertex.number(73,0),(int)vertex.number(74,0)});continue;}
            if(!has(tags,vertex.from,vertex.to,10)||!has(tags,vertex.from,vertex.to,20))continue;
            boolean hasStart=has(tags,vertex.from,vertex.to,40),hasEnd=has(tags,vertex.from,vertex.to,41);if(hasStart||hasEnd)vertexWidthOverride=true;
            points.add(new PointF(f(tags,vertex.from,vertex.to,10),f(tags,vertex.from,vertex.to,20)));bulges.add(vertex.number(42,0));
            startWidths.add(hasStart?vertex.number(40,defaultStart):defaultStart);endWidths.add(hasEnd?vertex.number(41,defaultEnd):defaultEnd);
        }
        if((flags&64)!=0)return segmentSet(DxfPolyMesh.polyface(pointArray(points),faces));
        if((flags&16)!=0)return segmentSet(DxfPolyMesh.polygonMesh(pointArray(points),(int)header.number(71,0),(int)header.number(72,0),(flags&1)!=0,(flags&32)!=0));
        boolean closed=(flags&1)!=0;Entity result=points.size()<2?null:bulgedPoly(points,bulges,closed);double uniform=DxfPolylineWidth.uniform(defaultStart,defaultEnd);
        if(!vertexWidthOverride&&uniform>1e-12)return geometricWidth(result,uniform);
        return variableGeometricWidth(result,points,bulges,startWidths,endWidths,closed);
    }

    private static Entity streamPolyline(PendingPoly p){
        if((p.flags&64)!=0)return segmentSet(DxfPolyMesh.polyface(pointArray(p.points),p.faces));
        if((p.flags&16)!=0)return segmentSet(DxfPolyMesh.polygonMesh(pointArray(p.points),p.mCount,p.nCount,(p.flags&1)!=0,(p.flags&32)!=0));
        Entity result=p.points.size()<2?null:bulgedPoly(p.points,p.bulges,p.closed);
        if(!p.variableWidth&&p.geometricWidth>1e-12)return geometricWidth(result,p.geometricWidth);
        return variableGeometricWidth(result,p.points,p.bulges,p.startWidths,p.endWidths,p.closed);
    }

    private static Entity variableGeometricWidth(Entity entity,List<PointF> points,List<Double> bulges,List<Double> startWidths,List<Double> endWidths,boolean closed){
        if(entity==null||points==null||points.size()<2)return entity;int n=points.size();double[] xs=new double[n],ys=new double[n],bs=new double[n],sw=new double[n],ew=new double[n];
        for(int i=0;i<n;i++){PointF q=points.get(i);xs[i]=q.x;ys[i]=q.y;bs[i]=bulges!=null&&i<bulges.size()?bulges.get(i):0d;sw[i]=startWidths!=null&&i<startWidths.size()?startWidths.get(i):0d;ew[i]=endWidths!=null&&i<endWidths.size()?endWidths.get(i):0d;}
        double[][] outlines=DxfPolylineWidth.variable(xs,ys,bs,sw,ew,closed);return outlines.length==0?entity:new VariableWidth(entity,outlines);
    }

    private static Entity geometricWidth(Entity entity,double width){return entity!=null&&DxfPolylineWidth.constant(width)>1e-12?new GeometricWidth(entity,width):entity;}
    private static Entity segmentSet(double[] packed){return packed!=null&&packed.length>=4?new SegmentSet(packed):null;}
    private static double[] pointArray(List<PointF> points){double[] xy=new double[points.size()*2];for(int i=0;i<points.size();i++){xy[i*2]=points.get(i).x;xy[i*2+1]=points.get(i).y;}return xy;}

    private static Entity bulgedPoly(ArrayList<PointF> points,List<Double> bulges,boolean closed){
        int n=points.size();if(n<2)return null;
        double[] xs=new double[n],ys=new double[n],bs=new double[n];
        for(int i=0;i<n;i++){xs[i]=points.get(i).x;ys[i]=points.get(i).y;bs[i]=bulges!=null&&i<bulges.size()?bulges.get(i):0d;}
        ArrayList<PointF> sampled=packedPoints(DxfCurves.sampleBulgePolyline(xs,ys,bs,closed));
        return new Poly(sampled.size()>=2?sampled:points,closed);
    }

    private static ArrayList<PointF> packedPoints(double[] packed){
        ArrayList<PointF> points=new ArrayList<>();
        if(packed==null)return points;
        for(int i=0;i+1<packed.length;i+=2){
            if(Double.isFinite(packed[i])&&Double.isFinite(packed[i+1]))points.add(new PointF((float)packed[i],(float)packed[i+1]));
        }
        return points;
    }

    private static double[] repeatedValues(List<String>a,int from,int to,int wanted){
        ArrayList<Double> values=new ArrayList<>();
        for(int i=from;i+1<to;i+=2)if(intOf(a.get(i))==wanted){try{values.add(Double.parseDouble(a.get(i+1).trim()));}catch(Exception ignored){}}
        double[] result=new double[values.size()];for(int i=0;i<result.length;i++)result[i]=values.get(i);return result;
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
