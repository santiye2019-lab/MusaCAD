package com.musa.cad;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Bounded viewport-aware cache for immutable native scene pages.
 *
 * Pure Java so cache correctness can be proven without Android framework.
 * Payloads are produced off the UI thread. This does NOT fetch missing DWG
 * pages automatically and is not a substitute for the authoritative DXF.
 *
 * Pages use a two-part source-order cursor. An evicted/missing page breaks
 * completeness even if later pages are still cached. A viewport request
 * returns only pages whose conservative content bounds intersect the view.
 */
public final class NativeViewportPageCache<T> {
    public static final class Cursor implements Comparable<Cursor> {
        public final long top, leaf;
        public Cursor(long top,long leaf) {
            if(top<0||leaf<0)throw new IllegalArgumentException("negative cursor");
            this.top=top;this.leaf=leaf;
        }
        @Override public int compareTo(Cursor other) {
            int cmp=Long.compare(top,other.top);
            return cmp!=0?cmp:Long.compare(leaf,other.leaf);
        }
        @Override public boolean equals(Object other) {
            if(!(other instanceof Cursor))return false;
            Cursor c=(Cursor)other;return top==c.top&&leaf==c.leaf;
        }
        @Override public int hashCode() {
            return 31*Long.hashCode(top)+Long.hashCode(leaf);
        }
    }

    public static final class Page<T> {
        public final Cursor start,next;
        public final boolean hasMore;
        public final float minX,minY,maxX,maxY;
        public final long byteCost;
        public final T payload;
        private Page(Cursor from,Cursor to,boolean more,
                     float minX,float minY,float maxX,float maxY,
                     long bytes,T value) {
            this.start=from;this.next=to;this.hasMore=more;
            this.minX=minX;this.minY=minY;this.maxX=maxX;this.maxY=maxY;
            this.byteCost=bytes;this.payload=value;
        }
    }

    private final long maxBytes;
    private final int maxPages;
    /* LRU-by-access; use stable cursor sorting when rendering page content. */
    private final LinkedHashMap<Cursor,Page<T>> pages=
            new LinkedHashMap<>(16,.75f,true);
    /* Read-only membership lookup; checking completeness must not alter LRU. */
    private final HashMap<Cursor,Page<T>> index=new HashMap<>();
    private long heldBytes;
    private boolean everEvicted;

    public NativeViewportPageCache(long maxBytes,int maxPages) {
        if(maxBytes<=0||maxPages<=0)
            throw new IllegalArgumentException("invalid cache budget");
        this.maxBytes=maxBytes;this.maxPages=maxPages;
    }

    public synchronized boolean put(Cursor start,Cursor next,boolean more,
            float minX,float minY,float maxX,float maxY,long bytes,T payload) {
        if(start==null||next==null||start.compareTo(next)>=0||
                !Float.isFinite(minX)||!Float.isFinite(minY)||
                !Float.isFinite(maxX)||!Float.isFinite(maxY)||
                maxX<minX||maxY<minY||bytes<=0||payload==null)
            throw new IllegalArgumentException("invalid scene page or cursor");
        if(bytes>maxBytes)return false; // cannot ever fit: fail, never clear other pages
        Page<T> newPage=new Page<>(start,next,more,minX,minY,maxX,maxY,bytes,payload);
        Page<T> old=pages.remove(start);
        index.remove(start);
        if(old!=null)heldBytes-=old.byteCost;
        pages.put(start,newPage);
        index.put(start,newPage);
        heldBytes+=bytes;
        while(heldBytes>maxBytes||pages.size()>maxPages){
            Map.Entry<Cursor,Page<T>> oldest=pages.entrySet().iterator().next();
            heldBytes-=oldest.getValue().byteCost;
            pages.remove(oldest.getKey());
            index.remove(oldest.getKey());
            everEvicted=true;
        }
        return true;
    }

    public synchronized ArrayList<Page<T>> visible(float minX,float minY,
                         float maxX,float maxY) {
        if(!Float.isFinite(minX)||!Float.isFinite(minY)||
           !Float.isFinite(maxX)||!Float.isFinite(maxY)||
           maxX<minX||maxY<minY)
            throw new IllegalArgumentException("invalid viewport");
        ArrayList<Page<T>> result=new ArrayList<>();
        /* Iterate without cache.get(): offscreen pages must not become MRU. */
        for(Map.Entry<Cursor,Page<T>> entry:pages.entrySet()){
            Page<T> p=entry.getValue();
            if(p.maxX>=minX&&p.maxY>=minY&&
               p.minX<=maxX&&p.minY<=maxY)result.add(p);
        }
        /* Touch only pages that the caller actually intends to draw. */
        for(Page<T> page:result)pages.get(page.start);
        result.sort((a,b)->a.start.compareTo(b.start));
        return result;
    }

    /**
     * True only while an unbroken 0:0 -> terminal page chain is retained.
     * An empty or partially evicted cache must never claim full coverage.
     */
    public synchronized boolean isComplete(){
        Cursor cursor=new Cursor(0,0);
        HashSet<Cursor> visited=new HashSet<>();
        while(visited.add(cursor)){
            Page<T> p=index.get(cursor);
            if(p==null)return false;
            if(!p.hasMore)return true;
            cursor=p.next;
        }
        return false;
    }

    /** Lookup without marking an offscreen page as recently displayed. */
    public synchronized boolean contains(Cursor start){
        return start!=null&&index.containsKey(start);
    }

    public synchronized int pageCount(){return pages.size();}
    public synchronized long retainedBytes(){return heldBytes;}
    public synchronized boolean hasEvictedPages(){return everEvicted;}
    public synchronized void clear(){pages.clear();index.clear();heldBytes=0;everEvicted=false;}
}
