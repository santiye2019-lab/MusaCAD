package com.musa.cad;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import java.util.ArrayList;
import java.util.HashMap;
import java.lang.ref.WeakReference;

/**
 * Opt-in bounded preview compositor. NOT connected to production CadView yet.
 *
 * Loading is strictly off the UI thread via NativeCadEngine.previewNestedPage();
 * draw() is called only from the Android UI renderer. Its native scene pages
 * contain immutable geometry, but each NativeScene has Canvas scratch objects,
 * therefore NEVER draw one page simultaneously on two threads.
 *
 * When a page is evicted the compositor reports isComplete()==false; a caller
 * must fetch the missing pages or show a partial-loading indicator. It must not
 * pretend that missing geometry is outside the viewport.
 */
public final class NativePagedScenePreview {
    private final NativeViewportPageCache<NativeScene> cache;
    private final NativePageCatalog catalog=new NativePageCatalog(100000);
    // Stores only 32-byte digests, not evicted page geometry. Re-decoded
    // records must match their original words, including geometry and color.
    private final HashMap<NativeViewportPageCache.Cursor,byte[]> pageHashes=
            new HashMap<>();
    private Matrix worldToContent;
    private final Matrix scratchCombined=new Matrix();
    private final Matrix scratchInverse=new Matrix();
    private final RectF scratchVisible=new RectF();
    private boolean sourceStarted;
    /* Weak source identity guard: must not pin an evicted first page in RAM. */
    private WeakReference<NativeScene> firstPageRef;

    public NativePagedScenePreview(long maxRetainedBytes,int maxRetainedPages){
        cache=new NativeViewportPageCache<>(maxRetainedBytes,maxRetainedPages);
    }

    /** Adds one page generated from the full two-part source cursor. */
    public synchronized boolean retain(long startTop,long startLeaf,
                                          NativeCadEngine.NestedScenePage page){
        if(page==null||page.scene==null||page.scene.truncated)
            throw new IllegalArgumentException("incomplete or missing CAD page");
        if(!sourceStarted){
            if(startTop!=0||startLeaf!=0)
                throw new IllegalArgumentException("first page must start at 0:0");
        }
        RectF bbox=page.scene.occupiedWorldBounds();
        NativeViewportPageCache.Cursor start=
                new NativeViewportPageCache.Cursor(startTop,startLeaf);
        NativeViewportPageCache.Cursor next=
                new NativeViewportPageCache.Cursor(page.nextTopLevelIndex,
                                                  page.nextNestedLeafIndex);
        // Validate cursor/bounds/cap BEFORE cache.put can evict an older page.
        // A malformed continuation must leave all previous pages untouched.
        catalog.requireAppend(start,next,page.hasMore,bbox.left,bbox.top,
                              bbox.right,bbox.bottom,page.scene.primitiveCount);
        // Hash on the loader thread before cache eviction; a rejected page
        // must not displace any other valid cached geometry.
        byte[] digest=page.scene.contentSha256();
        boolean stored=cache.put(start,next,page.hasMore,
                bbox.left,bbox.top,bbox.right,bbox.bottom,
                page.scene.estimatedRetainedBytes(),page.scene);
        if(stored){
            catalog.append(start,next,page.hasMore,bbox.left,bbox.top,
                           bbox.right,bbox.bottom,page.scene.primitiveCount);
            pageHashes.put(start,digest);
        }
        if(stored&&!sourceStarted){
            worldToContent=page.scene.worldToContentTransform();
            firstPageRef=new WeakReference<>(page.scene);
            sourceStarted=true;
        }
        return stored;
    }

    /** Metadata remains available after a payload is evicted by the LRU. */
    public synchronized boolean isCatalogComplete(){return catalog.isDiscovered();}

    /** A bounded list of source cursors to fetch for a particular visible area. */
    public synchronized ArrayList<NativePageCatalog.Entry> missingViewportPages(
            float minX,float minY,float maxX,float maxY,int maxBatch){
        return new ArrayList<>(catalog.missingVisible(
                cache,minX,minY,maxX,maxY,maxBatch));
    }

    /**
     * Restores ONLY a previously discovered page. Rejects any read that
     * produces a changed cursor, geometry count or bounding box. This cannot
     * expand the catalog or silently accept a changed source DWG.
     */
    public synchronized boolean retainReload(NativePageCatalog.Entry expected,
                                             NativeCadEngine.NestedScenePage page){
        if(expected==null||page==null||page.scene==null||page.scene.truncated)
            throw new IllegalArgumentException("invalid reloaded DWG page");
        RectF bbox=page.scene.occupiedWorldBounds();
        NativeViewportPageCache.Cursor next=
                new NativeViewportPageCache.Cursor(page.nextTopLevelIndex,
                                                   page.nextNestedLeafIndex);
        if(!catalog.samePage(expected.start,next,page.hasMore,bbox.left,bbox.top,
                             bbox.right,bbox.bottom,page.scene.primitiveCount))
            throw new IllegalStateException("DWG sayfası yeniden okumada değişti");
        // Cursor, count, and bounds alone cannot detect a changed line,
        // color, handle/order or text inside the SAME bounding rectangle.
        // Reject before cache.put to avoid replacing/evicting valid pages.
        if(!NativeSceneFingerprint.matches(
                pageHashes.get(expected.start),page.scene.contentSha256()))
            throw new IllegalStateException("DWG sayfası içerik SHA-256 uyuşmuyor");
        return cache.put(expected.start,next,page.hasMore,
                         bbox.left,bbox.top,bbox.right,bbox.bottom,
                         page.scene.estimatedRetainedBytes(),page.scene);
    }

    /** The view must hold the exact first scene object from this source. */
    public synchronized boolean acceptsFirstScene(NativeScene active) {
        return active!=null&&firstPageRef!=null&&firstPageRef.get()==active;
    }

    public synchronized boolean isComplete(){return cache.isComplete();}
    public synchronized int pageCount(){return cache.pageCount();}
    public synchronized long retainedEstimateBytes(){return cache.retainedBytes();}
    public synchronized boolean isPartial(){return !cache.isComplete();}
    public synchronized void clear(){
        cache.clear();catalog.clear();pageHashes.clear();
        sourceStarted=false;worldToContent=null;firstPageRef=null;
    }

    /**
     * Renders only pages whose conservative geometry bounds intersect the
     * viewport. Every selected NativeScene also uses its own existing spatial
     * grid to cull individual primitives. No work to decode a DWG page here.
     *
     * Returns count of pages drawn, not a claim that the DWG was complete.
     */
    /**
     * Inverse-transform the current Canvas clip to a conservative DWG world
     * viewport; this same rectangle drives both culling and page reloads.
     * It is a copied value, never a mutable shared scratch RectF.
     */
    public synchronized RectF visibleWorldBounds(Canvas canvas,Matrix contentToScreen){
        if(canvas==null||contentToScreen==null||worldToContent==null)return null;
        scratchCombined.setConcat(contentToScreen,worldToContent);
        if(!scratchCombined.invert(scratchInverse))return null;
        Rect clip=canvas.getClipBounds();
        if(clip==null||clip.isEmpty())return null;
        scratchVisible.set(clip);
        scratchInverse.mapRect(scratchVisible);
        float worldPad=Math.max(scratchVisible.width()/Math.max(1,clip.width()),
                                scratchVisible.height()/Math.max(1,clip.height()))*12f;
        if(Float.isFinite(worldPad)&&worldPad>0f)
            scratchVisible.inset(-worldPad,-worldPad);
        return new RectF(scratchVisible);
    }

    public synchronized int draw(Canvas canvas,Matrix contentToScreen,
                                 boolean fastNavigation){
        RectF viewport=visibleWorldBounds(canvas,contentToScreen);
        if(viewport==null)return 0;
        ArrayList<NativeViewportPageCache.Page<NativeScene>> visible=
                cache.visible(viewport.left,viewport.top,
                              viewport.right,viewport.bottom);
        for(NativeViewportPageCache.Page<NativeScene> page:visible)
            page.payload.draw(canvas,contentToScreen,fastNavigation);
        return visible.size();
    }
}
