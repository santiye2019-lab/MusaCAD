package com.musa.cad;

import java.util.Arrays;
import java.util.List;

/**
 * Immutable, metadata-only viewport candidate index for a completely
 * discovered DWG page catalog. No actual geometry is retained here.
 *
 * Keep four sorted projections of each page's occupied X/Y interval. For
 * a viewport query, scan the SMALLEST of the four guaranteed supersets:
 * minX <= right, maxX >= left, minY <= bottom, maxY >= top.
 * Every truly intersecting page belongs to all four supersets. Follow up
 * with exact 2D intersection and normal center-distance/LRU logic.
 *
 * Sorted once, on the page decoder worker after the terminal page arrives;
 * user pinch/zoom never sorts/rebuilds this data.
 */
public final class NativePageSpatialIndex {
    public static final class Span {
        public final NativePageCatalog.Entry[] entries;
        public final int from,to;
        Span(NativePageCatalog.Entry[] entries,int from,int to){
            this.entries=entries;this.from=from;this.to=to;
        }
        public int size(){return to-from;}
    }

    private final NativePageCatalog.Entry[] minX,maxX,minY,maxY;

    public NativePageSpatialIndex(List<NativePageCatalog.Entry> entries){
        if(entries==null)throw new IllegalArgumentException("missing page catalog");
        NativePageCatalog.Entry[] base=entries.toArray(new NativePageCatalog.Entry[0]);
        minX=base.clone();maxX=base.clone();minY=base.clone();maxY=base.clone();
        Arrays.sort(minX,(a,b)->Float.compare(a.minX,b.minX));
        Arrays.sort(maxX,(a,b)->Float.compare(a.maxX,b.maxX));
        Arrays.sort(minY,(a,b)->Float.compare(a.minY,b.minY));
        Arrays.sort(maxY,(a,b)->Float.compare(a.maxY,b.maxY));
    }

    public int size(){return minX.length;}

    private static float key(NativePageCatalog.Entry entry,int axis){
        switch(axis){
            case 0:return entry.minX;
            case 1:return entry.maxX;
            case 2:return entry.minY;
            default:return entry.maxY;
        }
    }

    /** First entry with key >= value. */
    private static int lowerBound(NativePageCatalog.Entry[] entries,float value,int axis){
        int l=0,r=entries.length;
        while(l<r){
            int m=l+(r-l)/2;
            if(Float.compare(key(entries[m],axis),value)<0)l=m+1;
            else r=m;
        }
        return l;
    }

    /** First entry with key > value. */
    private static int upperBound(NativePageCatalog.Entry[] entries,float value,int axis){
        int l=0,r=entries.length;
        while(l<r){
            int m=l+(r-l)/2;
            if(Float.compare(key(entries[m],axis),value)<=0)l=m+1;
            else r=m;
        }
        return l;
    }

    public Span candidates(float left,float top,float right,float bottom){
        if(!Float.isFinite(left)||!Float.isFinite(top)||
           !Float.isFinite(right)||!Float.isFinite(bottom)||
           right<left||bottom<top)
            throw new IllegalArgumentException("invalid viewport");
        int minXCandidates=upperBound(minX,right,0);
        int maxXFirst=lowerBound(maxX,left,1);
        int minYCandidates=upperBound(minY,bottom,2);
        int maxYFirst=lowerBound(maxY,top,3);
        int n=minX.length;
        Span best=new Span(minX,0,minXCandidates);
        if(n-maxXFirst<best.size())best=new Span(maxX,maxXFirst,n);
        if(minYCandidates<best.size())best=new Span(minY,0,minYCandidates);
        if(n-maxYFirst<best.size())best=new Span(maxY,maxYFirst,n);
        return best;
    }
}
