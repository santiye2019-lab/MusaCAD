package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.*;
import java.util.*;

public class CadView extends View {
    public enum Mode { PAN, CALIBRATE, DISTANCE, AREA }
    public interface Listener {
        void onMeasurement(String value);
        void onCalibrationRequested(double pixelDistance);
        void onSelectionReady();
    }

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final ArrayList<PointF> points = new ArrayList<>();
    private final Matrix imageMatrix = new Matrix();
    private final Matrix inverse = new Matrix();
    private Bitmap drawing;
    private DxfParser.Result vectorDrawing;
    private Mode mode = Mode.PAN;
    private Listener listener;
    private float lastX, lastY, scale = 1f;
    private double unitsPerImagePixel = 1d;
    private String unitName = "piksel";
    private boolean multiTouch;
    private float[] snapPoints=new float[0];
    private boolean snapEnabled=true,lastSnapped;

    private boolean selecting, draggingSelection, exporting;
    private float selectionX, selectionY, selectionEndX, selectionEndY;
    private final ScaleGestureDetector scaleDetector;
    private final GestureDetector gestureDetector;

    public CadView(Context c, AttributeSet a) {
        super(c, a);
        setBackgroundColor(Color.rgb(18,24,30));
        scaleDetector = new ScaleGestureDetector(c, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            public boolean onScale(ScaleGestureDetector d) {
                float next=Math.max(.001f,Math.min(200f,scale*d.getScaleFactor()));
                float f=next/scale; scale=next;
                imageMatrix.postScale(f,f,d.getFocusX(),d.getFocusY());
                invalidate();
                return true;
            }
        });
        gestureDetector = new GestureDetector(c,new GestureDetector.SimpleOnGestureListener(){
            @Override public boolean onDoubleTap(MotionEvent e){
                if(mode!=Mode.PAN||!hasDrawing())return false;
                fit();invalidate();notifyValue();return true;
            }
        });
    }

    private boolean hasDrawing(){return drawing!=null||vectorDrawing!=null;}
    private int contentWidth(){return vectorDrawing!=null?vectorDrawing.contentWidth():drawing!=null?drawing.getWidth():0;}
    private int contentHeight(){return vectorDrawing!=null?vectorDrawing.contentHeight():drawing!=null?drawing.getHeight():0;}

    public void setSnapPoints(float[] points){snapPoints=points==null?new float[0]:points.clone();lastSnapped=false;}
    public void setSnapEnabled(boolean enabled){snapEnabled=enabled;lastSnapped=false;invalidate();}

    public boolean beginSelection(){
        if(!hasDrawing())return false;
        selecting=true;draggingSelection=false;notifyValue();invalidate();return true;
    }
    public void cancelSelection(){selecting=false;draggingSelection=false;notifyValue();invalidate();}

    public void replaceVisibleDrawing(DxfParser.Result result){
        if(vectorDrawing==null||result==null)throw new IllegalArgumentException("Vektör çizim bulunamadı");
        vectorDrawing=result;drawing=null;selecting=false;draggingSelection=false;points.clear();
        setSnapPoints(result.snapPoints);notifyValue();invalidate();
    }

    public void setListener(Listener l){listener=l;}

    public void setMode(Mode m){
        lastSnapped=false;selecting=false;draggingSelection=false;mode=m;points.clear();notifyValue();invalidate();
    }

    public void setDrawing(Bitmap b){
        snapPoints=new float[0];lastSnapped=false;selecting=false;draggingSelection=false;
        vectorDrawing=null;drawing=b;unitsPerImagePixel=1;unitName="piksel";mode=Mode.PAN;points.clear();
        imageMatrix.reset();fit();invalidate();
    }

    public void setVectorDrawing(DxfParser.Result result){
        if(result==null)throw new IllegalArgumentException("Çizim yok");
        snapPoints=result.snapPoints.clone();lastSnapped=false;selecting=false;draggingSelection=false;
        drawing=null;vectorDrawing=result;
        unitsPerImagePixel=result.automaticUnits?result.unitsPerImagePixel:1d;unitName=result.automaticUnits?result.unitName:"piksel";
        mode=Mode.PAN;points.clear();
        imageMatrix.reset();fit();invalidate();
    }

    public void fitToScreen(){if(hasDrawing()){fit();invalidate();notifyValue();}}

    public void zoomBy(float factor){
        if(!hasDrawing()||!Float.isFinite(factor)||factor<=0f)return;
        float next=Math.max(.001f,Math.min(200f,scale*factor));
        float applied=next/scale;scale=next;
        imageMatrix.postScale(applied,applied,getWidth()/2f,getHeight()/2f);
        invalidate();notifyValue();
    }

    public void undo(){
        lastSnapped=false;
        if(selecting){cancelSelection();return;}
        if(!points.isEmpty())points.remove(points.size()-1);
        notifyValue();invalidate();
    }

    public void clearMeasurement(){
        lastSnapped=false;
        if(selecting){cancelSelection();return;}
        points.clear();notifyValue();invalidate();
    }

    public void setCalibration(double realDistance, String unit){
        if(!Double.isFinite(realDistance)||realDistance<=0||points.size()!=2)throw new IllegalArgumentException();
        double px=distance(points.get(0),points.get(1));
        if(px<=0)throw new IllegalArgumentException();
        unitsPerImagePixel=realDistance/px;
        unitName=unit;lastSnapped=false;points.clear();mode=Mode.DISTANCE;notifyValue();invalidate();
    }

    private void fit(){
        int width=contentWidth(),height=contentHeight();
        if(width<=0||height<=0||getWidth()==0||getHeight()==0)return;
        float s=Math.min((float)getWidth()/width,(float)getHeight()/height);
        imageMatrix.reset();
        imageMatrix.postScale(s,s);
        imageMatrix.postTranslate((getWidth()-width*s)/2f,(getHeight()-height*s)/2f);
        scale=s;
    }

    @Override protected void onSizeChanged(int w,int h,int ow,int oh){
        if(selecting)cancelSelection();
        if(ow==0)fit();
    }

    @Override protected void onDraw(Canvas c){
        super.onDraw(c);
        if(vectorDrawing!=null)vectorDrawing.drawVector(c,imageMatrix);
        else if(drawing!=null)c.drawBitmap(drawing,imageMatrix,paint);
        else drawWelcome(c);

        paint.setStrokeWidth(4);paint.setStyle(Paint.Style.STROKE);paint.setColor(Color.rgb(25,181,165));
        ArrayList<PointF> screen=new ArrayList<>();
        for(PointF point:points){
            float[] xy={point.x,point.y};imageMatrix.mapPoints(xy);screen.add(new PointF(xy[0],xy[1]));
        }
        if(screen.size()>1){
            Path p=new Path();p.moveTo(screen.get(0).x,screen.get(0).y);
            for(int i=1;i<screen.size();i++)p.lineTo(screen.get(i).x,screen.get(i).y);
            if(mode==Mode.AREA&&screen.size()>2)p.close();
            c.drawPath(p,paint);
        }
        paint.setStyle(Paint.Style.FILL);
        for(PointF p:screen)c.drawCircle(p.x,p.y,8,paint);

        if(lastSnapped&&!screen.isEmpty()&&!exporting){
            PointF point=screen.get(screen.size()-1);paint.setStyle(Paint.Style.STROKE);
            paint.setColor(Color.WHITE);paint.setStrokeWidth(2);
            c.drawRect(point.x-12,point.y-12,point.x+12,point.y+12,paint);
            paint.setStyle(Paint.Style.FILL);
        }
        if(selecting&&draggingSelection&&!exporting){
            paint.setStyle(Paint.Style.STROKE);paint.setColor(Color.YELLOW);paint.setStrokeWidth(3);
            c.drawRect(Math.min(selectionX,selectionEndX),Math.min(selectionY,selectionEndY),
                Math.max(selectionX,selectionEndX),Math.max(selectionY,selectionEndY),paint);
            paint.setStyle(Paint.Style.FILL);
        }
    }

    private void drawWelcome(Canvas c){
        paint.setTextAlign(Paint.Align.CENTER);paint.setColor(Color.LTGRAY);paint.setTextSize(36);
        c.drawText("MusaCAD",getWidth()/2f,getHeight()/2f-26,paint);
        paint.setTextSize(22);c.drawText("DWG / DXF dosyası açın",getWidth()/2f,getHeight()/2f+18,paint);
        paint.setTextSize(16);paint.setColor(Color.GRAY);c.drawText("İki parmak: yakınlaştır • Çift dokun: sığdır",getWidth()/2f,getHeight()/2f+54,paint);
    }

    @Override public boolean onTouchEvent(MotionEvent e){
        if(!hasDrawing())return true;
        if(selecting)return selectionTouch(e);
        if(mode==Mode.PAN)gestureDetector.onTouchEvent(e);
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN)multiTouch=false;
        if(e.getPointerCount()>1)multiTouch=true;
        scaleDetector.onTouchEvent(e);
        if(multiTouch)return true;

        if(e.getAction()==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();return true;}
        if(e.getAction()==MotionEvent.ACTION_MOVE&&mode==Mode.PAN){
            float dx=e.getX()-lastX,dy=e.getY()-lastY;
            imageMatrix.postTranslate(dx,dy);lastX=e.getX();lastY=e.getY();invalidate();return true;
        }
        if(e.getAction()==MotionEvent.ACTION_UP&&mode!=Mode.PAN){
            if(mode==Mode.CALIBRATE&&points.size()>=2)points.clear();
            float[] xy={e.getX(),e.getY()};
            if(!imageMatrix.invert(inverse))return true;
            inverse.mapPoints(xy);
            if(xy[0]<0||xy[1]<0||xy[0]>contentWidth()||xy[1]>contentHeight())return true;
            int snapped=snapEnabled?SnapPoints.nearest(snapPoints,xy[0],xy[1],scale,
                18*getResources().getDisplayMetrics().density):-1;
            lastSnapped=snapped>=0;
            if(lastSnapped){xy[0]=snapPoints[snapped];xy[1]=snapPoints[snapped+1];}
            points.add(new PointF(xy[0],xy[1]));
            if(mode==Mode.CALIBRATE&&points.size()==2&&listener!=null)
                listener.onCalibrationRequested(distance(points.get(0),points.get(1)));
            notifyValue();invalidate();return true;
        }
        return true;
    }

    private void notifyValue(){
        if(listener==null)return;
        if(selecting){listener.onMeasurement("Alanı sürükleyerek seçin • İptal: GERİ");return;}
        if(mode==Mode.CALIBRATE){
            listener.onMeasurement(points.size()<2?"Bilinen uzunluğun iki ucunu seçin":"Gerçek uzunluğu girin");
        }else if(mode==Mode.DISTANCE){
            double sum=0;for(int i=1;i<points.size();i++)sum+=distance(points.get(i-1),points.get(i));
            listener.onMeasurement(points.size()<2?"Mesafe için en az 2 nokta seçin":String.format(Locale.getDefault(),"Mesafe: %.3f %s",sum*unitsPerImagePixel,unitName));
        }else if(mode==Mode.AREA){
            double a=0;
            if(points.size()>2){
                for(int i=0;i<points.size();i++){PointF p=points.get(i),q=points.get((i+1)%points.size());a+=p.x*q.y-q.x*p.y;}
                a=Math.abs(a)/2*unitsPerImagePixel*unitsPerImagePixel;
            }
            listener.onMeasurement(points.size()<3?"Alan için en az 3 nokta seçin":String.format(Locale.getDefault(),"Alan: %.3f %s²",a,unitName));
        }else{
            listener.onMeasurement("Sürükle: gez • İki parmak: yakınlaştır • Çift dokun: sığdır");
        }
    }

    private boolean selectionTouch(MotionEvent e){
        int action=e.getActionMasked();
        if(action==MotionEvent.ACTION_DOWN){
            selectionX=selectionEndX=e.getX();selectionY=selectionEndY=e.getY();
            draggingSelection=true;invalidate();return true;
        }
        if(action==MotionEvent.ACTION_CANCEL||e.getPointerCount()>1){
            draggingSelection=false;invalidate();return true;
        }
        if(!draggingSelection)return true;
        selectionEndX=e.getX();selectionEndY=e.getY();invalidate();
        if(action==MotionEvent.ACTION_UP){
            if(selectionBounds()==null){
                draggingSelection=false;
                if(listener!=null)listener.onMeasurement("Çizim üzerinde daha geniş bir alan seçin");
            }else if(listener!=null)listener.onSelectionReady();
        }
        return true;
    }

    private SelectionBounds selectionBounds(){
        if(!selecting||!draggingSelection||!hasDrawing())return null;
        SelectionBounds b=SelectionBounds.clip(selectionX,selectionY,selectionEndX,selectionEndY,
            getWidth(),getHeight(),Math.max(8,(int)(8*getResources().getDisplayMetrics().density)));
        if(b==null)return null;
        RectF visible=new RectF(0,0,contentWidth(),contentHeight());imageMatrix.mapRect(visible);
        return RectF.intersects(visible,new RectF(b.left,b.top,b.left+b.width,b.top+b.height))?b:null;
    }

    public Bitmap selectionSnapshot(){
        SelectionBounds bounds=selectionBounds();
        if(bounds==null)throw new IllegalStateException("Önce çizim üzerinde alan seçin");
        Bitmap b=Bitmap.createBitmap(bounds.width,bounds.height,Bitmap.Config.ARGB_8888);
        Canvas canvas=new Canvas(b);canvas.translate(-bounds.left,-bounds.top);
        exporting=true;
        try{draw(canvas);}finally{exporting=false;}
        return b;
    }

    private double distance(PointF a,PointF b){return Math.hypot(a.x-b.x,a.y-b.y);}

    public Bitmap snapshot(){
        if(!hasDrawing())throw new IllegalStateException("Önce çizim açın");
        Bitmap b=Bitmap.createBitmap(getWidth(),getHeight(),Bitmap.Config.ARGB_8888);
        draw(new Canvas(b));return b;
    }
}
