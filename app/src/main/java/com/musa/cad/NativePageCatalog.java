package com.musa.cad;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Comparator;
import java.util.PriorityQueue;

/**
 * Metadata-only catalog of sequential native DWG scene pages.
 *
 * The geometry can be evicted from NativeViewportPageCache while the small
 * cursor/bounds metadata stays available to request that SAME page again.
 * No payload, DWG source bytes, file paths, images or private coordinates are
 * uploaded or persisted. An explicit page limit bounds catalog memory.
 */
public final class NativePageCatalog {
    public static final class Entry {
        public final NativeViewportPageCache.Cursor start,next;
        public final boolean hasMore;
        public final float minX,minY,maxX,maxY;
        public final int primitives;
        private Entry(NativeViewportPageCache.Cursor start,
                NativeViewportPageCache.Cursor next,boolean more,
                float minX,float minY,float maxX,float maxY,int primitives){
            this.start=start;this.next=next;this.hasMore=more;
            this.minX=minX;this.minY=minY;this.maxX=maxX;this.maxY=maxY;
            this.primitives=primitives;
        }
        public boolean intersects(float x0,float y0,float x1,float y1){
            return maxX>=x0 && maxY>=y0 && minX<=x1 && minY<=y1;
        }
    }

    /* While a large DWG is still decoding, the sorted viewportIndex cannot
     * be finalized. Scanning all discovered pages at every pinch frame can
     * cause UI stalls. Instead group 128 sequential pages under conservative
     * occupied-world bounding boxes, maintained once on the decoder thread.
     * Skipping a non-intersecting group can NEVER skip visible geometry.
     * This is metadata only; no payloads or DWG bytes are duplicated. */
    private static final int PARTIAL_GROUP_SIZE=128;
    private static final class PartialGroup {
        final int from;
        int to;
        float minX,minY,maxX,maxY;
        PartialGroup(int from,Entry first){
            this.from=from;
            this.to=from+1;
            minX=first.minX;minY=first.minY;
            maxX=first.maxX;maxY=first.maxY;
        }
        void include(Entry entry){
            to++;
            minX=Math.min(minX,entry.minX);
            minY=Math.min(minY,entry.minY);
            maxX=Math.max(maxX,entry.maxX);
            maxY=Math.max(maxY,entry.maxY);
        }
        boolean intersects(float x0,float y0,float x1,float y1){
            return maxX>=x0&&maxY>=y0&&minX<=x1&&minY<=y1;
        }
    }
    private final ArrayList<PartialGroup> partialGroups=new ArrayList<>();

    private final int maxEntries;
    private final ArrayList<Entry> ordered=new ArrayList<>();
    private final HashMap<NativeViewportPageCache.Cursor,Entry> byStart=new HashMap<>();
    private NativeViewportPageCache.Cursor expected=
            new NativeViewportPageCache.Cursor(0,0);
    private boolean discoveredAll;
    // Built once on the decoder worker when terminal discovery finishes.
    private NativePageSpatialIndex viewportIndex;

    public NativePageCatalog(int maxEntries){
        if(maxEntries<1||maxEntries>100000)
            throw new IllegalArgumentException("invalid maximum page entries");
        this.maxEntries=maxEntries;
    }
    private static void checkBounds(float minX,float minY,float maxX,float maxY){
        if(!Float.isFinite(minX)||!Float.isFinite(minY)||
           !Float.isFinite(maxX)||!Float.isFinite(maxY)||
           maxX<minX||maxY<minY)throw new IllegalArgumentException("invalid page bounds");
    }

    /**
     * Append ONLY the next sequential page. Duplicate, gap and branched
     * cursors fail closed. The first terminal page finalizes discovery.
     */
    /**
     * Validate a prospective next page BEFORE updating any geometry cache.
     * Fail-closed input must not evict a valid page before catalog rejection.
     * Does not alter discovery state.
     */
    public synchronized void requireAppend(NativeViewportPageCache.Cursor start,
            NativeViewportPageCache.Cursor next,boolean more,
            float minX,float minY,float maxX,float maxY,int primitives){
        checkBounds(minX,minY,maxX,maxY);
        if(discoveredAll||ordered.size()>=maxEntries||start==null||next==null||
           !expected.equals(start)||next.compareTo(start)<=0||primitives<1)
            throw new IllegalArgumentException("invalid DWG catalog continuation");
    }

    public synchronized void append(NativeViewportPageCache.Cursor start,
            NativeViewportPageCache.Cursor next,boolean more,
            float minX,float minY,float maxX,float maxY,int primitives){
        requireAppend(start,next,more,minX,minY,maxX,maxY,primitives);
        Entry e=new Entry(start,next,more,minX,minY,maxX,maxY,primitives);
        ordered.add(e);byStart.put(start,e);
        int pageIndex=ordered.size()-1;
        if(pageIndex%PARTIAL_GROUP_SIZE==0)
            partialGroups.add(new PartialGroup(pageIndex,e));
        else
            partialGroups.get(partialGroups.size()-1).include(e);
        expected=next;
        if(!more){
            discoveredAll=true;
            // Do the one-time index construction while loading, never on
            // the first pinch-zoom request from the Android UI thread.
            if(ordered.size()>=128)
                viewportIndex=new NativePageSpatialIndex(ordered);
        }
    }

    public synchronized Entry get(NativeViewportPageCache.Cursor start){
        return byStart.get(start);
    }
    public synchronized boolean isDiscovered(){return discoveredAll;}
    public synchronized int size(){return ordered.size();}

    /**
     * Missing pages intersecting the requested viewport. Geometry is absent
     * from the active cache, so exactly these cursor starts can be re-decoded.
     * Sort by distance to viewport center, tie-break by source order.
     */
    public synchronized List<Entry> missingVisible(
            NativeViewportPageCache<?> cache,
            float minX,float minY,float maxX,float maxY,int maxResults){
        if(cache==null||maxResults<1||maxResults>256)
            throw new IllegalArgumentException("invalid missing-page request");
        checkBounds(minX,minY,maxX,maxY);
        final double cx=((double)minX+maxX)*.5,cy=((double)minY+maxY)*.5;
        Comparator<Entry> nearestFirst=(a,b)->{
            double ax=((double)a.minX+a.maxX)*.5-cx;
            double ay=((double)a.minY+a.maxY)*.5-cy;
            double bx=((double)b.minX+b.maxX)*.5-cx;
            double by=((double)b.minY+b.maxY)*.5-cy;
            int cmp=Double.compare(ax*ax+ay*ay,bx*bx+by*by);
            return cmp==0?a.start.compareTo(b.start):cmp;
        };
        // Keep only the closest maxResults pages. Sorting every missing page
        // could allocate/sort 100,000 entries on each pinch/zoom redraw.
        // The max-heap's head is the WORST retained candidate.
        PriorityQueue<Entry> closest=new PriorityQueue<>(
                maxResults,nearestFirst.reversed());
        if(viewportIndex!=null){
            NativePageSpatialIndex.Span candidates=
                    viewportIndex.candidates(minX,minY,maxX,maxY);
            for(int i=candidates.from;i<candidates.to;i++){
                Entry e=candidates.entries[i];
                if(!e.intersects(minX,minY,maxX,maxY)||cache.contains(e.start))
                    continue;
                if(closest.size()<maxResults)closest.add(e);
                else if(nearestFirst.compare(e,closest.peek())<0){
                    closest.remove();closest.add(e);
                }
            }
        }else{
            // The catalog may contain tens of thousands of discovered pages
            // before reaching EOF. Conservative incremental group envelopes
            // skip whole offscreen ranges without rebuilding a sorted index
            // during every Android pinch/zoom. Exact page checks are retained.
            for(PartialGroup group:partialGroups){
                if(!group.intersects(minX,minY,maxX,maxY))continue;
                for(int i=group.from;i<group.to;i++){
                    Entry page=ordered.get(i);
                    if(!page.intersects(minX,minY,maxX,maxY)||
                       cache.contains(page.start))continue;
                    if(closest.size()<maxResults)closest.add(page);
                    else if(nearestFirst.compare(page,closest.peek())<0){
                        closest.remove();closest.add(page);
                    }
                }
            }
        }
        ArrayList<Entry> misses=new ArrayList<>(closest);
        misses.sort(nearestFirst);
        return misses;
    }

    /**
     * An eviction reload must return the same next cursor, termination state,
     * primitive count and page bounds as its first decode. No new branch may
     * be silently accepted as if it were the original DWG stream.
     */
    public synchronized boolean samePage(NativeViewportPageCache.Cursor start,
            NativeViewportPageCache.Cursor next,boolean more,
            float minX,float minY,float maxX,float maxY,int primitives){
        Entry e=byStart.get(start);
        return e!=null&&e.next.equals(next)&&e.hasMore==more&&
                e.primitives==primitives&&
                Float.compare(e.minX,minX)==0&&Float.compare(e.minY,minY)==0&&
                Float.compare(e.maxX,maxX)==0&&Float.compare(e.maxY,maxY)==0;
    }

    public synchronized void clear(){
        ordered.clear();byStart.clear();partialGroups.clear();
        viewportIndex=null;
        expected=new NativeViewportPageCache.Cursor(0,0);
        discoveredAll=false;
    }
}
