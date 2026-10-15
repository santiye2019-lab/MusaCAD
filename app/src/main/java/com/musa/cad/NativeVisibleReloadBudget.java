package com.musa.cad;

import java.util.HashSet;

/**
 * Upper bound for reload work from a single visible DWG viewport request.
 *
 * Tracks attempted two-part cursors, including pages evicted during this same
 * request. If an oversized viewport cannot fit in the cache, the loader must
 * stop instead of re-decoding the same pages forever. The preview stays visibly
 * partial until the viewport changes or the cache budget can satisfy it.
 *
 * Pure Java: Android-free unit tests exercise the paging policy.
 */
public final class NativeVisibleReloadBudget {
    private final int maxAttempts;
    private final HashSet<NativeViewportPageCache.Cursor> attempted=new HashSet<>();

    public NativeVisibleReloadBudget(int maxAttempts){
        if(maxAttempts<1||maxAttempts>256)
            throw new IllegalArgumentException("invalid reload attempt cap");
        this.maxAttempts=maxAttempts;
    }

    /** Reserve a cursor before decoding. False means duplicate or exhausted. */
    public boolean reserve(NativeViewportPageCache.Cursor start){
        if(start==null||attempted.size()>=maxAttempts)return false;
        return attempted.add(start);
    }

    public int attemptedCount(){return attempted.size();}
}
