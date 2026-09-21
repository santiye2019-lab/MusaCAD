package com.musa.cad;

import android.graphics.*;
import java.io.IOException;
import java.util.*;

/** Lightweight direct-DWG scene used for fast first paint and high-speed navigation. */
public final class NativeScene {
    private static final int SIZE=2400,MARGIN=80;
    private static final RectF UNIT_OVAL=new RectF(-1f,-1f,1f,1f);
    private final float[] raw;
    private final int[] offsets;
    private final RectF[] bounds;
    private final Matrix worldToContent;
    private final RectF worldBounds;
    private final Grid grid;
    public final int primitiveCount;
    public final boolean truncated;

    static NativeScene fromRaw(float[] values)throws IOException{
        if(values==null||values.length<6)throw new IOException("Native sahne üretilemedi");
        int version=Math.round(values[0]);
        if(version!=1&&version!=2)throw new IOException("Native sahne sürümü desteklenmiyor");
        int expected=Math.max(0,Math.round(values[1]));
        RectF wb=new RectF(values[2],values[3],values[4],values[5]);
        if(!finite(wb.left)||!finite(wb.top)||!finite(wb.right)||!finite(wb.bottom)||wb.width()<=0||wb.height()<=0)throw new IOException("Native çizim sınırları geçersiz");
        boolean truncated=version>=2&&values.length>=7&&values[6]!=0f;
        int p=version>=2?7:6;
        ArrayList<Integer> os=new ArrayList<>();ArrayList<RectF> bs=new ArrayList<>();
        while(p<values.length){
            int start=p,type=Math.round(values[p++]);RectF b=new RectF(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            if(type==1){
                if(p+5>values.length)break;p++;float x1=values[p++],y1=values[p++],x2=values[p++],y2=values[p++];add(b,x1,y1);add(b,x2,y2);
            }else if(type==2){
                if(p+3>values.length)break;p+=2;int n=Math.round(values[p++]);if(n<2||p+n*2>values.length)break;for(int i=0;i<n;i++)add(b,values[p++],values[p++]);
            }else if(type==3){
                if(p+3>values.length)break;p++;add(b,values[p++],values[p++]);
            }else if(type==4){
                if(p+9>values.length)break;p++;float cx=values[p++],cy=values[p++],ux=values[p++],uy=values[p++],vx=values[p++],vy=values[p++];p+=2;
                float rx=(float)Math.hypot(ux,vx),ry=(float)Math.hypot(uy,vy);add(b,cx-rx,cy-ry);add(b,cx+rx,cy+ry);
            }else break;
            if(valid(b)){os.add(start);bs.add(b);}
        }
        if(os.isEmpty())throw new IOException("Native sahnede görüntülenebilir geometri yok");
        int[] offsets=new int[os.size()];RectF[] bounds=new RectF[os.size()];for(int i=0;i<offsets.length;i++){offsets[i]=os.get(i);bounds[i]=bs.get(i);}
        Matrix view=new Matrix();float scale=Math.min((SIZE-2f*MARGIN)/wb.width(),(SIZE-2f*MARGIN)/wb.height());
        view.postTranslate(-wb.left,-wb.bottom);view.postScale(scale,-scale);view.postTranslate(MARGIN+(SIZE-2*MARGIN-wb.width()*scale)/2f,MARGIN+(SIZE-2*MARGIN-wb.height()*scale)/2f);
        return new NativeScene(values,offsets,bounds,view,wb,truncated||expected>offsets.length);
    }

    private NativeScene(float[] raw,int[] offsets,RectF[] bounds,Matrix view,RectF worldBounds,boolean truncated){
        this.raw=raw;this.offsets=offsets;this.bounds=bounds;this.worldToContent=new Matrix(view);this.worldBounds=new RectF(worldBounds);this.primitiveCount=offsets.length;this.truncated=truncated;grid=new Grid(bounds,worldBounds);
    }

    public int contentWidth(){return SIZE;}public int contentHeight(){return SIZE;}

    public PointF contentToWorld(float x,float y){
        float[]pt={x,y};Matrix inv=new Matrix();if(!worldToContent.invert(inv))return new PointF(x,y);inv.mapPoints(pt);return new PointF(pt[0],pt[1]);
    }
    public PointF worldToContent(float x,float y){float[]pt={x,y};worldToContent.mapPoints(pt);return new PointF(pt[0],pt[1]);}
    public float drawingToContentScale(){float[]v=new float[9];worldToContent.getValues(v);return Math.max(1e-9f,(float)Math.hypot(v[Matrix.MSCALE_X],v[Matrix.MSKEW_Y]));}
    /** Once the full DXF model is ready, use its exact world-to-content transform for gesture previews. */
    public void alignTo(Matrix drawingToContent){if(drawingToContent!=null)worldToContent.set(drawingToContent);}

    public void draw(Canvas canvas,Matrix contentToScreen){
        if(canvas==null||contentToScreen==null)return;
        Matrix combined=new Matrix();combined.setConcat(contentToScreen,worldToContent);
        Rect clip=canvas.getClipBounds();RectF visible=new RectF(clip);Matrix inv=new Matrix();if(combined.invert(inv)){inv.mapRect(visible);float pad=Math.max(worldBounds.width(),worldBounds.height())*.001f;visible.inset(-pad,-pad);}else visible=null;
        Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.15f);grid.draw(canvas,paint,combined,visible);
    }

    private void drawPrimitive(int index,Canvas canvas,Paint paint,Matrix matrix,float[] line,float[] point,Path path,Matrix local,Matrix target){
        int p=offsets[index],type=Math.round(raw[p++]),aci=Math.round(raw[p++]);paint.setColor(DxfColor.aciArgb(aci));
        if(type==1){
            line[0]=raw[p++];line[1]=raw[p++];line[2]=raw[p++];line[3]=raw[p++];matrix.mapPoints(line);canvas.drawLine(line[0],line[1],line[2],line[3],paint);
        }else if(type==2){
            boolean closed=raw[p++]!=0;int n=Math.round(raw[p++]);path.rewind();for(int i=0;i<n;i++){point[0]=raw[p++];point[1]=raw[p++];matrix.mapPoints(point);if(i==0)path.moveTo(point[0],point[1]);else path.lineTo(point[0],point[1]);}if(closed)path.close();canvas.drawPath(path,paint);
        }else if(type==3){
            point[0]=raw[p++];point[1]=raw[p++];matrix.mapPoints(point);float r=3.5f;canvas.drawLine(point[0]-r,point[1],point[0]+r,point[1],paint);canvas.drawLine(point[0],point[1]-r,point[0],point[1]+r,paint);
        }else if(type==4){
            float cx=raw[p++],cy=raw[p++],ux=raw[p++],uy=raw[p++],vx=raw[p++],vy=raw[p++],start=raw[p++],sweep=raw[p++];
            path.rewind();path.addArc(UNIT_OVAL,(float)Math.toDegrees(start),(float)Math.toDegrees(sweep));
            local.setValues(new float[]{ux,vx,cx,uy,vy,cy,0f,0f,1f});target.setConcat(matrix,local);path.transform(target);canvas.drawPath(path,paint);
        }
    }

    private final class Grid {
        final int cells;final RectF area;final float cw,ch,epsilon;final IntList[] buckets;final IntList overflow=new IntList();final int[] seen;int query=1;
        Grid(RectF[] bounds,RectF area){
            this.area=new RectF(area);cells=bounds.length>60000?64:bounds.length>10000?48:32;cw=Math.max(1e-9f,area.width()/cells);ch=Math.max(1e-9f,area.height()/cells);epsilon=Math.max(1e-7f,Math.max(area.width(),area.height())*1e-7f);buckets=new IntList[cells*cells];seen=new int[bounds.length];
            for(int i=0;i<bounds.length;i++){RectF b=bounds[i];int x0=x(b.left),x1=x(b.right),y0=y(b.top),y1=y(b.bottom);int span=(x1-x0+1)*(y1-y0+1);if(span>64){overflow.add(i);continue;}for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++){int at=yy*cells+xx;if(buckets[at]==null)buckets[at]=new IntList();buckets[at].add(i);}}
        }
        void draw(Canvas c,Paint p,Matrix m,RectF visible){
            int mark=nextMark();float[]line=new float[4],point=new float[2];Path path=new Path();Matrix local=new Matrix(),target=new Matrix();
            if(visible==null){for(int i=0;i<offsets.length;i++)drawOne(i,c,p,m,line,point,path,local,target,mark,null);return;}
            drawList(overflow,c,p,m,line,point,path,local,target,mark,visible);if(!overlaps(area,visible,epsilon))return;int x0=x(visible.left),x1=x(visible.right),y0=y(visible.top),y1=y(visible.bottom);for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++)drawList(buckets[yy*cells+xx],c,p,m,line,point,path,local,target,mark,visible);
        }
        void drawList(IntList list,Canvas c,Paint p,Matrix m,float[]line,float[]point,Path path,Matrix local,Matrix target,int mark,RectF visible){if(list==null)return;for(int i=0;i<list.size;i++)drawOne(list.data[i],c,p,m,line,point,path,local,target,mark,visible);}
        void drawOne(int index,Canvas c,Paint p,Matrix m,float[]line,float[]point,Path path,Matrix local,Matrix target,int mark,RectF visible){if(seen[index]==mark)return;seen[index]=mark;if(visible!=null&&!overlaps(bounds[index],visible,epsilon))return;drawPrimitive(index,c,p,m,line,point,path,local,target);}
        int nextMark(){if(query==Integer.MAX_VALUE){Arrays.fill(seen,0);query=1;}return ++query;}
        int x(float v){return Math.max(0,Math.min(cells-1,(int)((v-area.left)/cw)));}int y(float v){return Math.max(0,Math.min(cells-1,(int)((v-area.top)/ch)));}
    }
    private static final class IntList{int[]data=new int[8];int size;void add(int v){if(size==data.length)data=Arrays.copyOf(data,data.length*2);data[size++]=v;}}

    private static boolean overlaps(RectF a,RectF b,float e){return a.right+e>=b.left&&b.right+e>=a.left&&a.bottom+e>=b.top&&b.bottom+e>=a.top;}
    private static boolean finite(float v){return Float.isFinite(v);}private static boolean valid(RectF b){return finite(b.left)&&finite(b.top)&&finite(b.right)&&finite(b.bottom)&&b.right>=b.left&&b.bottom>=b.top;}
    private static void add(RectF b,float x,float y){if(!finite(x)||!finite(y))return;b.left=Math.min(b.left,x);b.top=Math.min(b.top,y);b.right=Math.max(b.right,x);b.bottom=Math.max(b.bottom,y);}
}
