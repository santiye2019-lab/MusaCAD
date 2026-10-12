package com.musa.cad;

import java.util.Locale;
import java.util.function.LongSupplier;

/**
 * Per-file, in-memory load timings. CAD text, filenames, paths and coordinates
 * are NEVER recorded. A mark measures the elapsed time up to the indicated
 * stage, not the first hardware-displayed frame.
 */
public final class CadOpenTrace {
    public enum Phase { COPY_DONE, NATIVE_OPEN, NATIVE_SCENE_READY, DXF_EXPORT, FINAL_DXF_PARSE, VIEW_HANDOFF }
    private final LongSupplier clock;
    private final long started;
    private final long[] elapsedMs=new long[Phase.values().length];
    private long last;
    private long maxHeapBytes;
    private final boolean[] seen=new boolean[Phase.values().length];

    public CadOpenTrace(){this(System::nanoTime);}
    CadOpenTrace(LongSupplier clock){
        this.clock=clock;
        started=clock.getAsLong();
        last=started;
        sampleHeap();
    }

    public synchronized void mark(Phase phase){
        long now=clock.getAsLong();
        int i=phase.ordinal();
        if(!seen[i]){
            elapsedMs[i]=Math.max(0,(now-started)/1_000_000L);
            seen[i]=true;
        }
        last=Math.max(last,now);
        sampleHeap();
    }

    private void sampleHeap(){
        Runtime runtime=Runtime.getRuntime();
        maxHeapBytes=Math.max(maxHeapBytes,runtime.totalMemory()-runtime.freeMemory());
    }

    public synchronized String safeSummary(long inputBytes,int parsedEntities){
        StringBuilder s=new StringBuilder("inputMiB=");
        s.append(String.format(Locale.ROOT,"%.1f",Math.max(0L,inputBytes)/1048576d));
        s.append(" objects=").append(Math.max(0,parsedEntities));
        for(Phase p:Phase.values())if(seen[p.ordinal()]){
            s.append(' ').append(p.name()).append("Ms=").append(elapsedMs[p.ordinal()]);
        }
        s.append(" sampledHeapMiB=")
         .append(String.format(Locale.ROOT,"%.1f",maxHeapBytes/1048576d));
        return s.toString();
    }
}
