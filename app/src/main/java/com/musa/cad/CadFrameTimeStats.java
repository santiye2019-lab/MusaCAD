package com.musa.cad;

import java.util.Arrays;

/**
 * In-memory, opt-in UI drawing CPU-time samples. No files, identifiers,
 * telemetry network, frame-size guesses or per-sample allocations.
 *
 * This is time spent in CadView.onDraw, NOT SurfaceFlinger frame pacing,
 * display refresh latency, GPU time, or original-DWG visual fidelity.
 */
public final class CadFrameTimeStats {
    private static final int CAPACITY=256;
    private static final long SLOW_NS=16_666_667L;
    private final long[] moving=new long[CAPACITY];
    private final long[] settled=new long[CAPACITY];
    private int movingCount,movingCursor,settledCount,settledCursor;

    public static final class Group {
        public final int samples,overBudget;
        public final double p50Millis,p95Millis,maxMillis;
        private Group(int samples,int overBudget,double p50,double p95,double max){
            this.samples=samples;this.overBudget=overBudget;
            p50Millis=p50;p95Millis=p95;maxMillis=max;
        }
    }
    public static final class Snapshot {
        public final Group moving,settled;
        private Snapshot(Group moving,Group settled){
            this.moving=moving;this.settled=settled;
        }
    }

    public synchronized void sample(long elapsedNanos,boolean gestureActive){
        if(elapsedNanos<0)throw new IllegalArgumentException("negative draw duration");
        if(gestureActive){
            moving[movingCursor]=elapsedNanos;
            movingCursor=(movingCursor+1)%CAPACITY;
            if(movingCount<CAPACITY)movingCount++;
        }else{
            settled[settledCursor]=elapsedNanos;
            settledCursor=(settledCursor+1)%CAPACITY;
            if(settledCount<CAPACITY)settledCount++;
        }
    }

    private static Group summarize(long[] ring,int count){
        if(count==0)return new Group(0,0,0d,0d,0d);
        // A copy and sort happen ONLY when the caller explicitly asks for
        // a snapshot, never on CadView's pinch/pan UI hot path.
        long[] ordered=Arrays.copyOf(ring,count);
        Arrays.sort(ordered);
        int slow=0;
        for(long elapsed:ordered)if(elapsed>SLOW_NS)slow++;
        return new Group(count,slow,
                ordered[(int)Math.ceil(count*.50)-1]/1_000_000d,
                ordered[(int)Math.ceil(count*.95)-1]/1_000_000d,
                ordered[count-1]/1_000_000d);
    }

    public synchronized Snapshot snapshot(){
        return new Snapshot(summarize(moving,movingCount),
                            summarize(settled,settledCount));
    }
}
