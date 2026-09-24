package com.musa.cad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;
import java.util.Locale;

/** Orbitable XYZ wireframe. Measurements use actual drawing coordinates. */
public final class Mesh3dView extends View {
    private final Dxf3dMesh model;
    private final Paint lines=new Paint(Paint.ANTI_ALIAS_FLAG),marker=new Paint(Paint.ANTI_ALIAS_FLAG),caption=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final float[] screen;
    private final float[] segments;
    private float yaw=.65f,pitch=.32f,zoom=1f,lastX,lastY,pinchDistance;
    private int first=-1,second=-1;
    private boolean moved;

    public Mesh3dView(Context context,Dxf3dMesh model){
        super(context);this.model=model;screen=new float[model.xyz.length/3*2];segments=new float[model.edges.length*2];
        lines.setStyle(Paint.Style.STROKE);lines.setColor(Color.rgb(102,211,232));lines.setStrokeWidth(getResources().getDisplayMetrics().density);
        marker.setColor(Color.YELLOW);caption.setColor(Color.WHITE);caption.setTextSize(14f*getResources().getDisplayMetrics().scaledDensity);
        setBackgroundColor(Color.rgb(7,19,29));
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);float co=(float)Math.cos(yaw),si=(float)Math.sin(yaw),cp=(float)Math.cos(pitch),sp=(float)Math.sin(pitch);
        float scale=Math.min(getWidth(),getHeight())*zoom/(model.radius*2.6f);
        for(int i=0,j=0;i<model.xyz.length;i+=3,j+=2){
            float x=model.xyz[i]-model.cx,y=model.xyz[i+1]-model.cy,z=model.xyz[i+2]-model.cz;
            float right=co*x-si*y,away=si*x+co*y,up=cp*z-sp*away,depth=sp*z+cp*away;
            float perspective=Math.max(.45f,Math.min(2.5f,1f/(1f+depth/(model.radius*2.5f))));
            screen[j]=getWidth()*.5f+right*scale*perspective;screen[j+1]=getHeight()*.5f-up*scale*perspective;
        }
        int count=0;for(int i=0;i<model.edges.length;i+=2){int a=model.edges[i]*2,b=model.edges[i+1]*2;segments[count++]=screen[a];segments[count++]=screen[a+1];segments[count++]=screen[b];segments[count++]=screen[b+1];}
        canvas.drawLines(segments,0,count,lines);
        if(first>=0){int i=first*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);}
        if(second>=0){int i=second*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);float value=distance(first,second);canvas.drawText(String.format(Locale.getDefault(),"3B mesafe: %.3f çizim birimi",value),18f,getHeight()-22f,caption);}
    }
    private float distance(int a,int b){int i=a*3,j=b*3;float x=model.xyz[i]-model.xyz[j],y=model.xyz[i+1]-model.xyz[j+1],z=model.xyz[i+2]-model.xyz[j+2];return (float)Math.sqrt(x*x+y*y+z*z);}
    public int selectedVertex(){return first;}
    public void geometryChanged(){invalidate();}
    private void select(float x,float y){int at=-1;float best=36f*getResources().getDisplayMetrics().density;best*=best;
        for(int i=0;i<screen.length;i+=2){float dx=screen[i]-x,dy=screen[i+1]-y,dist=dx*dx+dy*dy;if(dist<best){best=dist;at=i/2;}}
        if(at<0)return;if(first<0||second>=0){first=at;second=-1;}else second=at;invalidate();
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        switch(e.getActionMasked()){
            case MotionEvent.ACTION_DOWN:lastX=e.getX();lastY=e.getY();moved=false;return true;
            case MotionEvent.ACTION_POINTER_DOWN:if(e.getPointerCount()>=2)pinchDistance=spacing(e);return true;
            case MotionEvent.ACTION_MOVE:
                if(e.getPointerCount()>=2){float next=spacing(e);if(pinchDistance>4f)zoom=Math.max(.15f,Math.min(25f,zoom*next/pinchDistance));pinchDistance=next;moved=true;invalidate();}
                else{float dx=e.getX()-lastX,dy=e.getY()-lastY;if(Math.abs(dx)+Math.abs(dy)>2f)moved=true;yaw+=dx*.008f;pitch=Math.max(-1.5f,Math.min(1.5f,pitch+dy*.008f));lastX=e.getX();lastY=e.getY();invalidate();}return true;
            case MotionEvent.ACTION_UP:if(!moved)select(e.getX(),e.getY());performClick();return true;
            case MotionEvent.ACTION_CANCEL:return true;
            default:return true;
        }
    }
    private static float spacing(MotionEvent e){float dx=e.getX(0)-e.getX(1),dy=e.getY(0)-e.getY(1);return (float)Math.hypot(dx,dy);}
    @Override public boolean performClick(){super.performClick();return true;}
}
