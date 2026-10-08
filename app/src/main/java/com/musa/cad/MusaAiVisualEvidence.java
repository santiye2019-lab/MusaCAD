package com.musa.cad;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.util.Base64;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Explicitly consented, bounded visual projection of the *loaded* vector CAD
 * sheet. No raw DWG is uploaded. Every tile carries drawing-content bounds
 * so visual observations can be compared with source CAD coordinates.
 *
 * Not OCR or object recognition; that is done by the online multimodal model.
 */
public final class MusaAiVisualEvidence {
    public interface Progress {void onProgress(String text);}

    private static final int TILE_SIDE=960;
    private static final int OVERVIEW_SIDE=800;
    private static final int MAX_TILES=5;
    private static final int MAX_BASE64_CHARS=1_650_000;
    private static final long MAX_RENDER_MS=20000L;

    public static final class Result {
        public final JSONObject json;
        public final int imageCount;
        public final int regionCount;
        public final boolean complete;
        Result(JSONObject json,int images,int regions,boolean complete){
            this.json=json;this.imageCount=images;this.regionCount=regions;this.complete=complete;
        }
    }

    public static Result render(DxfParser.Result drawing,Progress progress)throws Exception{
        if(drawing==null)throw new IllegalArgumentException("Vektör çizim henüz hazır değil");
        RectF full=drawing.printableContentBounds();
        if(full==null||!finite(full)||full.width()<=1||full.height()<=1)
            throw new IllegalStateException("Pafta sınırları geçersiz");
        List<RectF> regions=new ArrayList<>();
        regions.add(new RectF(full));
        float midX=full.centerX(),midY=full.centerY();
        final float overlap=.02f;
        float mx=full.width()*overlap,my=full.height()*overlap;
        regions.add(new RectF(full.left,full.top,midX+mx,midY+my));
        regions.add(new RectF(midX-mx,full.top,full.right,midY+my));
        regions.add(new RectF(full.left,midY-my,midX+mx,full.bottom));
        regions.add(new RectF(midX-mx,midY-my,full.right,full.bottom));
        JSONArray images=new JSONArray();
        int totalChars=0;
        long start=android.os.SystemClock.elapsedRealtime();
        for(int i=0;i<Math.min(regions.size(),MAX_TILES);i++){
            if(Thread.currentThread().isInterrupted())break;
            if(android.os.SystemClock.elapsedRealtime()-start>MAX_RENDER_MS)break;
            if(progress!=null)progress.onProgress("Gandalf • Pafta görseli hazırlanıyor ("+(i+1)+"/"+regions.size()+")…");
            RectF roi=regions.get(i);
            int side=i==0?OVERVIEW_SIDE:TILE_SIDE;
            Bitmap bitmap=null;
            byte[] jpg;
            try{
                bitmap=Bitmap.createBitmap(side,side,Bitmap.Config.RGB_565);
                Canvas canvas=new Canvas(bitmap);
                canvas.drawColor(Color.WHITE);
                Matrix fromContent=new Matrix();
                if(!fromContent.setRectToRect(roi,
                    new RectF(8f,8f,side-8f,side-8f),Matrix.ScaleToFit.CENTER))
                    continue;
                int save=canvas.save();
                canvas.clipRect(0,0,side,side);
                drawing.drawVectorForPrint(canvas,fromContent,false);
                canvas.restoreToCount(save);
                ByteArrayOutputStream output=new ByteArrayOutputStream(100000);
                if(!bitmap.compress(Bitmap.CompressFormat.JPEG,i==0?61:68,output))continue;
                jpg=output.toByteArray();
            }finally{if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
            if(jpg.length<=0)continue;
            String encoded=Base64.encodeToString(jpg,Base64.NO_WRAP);
            if(totalChars+encoded.length()>MAX_BASE64_CHARS)break;
            totalChars+=encoded.length();
            JSONObject one=new JSONObject();
            one.put("mime","image/jpeg");
            one.put("base64",encoded);
            one.put("label",i==0?"full-sheet-overview":"sheet-quadrant-"+i);
            one.put("width",side);
            one.put("height",side);
            JSONArray contentBounds=new JSONArray();
            contentBounds.put(roi.left).put(roi.top).put(roi.right).put(roi.bottom);
            one.put("contentBounds",contentBounds);
            images.put(one);
        }
        JSONObject payload=new JSONObject();
        payload.put("schema","musacad-visual-evidence/v1");
        payload.put("coordinateSpace","musacad-drawing-content");
        payload.put("images",images);
        payload.put("renderedRegionCount",images.length());
        payload.put("requestedRegionCount",regions.size());
        payload.put("fullSheetIncluded",images.length()>0);
        payload.put("complete",images.length()==regions.size());
        payload.put("rawDrawingIncluded",false);
        return new Result(payload,images.length(),regions.size(),images.length()==regions.size());
    }

    private static boolean finite(RectF r){
        return Float.isFinite(r.left)&&Float.isFinite(r.top)&&
            Float.isFinite(r.right)&&Float.isFinite(r.bottom);
    }
    private MusaAiVisualEvidence(){}
}
