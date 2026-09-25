package com.musa.cad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.MotionEvent;
import android.view.View;
import java.util.Arrays;
import java.util.Locale;

/** Orbitable XYZ surface and wireframe. Measurements use actual drawing coordinates. */
public final class Mesh3dView extends View {
    private final Dxf3dMesh model;
    private final Paint lines=new Paint(Paint.ANTI_ALIAS_FLAG),surface=new Paint(Paint.ANTI_ALIAS_FLAG),marker=new Paint(Paint.ANTI_ALIAS_FLAG),caption=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path facePath=new Path();
    private final float[] screen,depth;
    private final long[] faceOrder;
    private final float[] segments;
    private float yaw=.65f,pitch=.32f,zoom=1f,lastX,lastY,pinchDistance;
    private int first=-1,second=-1;
    private boolean moved,wireframe;

    public Mesh3dView(Context context,Dxf3dMesh model){
        super(context);this.model=model;screen=new float[model.xyz.length/3*2];depth=new float[model.xyz.length/3];faceOrder=new long[model.triangles.length/3];segments=new float[model.edges.length*2];
        lines.setStyle(Paint.Style.STROKE);lines.setColor(Color.rgb(102,211,232));lines.setStrokeWidth(getResources().getDisplayMetrics().density);
        surface.setStyle(Paint.Style.FILL);
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
            screen[j]=getWidth()*.5f+right*scale*perspective;screen[j+1]=getHeight()*.5f-up*scale*perspective;this.depth[i/3]=depth;
        }
        if(wireframe){
            int count=0;for(int i=0;i<model.edges.length;i+=2){int a=model.edges[i]*2,b=model.edges[i+1]*2;segments[count++]=screen[a];segments[count++]=screen[a+1];segments[count++]=screen[b];segments[count++]=screen[b+1];}
            canvas.drawLines(segments,0,count,lines);
        }else drawSurfaces(canvas);
        if(first>=0){int i=first*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);}
        if(second>=0){int i=second*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);float value=distance(first,second);canvas.drawText(String.format(Locale.getDefault(),"3B mesafe: %.3f çizim birimi",value),18f,105f*getResources().getDisplayMetrics().density,caption);}
    }
    private void drawSurfaces(Canvas canvas){
        for(int f=0;f<faceOrder.length;f++){
            int i=f*3,a=model.triangles[i],b=model.triangles[i+1],c=model.triangles[i+2];
            int bucket=(int)(-(depth[a]+depth[b]+depth[c])*333333f/model.radius);
            faceOrder[f]=((long)bucket<<32)|(f&0xffffffffL);
        }
        Arrays.sort(faceOrder);
        for(long key:faceOrder){
            int i=((int)key)*3,a=model.triangles[i],b=model.triangles[i+1],c=model.triangles[i+2];
            float ax=model.xyz[a*3],ay=model.xyz[a*3+1],az=model.xyz[a*3+2];
            float ux=model.xyz[b*3]-ax,uy=model.xyz[b*3+1]-ay,uz=model.xyz[b*3+2]-az;
            float vx=model.xyz[c*3]-ax,vy=model.xyz[c*3+1]-ay,vz=model.xyz[c*3+2]-az;
            float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
            float length=(float)Math.sqrt(nx*nx+ny*ny+nz*nz);
            if(length<1e-9f)continue;
            float light=Math.min(1f,Math.abs((.35f*nx-.4f*ny+.8f*nz)/length));
            surface.setColor(Color.rgb((int)(30+75*light),(int)(77+100*light),(int)(104+105*light)));
            facePath.rewind();facePath.moveTo(screen[a*2],screen[a*2+1]);facePath.lineTo(screen[b*2],screen[b*2+1]);facePath.lineTo(screen[c*2],screen[c*2+1]);facePath.close();canvas.drawPath(facePath,surface);
        }
    }
    private float distance(int a,int b){int i=a*3,j=b*3;float x=model.xyz[i]-model.xyz[j],y=model.xyz[i+1]-model.xyz[j+1],z=model.xyz[i+2]-model.xyz[j+2];return (float)Math.sqrt(x*x+y*y+z*z);}
    public int selectedVertex(){return first;}
    public void geometryChanged(){invalidate();}
    public boolean toggleWireframe(){wireframe=!wireframe;invalidate();return wireframe;}
    public void setWireframe(boolean enabled){wireframe=enabled;invalidate();}
    public void setIsometricView(){yaw=.65f;pitch=.32f;zoom=1f;invalidate();}
    public void setFrontView(){yaw=0f;pitch=0f;zoom=1f;invalidate();}
    public void setTopView(){yaw=0f;pitch=-(float)Math.PI/2f;zoom=1f;invalidate();}
    public void setRightView(){yaw=(float)Math.PI/2f;pitch=0f;zoom=1f;invalidate();}
    public void resetCamera(){setIsometricView();}
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
