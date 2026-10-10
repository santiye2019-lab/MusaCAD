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
    // Retry only: preserve the same drawing coordinates/regions while lowering
    // transport size when Cloudflare or the phone drops a large visual request.
    private static final int COMPACT_TILE_SIDE=896;
    private static final int COMPACT_OVERVIEW_SIDE=512;
    private static final int COMPACT_BASE64_CHARS=1_000_000;
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

    /** One overview + three disjoint-in-index detailed tiles per batch. */
    public static Result renderBatch(DxfParser.Result drawing,int batch,Progress progress)throws Exception{
        return renderBatchInternal(drawing,batch,progress,false);
    }

    /** Single bounded retry payload; same nine-region coverage, less data. */
    public static Result renderBatchCompact(DxfParser.Result drawing,int batch,Progress progress)throws Exception{
        return renderBatchInternal(drawing,batch,progress,true);
    }

    private static Result renderBatchInternal(DxfParser.Result drawing,int batch,
                                              Progress progress,boolean compact)throws Exception{
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
            int side=i==0?(compact?COMPACT_OVERVIEW_SIDE:OVERVIEW_SIDE)
                :(compact?COMPACT_TILE_SIDE:TILE_SIDE);
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
                int quality=compact?(i==0?42:45):(i==0?48:56);
                if(!bitmap.compress(Bitmap.CompressFormat.JPEG,quality,out))break;
                jpg=out.toByteArray();
            }finally{if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
            if(jpg.length==0)break;
            String encoded=Base64.encodeToString(jpg,Base64.NO_WRAP);
            if(encoded.length()>MAX_SINGLE_BASE64_CHARS||
               totalChars+encoded.length()>(compact?COMPACT_BASE64_CHARS:MAX_BASE64_CHARS))break;
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
        payload.put("reducedResolution",compact);
        payload.put("rawDrawingIncluded",false);
        return new Result(payload,images.length(),regions.size(),renderedTiles,complete);
    }

    /**
     * Additional focused inspection around positioned view-title candidates.
     * The crop is a heuristic neighbourhood, never an authoritative frame
     * boundary: a nearby title may be outside its drawing frame.
     * Up to twelve candidate views (three batches) are bounded independently
     * of the whole-sheet 3x3 sweep.
     */
    public static final int MAX_FOCUSED_VIEWS=12;
    public static final int FOCUSED_BATCH_SIZE=4;

    public static Result renderViewsBatch(DxfParser.Result drawing,
                                          List<MusaAiViewCatalog.View> titles,int batch,
                                          Progress progress)throws Exception{
        if(drawing==null||titles==null)throw new IllegalArgumentException("Pafta görünüm listesi eksik");
        ArrayList<MusaAiViewCatalog.View> positioned=new ArrayList<>();
        for(MusaAiViewCatalog.View v:titles){
            if(v==null||!v.positioned())continue;
            positioned.add(v);
            if(positioned.size()>=MAX_FOCUSED_VIEWS)break;
        }
        int batchCount=(positioned.size()+FOCUSED_BATCH_SIZE-1)/FOCUSED_BATCH_SIZE;
        if(batch<0||batch>=batchCount)throw new IllegalArgumentException("Geçersiz görünüm grubu");
        RectF whole=drawing.printableContentBounds();
        if(whole==null||!finite(whole)||whole.width()<=1||whole.height()<=1)
            throw new IllegalArgumentException("Geçersiz DWG kapsamı");
        int from=batch*FOCUSED_BATCH_SIZE;
        int until=Math.min(positioned.size(),from+FOCUSED_BATCH_SIZE);
        ArrayList<RectF> crops=new ArrayList<>();
        crops.add(new RectF(whole));
        ArrayList<MusaAiViewCatalog.View> views=new ArrayList<>();
        double approxColumns=Math.max(1,Math.ceil(Math.sqrt(positioned.size())));
        float candidateW=(float)(whole.width()/approxColumns*1.4d);
        float candidateH=(float)(whole.height()/approxColumns*1.4d);
        candidateW=Math.max(whole.width()*0.20f,Math.min(whole.width(),candidateW));
        candidateH=Math.max(whole.height()*0.20f,Math.min(whole.height(),candidateH));
        for(int index=from;index<until;index++){
            MusaAiViewCatalog.View v=positioned.get(index);
            PointF point=drawing.contentPointFromDrawing((float)v.x,(float)v.y);
            if(point==null||!Float.isFinite(point.x)||!Float.isFinite(point.y)){
                // Keep positional slot: the server rejects missing indexed crops.
                break;
            }
            // Titles often sit beneath the view drawing. Offset the crop centre
            // slightly upwards, while avoiding off-page rectangles.
            float l=Math.max(whole.left,Math.min(whole.right-candidateW,
                point.x-candidateW*0.5f));
            float t=Math.max(whole.top,Math.min(whole.bottom-candidateH,
                point.y-candidateH*0.65f));
            crops.add(new RectF(l,t,l+candidateW,t+candidateH));
            views.add(v);
        }

        JSONArray images=new JSONArray();
        int chars=0,details=0;
        long started=android.os.SystemClock.elapsedRealtime();
        for(int idx=0;idx<crops.size();idx++){
            if(Thread.currentThread().isInterrupted()||
                android.os.SystemClock.elapsedRealtime()-started>MAX_RENDER_MS)break;
            RectF roi=crops.get(idx);
            int side=idx==0?OVERVIEW_SIDE:TILE_SIDE;
            Bitmap bitmap=null;byte[] jpg;
            if(progress!=null)progress.onProgress("Gandalf • Kat/kesit/vaziyet görüntüsü "+
                (batch+1)+"/"+batchCount+" • "+idx+"/"+views.size());
            try{
                bitmap=Bitmap.createBitmap(side,side,Bitmap.Config.RGB_565);
                Canvas canvas=new Canvas(bitmap);
                canvas.drawColor(Color.WHITE);
                Matrix map=new Matrix();
                if(!map.setRectToRect(roi,new RectF(8,8,side-8,side-8),Matrix.ScaleToFit.CENTER))break;
                int save=canvas.save();
                canvas.clipRect(0,0,side,side);
                drawing.drawVectorForPrint(canvas,map,false);
                canvas.restoreToCount(save);
                ByteArrayOutputStream buffer=new ByteArrayOutputStream(128000);
                if(!bitmap.compress(Bitmap.CompressFormat.JPEG,idx==0?48:56,buffer))break;
                jpg=buffer.toByteArray();
            }finally{if(bitmap!=null&&!bitmap.isRecycled())bitmap.recycle();}
            String encoded=Base64.encodeToString(jpg,Base64.NO_WRAP);
            if(encoded.length()>MAX_SINGLE_BASE64_CHARS||
                chars+encoded.length()>MAX_BASE64_CHARS)break;
            chars+=encoded.length();
            JSONObject image=new JSONObject();
            image.put("mime","image/jpeg");
            image.put("base64",encoded);
            image.put("label",idx==0?"full-sheet-overview":"view-focus-"+(from+idx));
            image.put("width",side);
            image.put("height",side);
            image.put("contentBounds",bounds(roi.left,roi.top,roi.right,roi.bottom));
            PointF a=drawing.drawingPointFromContent(roi.left,roi.top);
            PointF b=drawing.drawingPointFromContent(roi.right,roi.bottom);
            image.put("drawingBounds",bounds(Math.min(a.x,b.x),Math.min(a.y,b.y),
                Math.max(a.x,b.x),Math.max(a.y,b.y)));
            if(idx>0){
                MusaAiViewCatalog.View v=views.get(idx-1);
                image.put("viewTitle",v.title);
                image.put("viewKind",v.kind.name().toLowerCase(java.util.Locale.ROOT));
                image.put("viewSourceId",v.sourceId);
                image.put("candidateCrop",true);
                details++;
            }
            images.put(image);
        }
        JSONObject out=new JSONObject();
        out.put("schema","musacad-visual-evidence/v1");
        out.put("viewSchema","musacad-visual-views/v1");
        out.put("viewBatch",batch+1);
        out.put("viewBatchCount",batchCount);
        out.put("totalCandidateViews",positioned.size());
        out.put("firstView",from+1);
        out.put("lastView",until);
        out.put("images",images);
        out.put("rawDrawingIncluded",false);
        out.put("candidateCropsNotVerifiedViewFrames",true);
        boolean complete=details==until-from&&images.length()==crops.size();
        out.put("complete",complete);
        return new Result(out,images.length(),crops.size(),details,complete);
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
