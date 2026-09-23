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
    /** Packed primitive bounds: left, top, right, bottom. Avoids one RectF object per entity. */
    private final float[] bounds;
    private final Matrix worldToContent;
    private final RectF worldBounds;
    private final Grid grid;
    private final Paint drawPaint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Matrix combinedMatrix=new Matrix(),inverseMatrix=new Matrix(),localMatrix=new Matrix(),targetMatrix=new Matrix();
    private final RectF visibleRect=new RectF();
    private final float[] drawLine=new float[4],drawPoint=new float[2];
    private final Path drawPath=new Path();
    public final int primitiveCount;
    public final boolean truncated;

    static NativeScene fromRaw(float[] values)throws IOException{
        if(values==null||values.length<6)throw new IOException("Native sahne üretilemedi");
        int version=Math.round(values[0]);
        if(version!=1&&version!=2&&version!=3&&version!=4)throw new IOException("Native sahne sürümü desteklenmiyor");
        int expected=Math.max(0,Math.round(values[1]));
        RectF wb=new RectF(values[2],values[3],values[4],values[5]);
        if(!finite(wb.left)||!finite(wb.top)||!finite(wb.right)||!finite(wb.bottom)||wb.width()<=0||wb.height()<=0)throw new IOException("Native çizim sınırları geçersiz");
        boolean truncated=version>=2&&values.length>=7&&values[6]!=0f;
        int p=version>=2?7:6;
        int maxPossible=Math.max(1,(values.length-p)/(version>=3?3:4));
        int initial=expected>0?Math.min(expected,maxPossible):Math.min(1024,maxPossible);
        int[] os=new int[Math.max(1,initial)];
        float[] bs=new float[Math.max(4,os.length*4)];
        int count=0;
        RectF b=new RectF();
        while(p<values.length){
            int start=p,encoded=Math.round(values[p++]),type;
            if(version>=4&&encoded<0){
                type=(-encoded-1)>>>8;if(p>=values.length)break;p++;
            }else if(version>=3)type=encoded>>>8;
            else{type=encoded;if(p>=values.length)break;p++;}
            b.set(Float.MAX_VALUE,Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE);
            if(type==1){
                if(p+4>values.length)break;float x1=values[p++],y1=values[p++],x2=values[p++],y2=values[p++];add(b,x1,y1);add(b,x2,y2);
            }else if(type==2){
                int n;
                if(version>=3){if(p>=values.length)break;n=Math.abs(Math.round(values[p++]));}
                else{if(p+2>values.length)break;p++;n=Math.round(values[p++]);}
                if(n<2||p+n*2>values.length)break;for(int i=0;i<n;i++)add(b,values[p++],values[p++]);
            }else if(type==3){
                if(p+2>values.length)break;add(b,values[p++],values[p++]);
            }else if(type==4){
                if(p+8>values.length)break;float cx=values[p++],cy=values[p++],ux=values[p++],uy=values[p++],vx=values[p++],vy=values[p++];p+=2;
                float rx=(float)Math.hypot(ux,vx),ry=(float)Math.hypot(uy,vy);add(b,cx-rx,cy-ry);add(b,cx+rx,cy+ry);
            }else break;
            if(valid(b)){
                if(count==os.length){
                    int next=Math.min(maxPossible,Math.max(count+1,os.length+(os.length>>1)+1));
                    os=Arrays.copyOf(os,next);bs=Arrays.copyOf(bs,next*4);
                }
                os[count]=start;int k=count*4;bs[k]=b.left;bs[k+1]=b.top;bs[k+2]=b.right;bs[k+3]=b.bottom;count++;
            }
        }
        if(count==0)throw new IOException("Native sahnede görüntülenebilir geometri yok");
        if(count!=os.length){os=Arrays.copyOf(os,count);bs=Arrays.copyOf(bs,count*4);}
        int[] offsets=os;float[] bounds=bs;
        Matrix view=new Matrix();float scale=Math.min((SIZE-2f*MARGIN)/wb.width(),(SIZE-2f*MARGIN)/wb.height());
        view.postTranslate(-wb.left,-wb.bottom);view.postScale(scale,-scale);view.postTranslate(MARGIN+(SIZE-2*MARGIN-wb.width()*scale)/2f,MARGIN+(SIZE-2*MARGIN-wb.height()*scale)/2f);
        return new NativeScene(values,offsets,bounds,view,wb,truncated||expected>offsets.length,version);
    }

    private final int streamVersion;
    private NativeScene(float[] raw,int[] offsets,float[] bounds,Matrix view,RectF worldBounds,boolean truncated,int streamVersion){
        this.raw=raw;this.offsets=offsets;this.bounds=bounds;this.worldToContent=new Matrix(view);this.worldBounds=new RectF(worldBounds);this.primitiveCount=offsets.length;this.truncated=truncated;this.streamVersion=streamVersion;grid=new Grid(bounds,worldBounds);
    }

    public int contentWidth(){return SIZE;}public int contentHeight(){return SIZE;}

    /** Small vector thumbnail without allocating the old full-size raster preview. */
    public Bitmap thumbnail(int width,int height){
        int w=Math.max(1,width),h=Math.max(1,height);Bitmap out=Bitmap.createBitmap(w,h,Bitmap.Config.RGB_565);Canvas canvas=new Canvas(out);canvas.drawColor(Color.rgb(18,24,30));
        Matrix fit=new Matrix();fit.setRectToRect(new RectF(0f,0f,SIZE,SIZE),new RectF(0f,0f,w,h),Matrix.ScaleToFit.CENTER);
        Matrix combined=new Matrix();combined.setConcat(fit,worldToContent);Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.15f);
        float[]line=new float[4],point=new float[2];Path path=new Path();Matrix local=new Matrix(),target=new Matrix();
        int stride=Math.max(1,(offsets.length+59999)/60000);
        for(int i=0;i<offsets.length;i+=stride)drawPrimitive(i,canvas,paint,combined,line,point,path,local,target);
        return out;
    }

    public PointF contentToWorld(float x,float y){
        float[]pt={x,y};Matrix inv=new Matrix();if(!worldToContent.invert(inv))return new PointF(x,y);inv.mapPoints(pt);return new PointF(pt[0],pt[1]);
    }
    public PointF worldToContent(float x,float y){float[]pt={x,y};worldToContent.mapPoints(pt);return new PointF(pt[0],pt[1]);}
    public float drawingToContentScale(){float[]v=new float[9];worldToContent.getValues(v);return Math.max(1e-9f,(float)Math.hypot(v[Matrix.MSCALE_X],v[Matrix.MSKEW_Y]));}
    /** Once the full DXF model is ready, use its exact world-to-content transform for gesture previews. */
    public void alignTo(Matrix drawingToContent){if(drawingToContent!=null)worldToContent.set(drawingToContent);}

    public void draw(Canvas canvas,Matrix contentToScreen){
        if(canvas==null||contentToScreen==null)return;
        combinedMatrix.setConcat(contentToScreen,worldToContent);
        Rect clip=canvas.getClipBounds();visibleRect.set(clip);RectF visible=null;float worldPerPixel=0f;
        if(combinedMatrix.invert(inverseMatrix)){
            inverseMatrix.mapRect(visibleRect);
            if(clip.width()>0&&clip.height()>0)worldPerPixel=Math.max(visibleRect.width()/clip.width(),visibleRect.height()/clip.height());
            float pad=worldPerPixel>0f?worldPerPixel*12f:0f;visibleRect.inset(-pad,-pad);visible=visibleRect;
        }
        drawPaint.reset();drawPaint.setAntiAlias(true);drawPaint.setStyle(Paint.Style.STROKE);drawPaint.setStrokeWidth(1.15f);
        float minWorldSpan=visible!=null&&offsets.length>50000&&worldPerPixel>0f?worldPerPixel*.18f:0f;
        grid.draw(canvas,drawPaint,combinedMatrix,visible,drawLine,drawPoint,drawPath,localMatrix,targetMatrix,minWorldSpan);
    }

    private void drawPrimitive(int index,Canvas canvas,Paint paint,Matrix matrix,float[] line,float[] point,Path path,Matrix local,Matrix target){
        int p=offsets[index],encoded=Math.round(raw[p++]),type,color;
        if(streamVersion>=4&&encoded<0){
            type=(-encoded-1)>>>8;color=DxfColor.trueColorArgb(Math.round(raw[p++]));
        }else if(streamVersion>=3){type=encoded>>>8;color=DxfColor.aciArgb(encoded&255);}
        else{type=encoded;color=DxfColor.aciArgb(Math.round(raw[p++]));}
        paint.setColor(color);
        if(type==1){
            line[0]=raw[p++];line[1]=raw[p++];line[2]=raw[p++];line[3]=raw[p++];matrix.mapPoints(line);canvas.drawLine(line[0],line[1],line[2],line[3],paint);
        }else if(type==2){
            boolean closed;int n;
            if(streamVersion>=3){int signed=Math.round(raw[p++]);closed=signed<0;n=Math.abs(signed);}else{closed=raw[p++]!=0;n=Math.round(raw[p++]);}
            path.rewind();for(int i=0;i<n;i++){point[0]=raw[p++];point[1]=raw[p++];matrix.mapPoints(point);if(i==0)path.moveTo(point[0],point[1]);else path.lineTo(point[0],point[1]);}if(closed)path.close();canvas.drawPath(path,paint);
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
        Grid(float[] packedBounds,RectF area){
            this.area=new RectF(area);int count=packedBounds.length/4;cells=count>60000?64:count>10000?48:32;cw=Math.max(1e-9f,area.width()/cells);ch=Math.max(1e-9f,area.height()/cells);epsilon=Math.max(1e-7f,Math.max(area.width(),area.height())*1e-7f);buckets=new IntList[cells*cells];seen=new int[count];
            for(int i=0;i<count;i++){
                int k=i*4;float left=packedBounds[k],top=packedBounds[k+1],right=packedBounds[k+2],bottom=packedBounds[k+3];
                int x0=x(left),x1=x(right),y0=y(top),y1=y(bottom);int span=(x1-x0+1)*(y1-y0+1);if(span>64){overflow.add(i);continue;}
                for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++){int at=yy*cells+xx;if(buckets[at]==null)buckets[at]=new IntList();buckets[at].add(i);}
            }
        }
        void draw(Canvas c,Paint p,Matrix m,RectF visible,float[]line,float[]point,Path path,Matrix local,Matrix target,float minWorldSpan){
            int mark=nextMark();
            if(visible==null){for(int i=0;i<offsets.length;i++)drawOne(i,c,p,m,line,point,path,local,target,mark,null,minWorldSpan);return;}
            drawList(overflow,c,p,m,line,point,path,local,target,mark,visible,minWorldSpan);if(!overlaps(area,visible,epsilon))return;int x0=x(visible.left),x1=x(visible.right),y0=y(visible.top),y1=y(visible.bottom);for(int yy=y0;yy<=y1;yy++)for(int xx=x0;xx<=x1;xx++)drawList(buckets[yy*cells+xx],c,p,m,line,point,path,local,target,mark,visible,minWorldSpan);
        }
        void drawList(IntList list,Canvas c,Paint p,Matrix m,float[]line,float[]point,Path path,Matrix local,Matrix target,int mark,RectF visible,float minWorldSpan){if(list==null)return;for(int i=0;i<list.size;i++)drawOne(list.data[i],c,p,m,line,point,path,local,target,mark,visible,minWorldSpan);}
        void drawOne(int index,Canvas c,Paint p,Matrix m,float[]line,float[]point,Path path,Matrix local,Matrix target,int mark,RectF visible,float minWorldSpan){
            if(seen[index]==mark)return;seen[index]=mark;
            if(visible!=null&&!overlaps(bounds,index,visible,epsilon))return;
            if(minWorldSpan>0f&&tooSmall(bounds,index,minWorldSpan))return;
            drawPrimitive(index,c,p,m,line,point,path,local,target);
        }
        int nextMark(){if(query==Integer.MAX_VALUE){Arrays.fill(seen,0);query=1;}return ++query;}
        int x(float v){return Math.max(0,Math.min(cells-1,(int)((v-area.left)/cw)));}int y(float v){return Math.max(0,Math.min(cells-1,(int)((v-area.top)/ch)));}
    }
    private static final class IntList{int[]data=new int[8];int size;void add(int v){if(size==data.length)data=Arrays.copyOf(data,data.length*2);data[size++]=v;}}

    private static boolean overlaps(RectF a,RectF b,float e){return a.right+e>=b.left&&b.right+e>=a.left&&a.bottom+e>=b.top&&b.bottom+e>=a.top;}
    private static boolean overlaps(float[] packed,int index,RectF b,float e){int k=index*4;return packed[k+2]+e>=b.left&&b.right+e>=packed[k]&&packed[k+3]+e>=b.top&&b.bottom+e>=packed[k+1];}
    private static boolean tooSmall(float[] packed,int index,float minSpan){int k=index*4;return packed[k+2]-packed[k]<minSpan&&packed[k+3]-packed[k+1]<minSpan;}
    private static boolean finite(float v){return Float.isFinite(v);}private static boolean valid(RectF b){return finite(b.left)&&finite(b.top)&&finite(b.right)&&finite(b.bottom)&&b.right>=b.left&&b.bottom>=b.top;}
    private static void add(RectF b,float x,float y){if(!finite(x)||!finite(y))return;b.left=Math.min(b.left,x);b.top=Math.min(b.top,y);b.right=Math.max(b.right,x);b.bottom=Math.max(b.bottom,y);}
}
