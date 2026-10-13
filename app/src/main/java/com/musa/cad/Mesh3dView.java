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
    public enum InteractionMode { ORBIT, MEASURE, EDIT }
    private final Dxf3dMesh model;
    private final Paint lines=new Paint(Paint.ANTI_ALIAS_FLAG),surface=new Paint(Paint.ANTI_ALIAS_FLAG),marker=new Paint(Paint.ANTI_ALIAS_FLAG),caption=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path facePath=new Path();
    private final float[] screen,depth;
    private final long[] faceOrder;
    private final float[] segments;
    private float yaw=.65f,pitch=.32f,zoom=1f,lastX,lastY,pinchDistance;
    private int first=-1,second=-1;
    private boolean moved,wireframe;
    private InteractionMode interactionMode=InteractionMode.ORBIT;
    /** The Activity owns source-validation and decides whether an edit can commit. */
    public interface AxisDragListener {boolean moveVertex(int vertex,double x,double y,double z);}
    /** Called exactly once for a completed pointer drag; never for preview moves/cancel. */
    public interface AxisDragCommitListener {void finished(int vertex,float[] before,float[] after);}
    private AxisDragListener axisDragListener;
    private AxisDragCommitListener axisDragCommitListener;
    private CadUcsFrame gizmoUcs=CadUcsFrame.world();
    private final Paint[] gizmoPaints={new Paint(Paint.ANTI_ALIAS_FLAG),
        new Paint(Paint.ANTI_ALIAS_FLAG),new Paint(Paint.ANTI_ALIAS_FLAG)};
    private boolean axisDragging;
    private int dragAxis=-1,dragVertex=-1;
    private float dragX,dragY;
    private final float[] dragOriginal=new float[3];

    public Mesh3dView(Context context,Dxf3dMesh model){
        super(context);this.model=model;screen=new float[model.xyz.length/3*2];depth=new float[model.xyz.length/3];faceOrder=new long[model.triangles.length/3];segments=new float[model.edges.length*2];
        lines.setStyle(Paint.Style.STROKE);lines.setColor(Color.rgb(102,211,232));lines.setStrokeWidth(getResources().getDisplayMetrics().density);
        surface.setStyle(Paint.Style.FILL);
        marker.setColor(Color.YELLOW);caption.setColor(Color.WHITE);caption.setTextSize(14f*getResources().getDisplayMetrics().scaledDensity);
        setBackgroundColor(Color.rgb(7,19,29));
        wireframe=model.triangles.length==0;
        int[] colors={Color.rgb(246,89,89),Color.rgb(91,225,133),Color.rgb(89,168,255)};
        for(int k=0;k<3;k++){
            gizmoPaints[k].setColor(colors[k]);
            gizmoPaints[k].setStrokeWidth(3f*getResources().getDisplayMetrics().density);
            gizmoPaints[k].setStrokeCap(Paint.Cap.ROUND);
        }
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
        if(wireframe||model.triangles.length==0)drawEdges(canvas);
        else{drawSurfaces(canvas);drawEdges(canvas);}
        if(first>=0&&interactionMode!=InteractionMode.ORBIT){int i=first*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);}
        if(interactionMode==InteractionMode.EDIT&&first>=0)drawUcsGizmo(canvas);
        if(interactionMode==InteractionMode.MEASURE&&second>=0){int i=second*2;canvas.drawCircle(screen[i],screen[i+1],7f,marker);float value=distance(first,second);canvas.drawText(String.format(Locale.getDefault(),"3B mesafe: %.3f çizim birimi",value),18f,105f*getResources().getDisplayMetrics().density,caption);}
        else if(interactionMode==InteractionMode.EDIT&&first>=0)canvas.drawText("Seçili köşe: "+(first+1),18f,105f*getResources().getDisplayMetrics().density,caption);
    }
    private boolean editableVertex(int vertex){
        return vertex>=0&&vertex<model.sourceLines.length&&model.sourceLines[vertex]>=0;
    }
    private double[][] projectedAxes(){
        double[][] axes=new double[3][];
        for(int k=0;k<3;k++)axes[k]=CadGizmoMath.axisDirection(gizmoUcs,k,yaw,pitch);
        return axes;
    }
    private void drawUcsGizmo(Canvas canvas){
        if(!editableVertex(first))return;
        float density=getResources().getDisplayMetrics().density;
        float arrow=56f*density;
        double[][] dirs=projectedAxes();
        float px=screen[first*2],py=screen[first*2+1];
        for(int axis=0;axis<3;axis++){
            double[] d=dirs[axis];
            double len=Math.hypot(d[0],d[1]);
            if(!Double.isFinite(len)||len<.12d)continue; // viewed edge-on: do not invent axis movement
            float endX=px+(float)(arrow*d[0]/len),endY=py+(float)(arrow*d[1]/len);
            Paint paint=gizmoPaints[axis];
            canvas.drawLine(px,py,endX,endY,paint);
            canvas.drawCircle(endX,endY,7f*density,paint);
            canvas.drawText(new String[]{"X","Y","Z"}[axis],endX+8f*density,endY-8f*density,paint);
        }
    }
    private float pixelsPerWorldUnit(int vertex){
        int k=vertex*3;
        float co=(float)Math.cos(yaw),si=(float)Math.sin(yaw);
        float cp=(float)Math.cos(pitch),sp=(float)Math.sin(pitch);
        float x=model.xyz[k]-model.cx,y=model.xyz[k+1]-model.cy,z=model.xyz[k+2]-model.cz;
        float away=si*x+co*y,depth=sp*z+cp*away;
        float perspective=Math.max(.45f,Math.min(2.5f,1f/(1f+depth/(model.radius*2.5f))));
        return Math.min(getWidth(),getHeight())*zoom/(model.radius*2.6f)*perspective;
    }
    public void setUcsFrame(CadUcsFrame frame){
        if(frame==null)throw new IllegalArgumentException("UCS frame required");
        gizmoUcs=frame;postInvalidateOnAnimation();
    }
    public void setAxisDragListener(AxisDragListener listener){axisDragListener=listener;}
    public void setAxisDragCommitListener(AxisDragCommitListener listener){
        axisDragCommitListener=listener;
    }
    public boolean isAxisDragging(){return axisDragging;}
    /** Cancel an in-flight preview before a save picker or exit decision. */
    public boolean cancelActiveAxisDrag(){
        if(!axisDragging)return false;
        restoreCancelledDrag();
        return true;
    }
    private boolean startAxisDrag(float x,float y){
        if(interactionMode!=InteractionMode.EDIT||!editableVertex(first)||axisDragListener==null)return false;
        double[][] directions=projectedAxes();
        float density=getResources().getDisplayMetrics().density;
        int axis=CadGizmoMath.pick(x-screen[first*2],y-screen[first*2+1],
            directions,56f*density,18f*density);
        if(axis<0)return false;
        dragAxis=axis;dragVertex=first;dragX=x;dragY=y;
        int k=first*3;
        for(int c=0;c<3;c++)dragOriginal[c]=model.xyz[k+c];
        axisDragging=true;
        return true;
    }
    private void restoreCancelledDrag(){
        if(axisDragging&&axisDragListener!=null&&dragVertex>=0){
            axisDragListener.moveVertex(dragVertex,
                dragOriginal[0],dragOriginal[1],dragOriginal[2]);
        }
        axisDragging=false;dragAxis=dragVertex=-1;postInvalidateOnAnimation();
    }
    private void moveDraggedAxis(float x,float y){
        if(!axisDragging||axisDragListener==null)return;
        double[] d=CadGizmoMath.axisDirection(gizmoUcs,dragAxis,yaw,pitch);
        // Model units, not screen pixels. Account for foreshortening and current zoom.
        double distance=CadGizmoMath.drawingDistance(
            x-dragX,y-dragY,d[0],d[1],pixelsPerWorldUnit(dragVertex));
        if(!Double.isFinite(distance)||Math.abs(distance)>1e6d)return;
        double[] delta=gizmoUcs.move(dragAxis,distance);
        double nx=dragOriginal[0]+delta[0],ny=dragOriginal[1]+delta[1],
            nz=dragOriginal[2]+delta[2];
        if(!Double.isFinite(nx)||!Double.isFinite(ny)||!Double.isFinite(nz)||
           Math.abs(nx)>1e9d||Math.abs(ny)>1e9d||Math.abs(nz)>1e9d)return;
        if(axisDragListener.moveVertex(dragVertex,nx,ny,nz))
            postInvalidateOnAnimation();
    }
    private void drawEdges(Canvas canvas){
        int count=0;
        for(int i=0;i<model.edges.length;i+=2){
            int a=model.edges[i]*2,b=model.edges[i+1]*2;
            segments[count++]=screen[a];segments[count++]=screen[a+1];
            segments[count++]=screen[b];segments[count++]=screen[b+1];
        }
        if(count>0)canvas.drawLines(segments,0,count,lines);
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
    public int selectedVertex(){return interactionMode==InteractionMode.EDIT?first:-1;}
    public InteractionMode interactionMode(){return interactionMode;}
    public void setInteractionMode(InteractionMode mode){
        restoreCancelledDrag();
        interactionMode=mode==null?InteractionMode.ORBIT:mode;
        first=second=-1;postInvalidateOnAnimation();
    }
    public void geometryChanged(){postInvalidateOnAnimation();}
    public boolean toggleWireframe(){wireframe=!wireframe;postInvalidateOnAnimation();return wireframe;}
    public void setWireframe(boolean enabled){wireframe=enabled;postInvalidateOnAnimation();}
    public void setIsometricView(){yaw=.65f;pitch=.32f;zoom=1f;postInvalidateOnAnimation();}
    public void setFrontView(){yaw=0f;pitch=0f;zoom=1f;postInvalidateOnAnimation();}
    public void setTopView(){yaw=0f;pitch=-(float)Math.PI/2f;zoom=1f;postInvalidateOnAnimation();}
    public void setRightView(){yaw=(float)Math.PI/2f;pitch=0f;zoom=1f;postInvalidateOnAnimation();}
    public void resetCamera(){setIsometricView();}
    private int nearestVertex(float x,float y){int at=-1;float best=36f*getResources().getDisplayMetrics().density;best*=best;
        for(int i=0;i<screen.length;i+=2){float dx=screen[i]-x,dy=screen[i+1]-y,dist=dx*dx+dy*dy;if(dist<best){best=dist;at=i/2;}}
        return at;
    }
    private void selectMeasurement(float x,float y){int at=nearestVertex(x,y);if(at<0)return;if(first<0||second>=0){first=at;second=-1;}else second=at;postInvalidateOnAnimation();}
    private void selectEditVertex(float x,float y){
        int at=nearestVertex(x,y);if(at<0)return;
        first=at;second=-1;postInvalidateOnAnimation();
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        switch(e.getActionMasked()){
            case MotionEvent.ACTION_DOWN:
                lastX=e.getX();lastY=e.getY();moved=false;
                if(startAxisDrag(lastX,lastY))return true;
                return true;
            case MotionEvent.ACTION_POINTER_DOWN:
                if(axisDragging){restoreCancelledDrag();moved=true;}
                if(e.getPointerCount()>=2)pinchDistance=spacing(e);
                return true;
            case MotionEvent.ACTION_MOVE:
                if(axisDragging){
                    if(e.getPointerCount()!=1){restoreCancelledDrag();return true;}
                    moveDraggedAxis(e.getX(),e.getY());
                    moved=true;return true;
                }
                if(e.getPointerCount()>=2){
                    float next=spacing(e);
                    if(pinchDistance>4f)zoom=Math.max(.15f,Math.min(25f,zoom*next/pinchDistance));
                    pinchDistance=next;moved=true;postInvalidateOnAnimation();
                }else{
                    float dx=e.getX()-lastX,dy=e.getY()-lastY;
                    if(Math.abs(dx)+Math.abs(dy)>2f)moved=true;
                    yaw+=dx*.008f;pitch=Math.max(-1.5f,Math.min(1.5f,pitch+dy*.008f));
                    lastX=e.getX();lastY=e.getY();postInvalidateOnAnimation();
                }
                return true;
            case MotionEvent.ACTION_UP:
                if(axisDragging){
                    moveDraggedAxis(e.getX(),e.getY());
                    if(axisDragCommitListener!=null&&dragVertex>=0){
                        int k=dragVertex*3;
                        float[] after={model.xyz[k],model.xyz[k+1],model.xyz[k+2]};
                        axisDragCommitListener.finished(dragVertex,dragOriginal.clone(),after);
                    }
                    axisDragging=false;dragAxis=dragVertex=-1;
                    performClick();return true;
                }
                if(!moved){
                    if(interactionMode==InteractionMode.MEASURE)selectMeasurement(e.getX(),e.getY());
                    else if(interactionMode==InteractionMode.EDIT)selectEditVertex(e.getX(),e.getY());
                }
                performClick();return true;
            case MotionEvent.ACTION_CANCEL:
                restoreCancelledDrag();return true;
            default:return true;
        }
    }
    private static float spacing(MotionEvent e){float dx=e.getX(0)-e.getX(1),dy=e.getY(0)-e.getY(1);return (float)Math.hypot(dx,dy);}
    @Override public boolean performClick(){super.performClick();return true;}
}
