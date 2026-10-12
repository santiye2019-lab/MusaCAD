package com.musa.cad;

import java.util.Arrays;
import java.util.Locale;

/**
 * Rolling CPU time of Canvas.onDraw only, not vsync/frame presentation time.
 * update: O(1), no per-frame allocation; report: copy+sort <=128 samples.
 */
public final class CadFrameDrawTimes {
    private static final int CAPACITY=128;
    private final long[] nanos=new long[CAPACITY];
    private int written;
    public synchronized void clear(){written=0;}
    public synchronized void record(long elapsedNanos){
        if(elapsedNanos<0||elapsedNanos>60_000_000_000L)return;
        nanos[written%CAPACITY]=elapsedNanos;
        written++;
    }
    public synchronized String safeSummary(){
        int n=Math.min(written,CAPACITY);
        if(n==0)return "Henüz çizim örneği yok";
        long[] ordered=Arrays.copyOf(nanos,n);
        Arrays.sort(ordered);
        double p50=ordered[(int)Math.ceil(n*.5d)-1]/1e6d;
        double p95=ordered[(int)Math.ceil(n*.95d)-1]/1e6d;
        int above=0;
        for(long v:ordered)if(v>16_700_000L)above++;
        return String.format(Locale.ROOT,
            "Canvas çizim CPU: %d kare, p50=%.1f ms, p95=%.1f ms, 16.7ms üstü=%d%% (GPU/vsync dahil değil)",
            n,p50,p95,Math.round(100d*above/n));
    }
}
