package com.musa.cad;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Base64;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Consent-gated, bounded visual render of the LOADED vector DWG sheet.
 * A complete sweep renders 3x3 overlapping, high-resolution tiles in up to
 * three independent batches. The caller must actually send and receive all
 * batches before reporting complete coverage. Never uploads raw DWG bytes.
 */
public final class MusaAiVisualEvidence {
    public interface Progress {void onProgress(String text);}

    private static final int TILE_SIDE=1200;
    private static final int OVERVIEW_SIDE=640;
    private static final int MAX_TILES=5;
    private static final int MAX_BASE64_CHARS=1_650_000;
    private static final long MAX_RENDER_MS=25000L;
    private static final int MAX_SINGLE_BASE64_CHARS=900000;

    public static final class Result {
        public final JSONObject json;
        public final int imageCount,regionCount,renderedTiles;
        public final boolean complete;
        Result(JSONObject json,int images,int regions,int renderedTiles,boolean complete){
            this.json=json;this.imageCount=images;this.regionCount=regions;
            this.renderedTiles=renderedTiles;this.complete=complete;
        }
    }

    /** Backward-compatible bounded single batch. NOT a complete visual sweep. */
    public static Result render(DxfParser.Result drawing,Progress progress)throws Exception{
        return renderBatch(drawing,0,progress);
    }

    /** One overview + up to four disjoint-in-index detailed tiles per batch. */
    public static Result renderBatch(DxfParser.Result drawing,int batch,Progress progress)throws Exception{
        if(drawing==null)throw new IllegalArgumentException("Vektör çizim hazır değil");
        if(batch<0||batch>=MusaAiVisualSweepPlan.BATCH_COUNT)
            throw new IllegalArgumentException("Geçersiz pafta tarama grubu");
        RectF full=drawing.printableContentBounds();
        if(full==null||!finite(full)||full.width()<=1||full.height()<=1)
            throw new IllegalStateException("Pafta sınırları geçersiz");

        List<RectF> regions=new ArrayList<>(MAX_TILES);
        List<String> names=new ArrayList<>(MAX_TILES);
        regions.add(new RectF(full));names.add("full-sheet-overview");
        List<MusaAiVisualSweepPlan.Tile> tiles=MusaAiVisualSweepPlan.tiles();
        int begin=MusaAiVisualSweepPlan.firstTile(batch);
        int end=MusaAiVisualSweepPlan.lastExclusive(batch);
        for(int i=begin;i<end;i++){
            MusaAiVisualSweepPlan.Tile t=tiles.get(i);
            regions.add(new RectF(
                (float)(full.left+full.width()*t.left),
                (float)(full.top+full.height()*t.top),
                (float)(full.left+full.width()*t.right),
                (float)(full.top+full.height()*t.bottom)));
            names.add(t.label());
        }

        JSONArray images=new JSONArray();
        int totalChars=0,renderedTiles=0;
        long start=android.os.SystemClock.elapsedRealtime();
        for(int i=0;i<Math.min(regions.size(),MAX_TILES);i++){
            if(Thread.currentThread().isInterrupted())break;
            if(android.os.SystemClock.elapsedRealtime()-start>MAX_RENDER_MS)break;
            if(progress!=null)progress.onProgress("Gandalf • Görsel pafta taranıyor: grup "+
                (batch+1)+"/"+MusaAiVisualSweepPlan.BATCH_COUNT+", bölge "+i+"/"+(regions.size()-1));
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
                    break;
                int saved=canvas.save();
                canvas.clipRect(0,0,side,side);
                drawing.drawVectorForPrint(canvas,fromContent,false);
                canvas.restoreToCount(saved);
                ByteArrayOutputStream out=new ByteArrayOutputStream(128000);
                if(!bitmap.compress(Bitmap.CompressFormat.JPEG,i==0?48:56,out))break;
                jpg=out.toByteArray();
            }finally{if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
            if(jpg.length==0)break;
            String encoded=Base64.encodeToString(jpg,Base64.NO_WRAP);
            if(encoded.length()>MAX_SINGLE_BASE64_CHARS||
               totalChars+encoded.length()>MAX_BASE64_CHARS)break;
            totalChars+=encoded.length();
            JSONObject one=new JSONObject();
            one.put("mime","image/jpeg");
            one.put("base64",encoded);
            one.put("label",names.get(i));
            one.put("width",side);
            one.put("height",side);
            one.put("contentBounds",bounds(roi.left,roi.top,roi.right,roi.bottom));
            PointF worldA=drawing.drawingPointFromContent(roi.left,roi.top);
            PointF worldB=drawing.drawingPointFromContent(roi.right,roi.bottom);
            if(worldA==null||worldB==null||
               !Float.isFinite(worldA.x)||!Float.isFinite(worldA.y)||
               !Float.isFinite(worldB.x)||!Float.isFinite(worldB.y))break;
            one.put("drawingBounds",bounds(Math.min(worldA.x,worldB.x),
                Math.min(worldA.y,worldB.y),Math.max(worldA.x,worldB.x),
                Math.max(worldA.y,worldB.y)));
            images.put(one);
            if(i>0)renderedTiles++;
        }

        JSONObject payload=new JSONObject();
        payload.put("schema","musacad-visual-evidence/v1");
        payload.put("coordinateSpace","musacad-drawing-content");
        payload.put("drawingCoordinateSpace","dwg-world");
        payload.put("sweepSchema","musacad-visual-sweep/v1");
        payload.put("sweepBatch",batch+1);
        payload.put("sweepBatchCount",MusaAiVisualSweepPlan.BATCH_COUNT);
        payload.put("totalDetailedTiles",MusaAiVisualSweepPlan.TILE_COUNT);
        payload.put("firstTile",begin+1);
        payload.put("lastTile",end);
        payload.put("images",images);
        payload.put("renderedRegionCount",images.length());
        payload.put("requestedRegionCount",regions.size());
        payload.put("fullSheetIncluded",images.length()>0);
        boolean complete=renderedTiles==end-begin&&images.length()==regions.size();
        payload.put("complete",complete);
        payload.put("rawDrawingIncluded",false);
        return new Result(payload,images.length(),regions.size(),renderedTiles,complete);
    }

    private static JSONArray bounds(double a,double b,double c,double d) throws org.json.JSONException{
        JSONArray out=new JSONArray();out.put(a).put(b).put(c).put(d);return out;
    }
    private static boolean finite(RectF r){
        return Float.isFinite(r.left)&&Float.isFinite(r.top)&&
            Float.isFinite(r.right)&&Float.isFinite(r.bottom);
    }
    private MusaAiVisualEvidence(){}
}
