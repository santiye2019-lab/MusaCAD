package com.musa.cad;

import android.content.Context;
import android.graphics.*;
import android.util.AttributeSet;
import android.view.*;
import java.util.*;

public class CadView extends View {
    public enum Mode { PAN, CALIBRATE, DISTANCE, AREA }
    public interface Listener { void onMeasurement(String value); void onCalibrationRequested(double pixelDistance); }
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final ArrayList<PointF> points = new ArrayList<>();
    private final Matrix imageMatrix = new Matrix();
    private final Matrix inverse = new Matrix();
    private Bitmap drawing;
    private Mode mode = Mode.PAN;
    private Listener listener;
    private float lastX, lastY, scale = 1f;
    private double unitsPerImagePixel = 1d;
    private String unitName = "piksel";
    private boolean multiTouch;
    private final ScaleGestureDetector scaleDetector;

    public CadView(Context c, AttributeSet a) {
        super(c, a); setBackgroundColor(Color.rgb(18,24,30));
        scaleDetector = new ScaleGestureDetector(c, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            public boolean onScale(ScaleGestureDetector d) {
                float next=Math.max(.001f,Math.min(100f,scale*d.getScaleFactor()));
                float f=next/scale; scale=next;
                imageMatrix.postScale(f,f,d.getFocusX(),d.getFocusY()); invalidate(); return true;
            }
        });
    }
    public void setListener(Listener l){listener=l;}
    public void setMode(Mode m){mode=m; points.clear(); notifyValue(); invalidate();}
    public void setDrawing(Bitmap b){drawing=b; unitsPerImagePixel=1; unitName="piksel"; mode=Mode.PAN; points.clear(); imageMatrix.reset(); fit(); invalidate();}
    public void undo(){if(!points.isEmpty())points.remove(points.size()-1);notifyValue();invalidate();}
    public void clearMeasurement(){points.clear();notifyValue();invalidate();}
    public void setCalibration(double realDistance, String unit){
        if(!Double.isFinite(realDistance)||realDistance<=0||points.size()!=2)throw new IllegalArgumentException();
        double px=distance(points.get(0),points.get(1));
        if(px<=0)throw new IllegalArgumentException();
        unitsPerImagePixel=realDistance/px;
        unitName=unit; points.clear(); mode=Mode.DISTANCE; notifyValue(); invalidate();
    }
    private void fit(){
        if(drawing==null||getWidth()==0||getHeight()==0)return;
        float s=Math.min((float)getWidth()/drawing.getWidth(),(float)getHeight()/drawing.getHeight());
        imageMatrix.reset(); imageMatrix.postScale(s,s); imageMatrix.postTranslate((getWidth()-drawing.getWidth()*s)/2f,(getHeight()-drawing.getHeight()*s)/2f); scale=s;
    }
    protected void onSizeChanged(int w,int h,int ow,int oh){if(ow==0)fit();}
    protected void onDraw(Canvas c){
        super.onDraw(c);
        if(drawing!=null)c.drawBitmap(drawing,imageMatrix,paint); else drawWelcome(c);
        paint.setStrokeWidth(4);paint.setStyle(Paint.Style.STROKE);paint.setColor(Color.rgb(25,181,165));
        ArrayList<PointF> screen=new ArrayList<>();
        for(PointF point:points){float[] xy={point.x,point.y};imageMatrix.mapPoints(xy);screen.add(new PointF(xy[0],xy[1]));}
        if(screen.size()>1){Path p=new Path();p.moveTo(screen.get(0).x,screen.get(0).y);for(int i=1;i<screen.size();i++)p.lineTo(screen.get(i).x,screen.get(i).y);if(mode==Mode.AREA&&screen.size()>2)p.close();c.drawPath(p,paint);}
        paint.setStyle(Paint.Style.FILL); for(PointF p:screen)c.drawCircle(p.x,p.y,8,paint);
    }
    private void drawWelcome(Canvas c){
        paint.setTextAlign(Paint.Align.CENTER);paint.setColor(Color.LTGRAY);paint.setTextSize(38);c.drawText("DWG / DXF görüntüleyici",getWidth()/2f,getHeight()/2f-20,paint);
        paint.setTextSize(25);c.drawText("Dosya Aç düğmesine dokunun",getWidth()/2f,getHeight()/2f+28,paint);
    }
    public boolean onTouchEvent(android.view.MotionEvent e){
        if(drawing==null)return true;
        if(e.getActionMasked()==MotionEvent.ACTION_DOWN)multiTouch=false;
        if(e.getPointerCount()>1)multiTouch=true;
        scaleDetector.onTouchEvent(e); if(multiTouch)return true;
        if(e.getAction()==MotionEvent.ACTION_DOWN){lastX=e.getX();lastY=e.getY();return true;}
        if(e.getAction()==MotionEvent.ACTION_MOVE&&mode==Mode.PAN){float dx=e.getX()-lastX,dy=e.getY()-lastY;imageMatrix.postTranslate(dx,dy);lastX=e.getX();lastY=e.getY();invalidate();return true;}
        if(e.getAction()==MotionEvent.ACTION_UP&&mode!=Mode.PAN){if(mode==Mode.CALIBRATE&&points.size()>=2)points.clear();
            float[] xy={e.getX(),e.getY()};if(!imageMatrix.invert(inverse))return true;inverse.mapPoints(xy);
            if(xy[0]<0||xy[1]<0||xy[0]>drawing.getWidth()||xy[1]>drawing.getHeight())return true;
            points.add(new PointF(xy[0],xy[1]));if(mode==Mode.CALIBRATE&&points.size()==2&&listener!=null)listener.onCalibrationRequested(distance(points.get(0),points.get(1)));notifyValue();invalidate();return true;}
        return true;
    }
    private void notifyValue(){
        if(listener==null)return;
        if(mode==Mode.CALIBRATE) listener.onMeasurement(points.size()<2?"Bilinen uzunluğun iki ucunu seçin":"Gerçek uzunluğu girin");
        else if(mode==Mode.DISTANCE){double sum=0;for(int i=1;i<points.size();i++)sum+=distance(points.get(i-1),points.get(i));listener.onMeasurement(points.size()<2?"Mesafe için en az 2 nokta seçin":String.format(Locale.getDefault(),"Mesafe: %.3f %s",sum*unitsPerImagePixel,unitName));}
        else if(mode==Mode.AREA){double a=0;if(points.size()>2){for(int i=0;i<points.size();i++){PointF p=points.get(i),q=points.get((i+1)%points.size());a+=p.x*q.y-q.x*p.y;}a=Math.abs(a)/2*unitsPerImagePixel*unitsPerImagePixel;}listener.onMeasurement(points.size()<3?"Alan için en az 3 nokta seçin":String.format(Locale.getDefault(),"Alan: %.3f %s²",a,unitName));}
        else listener.onMeasurement("Yakınlaştırmak için iki parmak kullanın");
    }
    private double distance(PointF a,PointF b){return Math.hypot(a.x-b.x,a.y-b.y);}
    public Bitmap snapshot(){if(drawing==null)throw new IllegalStateException("Önce çizim açın");Bitmap b=Bitmap.createBitmap(getWidth(),getHeight(),Bitmap.Config.ARGB_8888);draw(new Canvas(b));return b;}
}
