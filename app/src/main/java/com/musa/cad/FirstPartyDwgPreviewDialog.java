package com.musa.cad;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * EXPLICIT read-only diagnostic for authentic AC1032/R2018 2D geometry.
 * Never changes MainActivity's editor, native session or project file.
 * Missing entities, layers, visibility and unresolved BYLAYER/BYBLOCK are
 * NOT upgraded to a complete drawing. Source is opened READ ONLY by JNI.
 */
public final class FirstPartyDwgPreviewDialog {
    private static final int MAX_FLOAT_WORDS=2*1024*1024;
    private FirstPartyDwgPreviewDialog(){}

    public static void show(Activity owner,File original){
        show(owner,original,false);
    }

    /** Optional temporary cache ownership for the standalone SAF picker.
     * The source copy is deleted AFTER JNI has finished its read-only scan;
     * an already-open project source is never removed. */
    public static void show(Activity owner,File original,boolean temporaryCopy){
        if(owner==null||original==null||!original.isFile()){
            if(temporaryCopy&&original!=null)original.delete();
            return;
        }
        final AtomicBoolean dismissed=new AtomicBoolean(false);
        final LinearLayout layout=new LinearLayout(owner);
        layout.setOrientation(LinearLayout.VERTICAL);
        final int pad=Math.round(12f*owner.getResources().getDisplayMetrics().density);
        layout.setPadding(pad,pad,pad,pad);
        final TextView warning=new TextView(owner);
        warning.setText("DENEYSEL • EKSİK DWG GEOMETRİSİ / RENKLERİ OLABİLİR. "
            +"Bu görünüm yalnız kaynağı okur; düzenleme, kaydetme ve "
            +"tam dosya açma yerine geçmez.");
        warning.setTextColor(0xffb71c1c);
        layout.addView(warning,new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.WRAP_CONTENT));

        final TextView status=new TextView(owner);
        status.setPadding(0,pad/2,0,pad/2);
        status.setText("Gerçek AC1032 kaynağı doğrulanıyor…");
        layout.addView(status,new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.WRAP_CONTENT));

        final SceneCanvas sceneCanvas=new SceneCanvas(owner);
        int height=Math.round(380f*owner.getResources().getDisplayMetrics().density);
        layout.addView(sceneCanvas,new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,height));

        final TextView help=new TextView(owner);
        help.setText("Tek parmak: kaydır  •  İki parmak: yakınlaştır  •  Çift dokun: sığdır");
        help.setPadding(0,pad/2,0,0);
        layout.addView(help,new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,LinearLayout.LayoutParams.WRAP_CONTENT));

        final AlertDialog dialog=new AlertDialog.Builder(owner)
            .setTitle("MusaCAD • Bağımsız 2B DWG Test Görünümü")
            .setView(layout)
            .setPositiveButton("KAPAT",null)
            .create();
        dialog.setOnDismissListener(ignored->dismissed.set(true));
        dialog.show();
        final File source=original;
        Thread task=new Thread(()->{
            try{
                /* JNI checks original AC1032, all cached handle/CRC records,
                 * strict capacity and source-preserving read-only semantics.
                 * Do NOT run this potentially costly parse on the UI thread. */
                FirstPartyDwgPreview.Result preview=
                    FirstPartyDwgPreview.read(source,MAX_FLOAT_WORDS);
                owner.runOnUiThread(()->{
                    if(dismissed.get()||owner.isFinishing()||owner.isDestroyed())return;
                    sceneCanvas.setScene(preview.scene);
                    FirstPartyDwgPreview.Readiness a=preview.readiness;
                    status.setText("Gerçek DWG • "+preview.scene.primitiveCount+
                        " çizilen öğe • "+
                        " Kaynak kayıt: "+a.indexed+
                        " (tanınan 2B: "+a.planarRecognized+
                        ", katman+renk kaynağı doğrulanan: "+a.sourceStyleLinked+
                        ", çözülmeyen: "+a.sourceStyleUnresolved+
                        ", diğer kayıt: "+a.otherRecords+")"+
                        "\nDoğrudan renk: "+a.directAci+
                        " • BYLAYER: "+a.byLayer+
                        " • BYBLOCK: "+a.byBlock+
                        " • karmaşık/görünmez: "+a.complex+
                        " • sınırlı katman renk kanıtı: "+a.narrowLayerAciEvidence+
                        "\nKISMİ GÖRÜNÜM • Katman açık/kapalı ve gerçek"+
                        " son renkler doğrulanmadı; kayıt sayısı çizim öğesi"+
                        " sayısı değildir.");
                });
            }catch(IOException | RuntimeException ex){
                final String message=ex.getMessage()==null?
                    "Bilinmeyen DWG doğrulama hatası":ex.getMessage();
                owner.runOnUiThread(()->{
                    if(dismissed.get()||owner.isFinishing()||owner.isDestroyed())return;
                    status.setText("Bağımsız DWG önizleme açılamadı: "+message+
                                   " • Ana çizim ve kaynak dosya değiştirilmedi.");
                });
            }finally{
                /* Only the temporary cache copy is owned by this diagnostic.
                 * Never delete a caller-owned project/original DWG. */
                if(temporaryCopy)source.delete();
            }
        },"MusaCAD-OriginalDWG-2D-Diagnostic");
        task.setDaemon(true);
        task.start();
    }

    /** View-only graphics; all drawing coordinates are computed in C11. */
    private static final class SceneCanvas extends View {
        private NativeScene scene;
        private final Matrix contentToScreen=new Matrix();
        private float zoom=1f,downX,downY;
        private final ScaleGestureDetector scaler;
        private final GestureDetector gestures;
        SceneCanvas(Activity owner){
            super(owner);
            setBackgroundColor(CadRenderPalette.SCREEN_BACKGROUND);
            scaler=new ScaleGestureDetector(owner,new ScaleGestureDetector.SimpleOnScaleGestureListener(){
                @Override public boolean onScale(ScaleGestureDetector detector){
                    float desired=zoom*detector.getScaleFactor();
                    if(!Float.isFinite(desired)||desired<0.8f||desired>48f)return false;
                    float step=desired/zoom;
                    zoom=desired;
                    contentToScreen.postScale(step,step,
                                             detector.getFocusX(),detector.getFocusY());
                    invalidate();
                    return true;
                }
            });
            gestures=new GestureDetector(owner,new GestureDetector.SimpleOnGestureListener(){
                @Override public boolean onDown(MotionEvent e){return true;}
                @Override public boolean onDoubleTap(MotionEvent e){
                    fit();
                    return true;
                }
            });
        }
        void setScene(NativeScene next){scene=next;fit();}
        private void fit(){
            if(getWidth()<=0||getHeight()<=0)return;
            zoom=1f;
            float scale=Math.min(getWidth()/2400f,getHeight()/2400f);
            contentToScreen.reset();
            contentToScreen.postScale(scale,scale);
            contentToScreen.postTranslate(
                (getWidth()-2400f*scale)/2f,
                (getHeight()-2400f*scale)/2f);
            invalidate();
        }
        @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){
            super.onSizeChanged(w,h,oldw,oldh);
            fit();
        }
        @Override protected void onDraw(Canvas canvas){
            super.onDraw(canvas);
            if(scene!=null)scene.draw(canvas,contentToScreen);
        }
        @Override public boolean onTouchEvent(MotionEvent event){
            scaler.onTouchEvent(event);
            gestures.onTouchEvent(event);
            switch(event.getActionMasked()){
                case MotionEvent.ACTION_DOWN:
                    downX=event.getX();downY=event.getY();
                    return true;
                case MotionEvent.ACTION_MOVE:
                    if(event.getPointerCount()==1&&!scaler.isInProgress()){
                        float dx=event.getX()-downX,dy=event.getY()-downY;
                        if(Float.isFinite(dx)&&Float.isFinite(dy)){
                            contentToScreen.postTranslate(dx,dy);
                            invalidate();
                        }
                    }
                    downX=event.getX();downY=event.getY();
                    return true;
                case MotionEvent.ACTION_POINTER_UP:
                    downX=event.getX();downY=event.getY();
                    return true;
                case MotionEvent.ACTION_UP:
                    performClick();
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    return true;
                default:return true;
            }
        }
        @Override public boolean performClick(){super.performClick();return true;}
    }
}
