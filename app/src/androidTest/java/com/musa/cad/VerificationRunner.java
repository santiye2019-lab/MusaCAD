package com.musa.cad;

import android.app.Instrumentation;
import android.graphics.*;
import android.os.*;
import android.view.MotionEvent;
import java.io.*;
import java.util.*;

/** Runs real Android Canvas, Matrix, touch, calibration and JNI paths. */
public class VerificationRunner extends Instrumentation {
    private final StringBuilder report=new StringBuilder();
    private File evidence;
    @Override public void onCreate(Bundle args){super.onCreate(args);start();}
    private static void require(boolean ok,String reason){if(!ok)throw new AssertionError(reason);}
    private void passed(String name){report.append("PASS ").append(name).append('\n');}
    @Override public void onStart(){
        Bundle result=new Bundle();
        try{
            evidence=new File(getTargetContext().getFilesDir(),"verification");evidence.mkdirs();
            verifyNative();
            Throwable[] failure={null};
            runOnMainSync(()->{try{verifyGraphics();verifyMeasurements();}catch(Throwable t){failure[0]=t;}});
            if(failure[0]!=null)throw failure[0];
            report.append("MUSACAD_VERIFIED\n");
            try(FileOutputStream out=new FileOutputStream(new File(evidence,"results.txt"))){out.write(report.toString().getBytes("UTF-8"));}
            result.putString("stream",report.toString());finish(-1,result);
        }catch(Throwable t){
            StringWriter trace=new StringWriter();t.printStackTrace(new PrintWriter(trace));
            result.putString("stream",report+"FAIL\n"+trace);finish(0,result);
        }
    }
    private void png(Bitmap bitmap,String name)throws IOException {
        try(FileOutputStream out=new FileOutputStream(new File(evidence,name))){require(bitmap.compress(Bitmap.CompressFormat.PNG,100,out),"PNG failed");}
    }
    private void verifyNative()throws Exception {
        File input=new File(getTargetContext().getCacheDir(),"reference.dwg");
        try(InputStream in=getContext().getAssets().open("reference.dwg");OutputStream out=new FileOutputStream(input)){
            byte[] b=new byte[65536];int n;while((n=in.read(b))>=0)out.write(b,0,n);
        }
        long before=input.length();DxfParser.Result parsed=NativeDwg.read(input,getTargetContext().getCacheDir());
        require(parsed.entityCount>0,"JNI produced no geometry");require(input.length()==before,"Original changed");
        for(File f:getTargetContext().getCacheDir().listFiles())require(!f.getName().startsWith("MusaCAD_donusen_"),"Converted cache not removed");
        png(parsed.bitmap,"native-reference.png");
        passed("Native DWG conversion/render: "+parsed.entityCount+" entities, warnings="+parsed.conversionWarnings);
        parsed.bitmap.recycle();input.delete();
    }
    private static boolean ink(Bitmap b,int x,int y){
        int background=Color.rgb(18,24,30);
        for(int dy=-3;dy<=3;dy++)for(int dx=-3;dx<=3;dx++)if(b.getPixel(x+dx,y+dy)!=background)return true;
        return false;
    }
    private void verifyGraphics()throws Exception {
        String dxf="0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n8\nFRAME\n70\n1\n10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"+
            "0\nLWPOLYLINE\n8\nARC\n70\n0\n10\n0\n20\n0\n42\n-1\n10\n10\n20\n0\n0\nENDSEC\n0\nEOF\n";
        File f=new File(getTargetContext().getCacheDir(),"geometry.dxf");
        try(FileOutputStream out=new FileOutputStream(f)){out.write(dxf.getBytes("UTF-8"));}
        DxfParser.Result all=DxfParser.render(f);
        require(all.entityCount==2&&all.layerCount==2,"Scene count");
        require(ink(all.bitmap,80,1200)&&ink(all.bitmap,1200,80),"Square edges missing");
        png(all.bitmap,"square-and-arc.png");
        DxfParser.Result arc=all.withVisibleLayers(Collections.singleton("ARC"));
        require(arc.entityCount==1,"Layer count");
        require(ink(arc.bitmap,1200,1200),"Arc apex missing or direction inverted");
        require(!ink(arc.bitmap,1200,2320),"Arc rendered as straight chord");
        require(!ink(arc.bitmap,80,1200),"Hidden layer still visible");
        require(arc.snapPoints.length==4,"Control points included in snapping");
        require(arc.snapIndex.nearest(81,2319,1,8)>=0,"Endpoint snap missing");
        png(arc.bitmap,"arc-only.png");
        passed("Android Canvas: square edges, semicircle direction, layer hiding, endpoint-only snapping");
        all.bitmap.recycle();arc.bitmap.recycle();f.delete();
    }
    private static void tap(CadView view,float x,float y){
        long now=SystemClock.uptimeMillis();
        MotionEvent down=MotionEvent.obtain(now,now,MotionEvent.ACTION_DOWN,x,y,0);
        MotionEvent up=MotionEvent.obtain(now,now+20,MotionEvent.ACTION_UP,x,y,0);
        view.onTouchEvent(down);view.onTouchEvent(up);down.recycle();up.recycle();
    }
    private static void pinchEvent(CadView v,long start,int dt,int action,float left,float right,int count){
        MotionEvent.PointerProperties[] props=new MotionEvent.PointerProperties[count];
        MotionEvent.PointerCoords[] coords=new MotionEvent.PointerCoords[count];
        for(int i=0;i<count;i++){
            props[i]=new MotionEvent.PointerProperties();props[i].id=i;props[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
            coords[i]=new MotionEvent.PointerCoords();coords[i].x=i==0?left:right;coords[i].y=300;coords[i].pressure=1;coords[i].size=1;
        }
        MotionEvent e=MotionEvent.obtain(start,start+dt,action,count,props,coords,0,0,1,1,0,0,android.view.InputDevice.SOURCE_TOUCHSCREEN,0);
        v.onTouchEvent(e);e.recycle();
    }
    private void verifyMeasurements()throws Exception {
        Locale old=Locale.getDefault();Locale.setDefault(Locale.US);
        try{
            CadView v=new CadView(getTargetContext(),null);v.layout(0,0,1000,1000);
            Bitmap source=Bitmap.createBitmap(1000,1000,Bitmap.Config.ARGB_8888);source.eraseColor(Color.rgb(18,24,30));v.setDrawing(source);
            String[] reading={""};v.setListener(new CadView.Listener(){
                public void onMeasurement(String text){reading[0]=text;}
                public void onCalibrationRequested(double pixels){}
                public void onSelectionReady(){}
            });
            v.setMode(CadView.Mode.CALIBRATE);tap(v,100,100);tap(v,600,100);v.setCalibration(10,"m");
            v.setMode(CadView.Mode.DISTANCE);tap(v,100,100);tap(v,250,300);
            require(reading[0].equals("Mesafe: 5.000 m"),"Distance: "+reading[0]);
            passed("Touch + calibration: 3-4-5 triangle gives 5.000 m");
            v.setMode(CadView.Mode.AREA);tap(v,100,100);tap(v,600,100);tap(v,600,600);tap(v,100,600);
            require(reading[0].equals("Alan: 100.000 m²"),"Area: "+reading[0]);
            png(v.snapshot(),"area-100m2.png");passed("Touch area: calibrated 10m square gives 100.000 m2");
            v.undo();require(reading[0].equals("Alan: 50.000 m²"),"Undo area: "+reading[0]);
            v.clearMeasurement();v.setMode(CadView.Mode.PAN);
            long now=SystemClock.uptimeMillis();
            MotionEvent down=MotionEvent.obtain(now,now,MotionEvent.ACTION_DOWN,200,200,0);
            MotionEvent move=MotionEvent.obtain(now,now+20,MotionEvent.ACTION_MOVE,250,250,0);
            MotionEvent up=MotionEvent.obtain(now,now+40,MotionEvent.ACTION_UP,250,250,0);
            v.onTouchEvent(down);v.onTouchEvent(move);v.onTouchEvent(up);down.recycle();move.recycle();up.recycle();
            v.setMode(CadView.Mode.DISTANCE);tap(v,150,150);tap(v,300,350);
            require(reading[0].equals("Mesafe: 5.000 m"),"Pan changed calibration: "+reading[0]);
            passed("Undo and pan preserve measurement semantics");
            v.setMode(CadView.Mode.PAN);long pinch=SystemClock.uptimeMillis();
            pinchEvent(v,pinch,0,MotionEvent.ACTION_DOWN,200,400,1);
            pinchEvent(v,pinch,20,MotionEvent.ACTION_POINTER_DOWN|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),200,400,2);
            pinchEvent(v,pinch,40,MotionEvent.ACTION_MOVE,150,450,2);
            pinchEvent(v,pinch,60,MotionEvent.ACTION_MOVE,100,500,2);
            pinchEvent(v,pinch,80,MotionEvent.ACTION_POINTER_UP|(1<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),100,500,2);
            pinchEvent(v,pinch,100,MotionEvent.ACTION_UP,100,500,1);
            v.setMode(CadView.Mode.DISTANCE);tap(v,200,200);tap(v,500,600);
            require(reading[0].equals("Mesafe: 5.000 m"),"Pinch changed measurement: "+reading[0]);
            passed("Two-finger 2x zoom preserves calibrated distance");
            source.recycle();
        }finally{Locale.setDefault(old);}
    }
}
