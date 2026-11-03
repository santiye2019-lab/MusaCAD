package com.musa.cad;

import java.util.concurrent.atomic.AtomicLong;

/** Host JVM regression for anonymous CAD-open timing metadata. */
public final class CadOpenTraceDiagnosticTest {
    private static void check(boolean condition,String why){
        if(!condition)throw new AssertionError(why);
    }
    public static void main(String[] args){
        AtomicLong ns=new AtomicLong(1000000000L);
        CadOpenTrace trace=new CadOpenTrace(ns::get);
        check(trace.totalMs()==0,"initial duration");
        check("UNKNOWN".equals(trace.latestStage()),"stage before first mark");
        ns.addAndGet(2_000_000L);
        trace.mark(CadOpenTrace.Phase.COPY_DONE);
        check("COPY".equals(trace.latestStage()),"copy stage");
        ns.addAndGet(3_000_000L);
        trace.mark(CadOpenTrace.Phase.NATIVE_OPEN);
        check("NATIVE_OPEN".equals(trace.latestStage()),"native stage");
        ns.addAndGet(4_000_000L);
        trace.mark(CadOpenTrace.Phase.NATIVE_SCENE_READY);
        check("NATIVE_SCENE".equals(trace.latestStage()),"scene stage");
        ns.addAndGet(5_000_000L);
        trace.mark(CadOpenTrace.Phase.DXF_EXPORT);
        check("DXF_EXPORT".equals(trace.latestStage()),"export stage");
        ns.addAndGet(7_000_000L);
        trace.mark(CadOpenTrace.Phase.FINAL_DXF_PARSE);
        check("DXF_PARSE".equals(trace.latestStage()),"parse stage");
        ns.addAndGet(8_000_000L);
        trace.mark(CadOpenTrace.Phase.VIEW_HANDOFF);
        check("VIEW_HANDOFF".equals(trace.latestStage()),"handoff stage");
        check(trace.totalMs()==29L,"total time in milliseconds");
        String safe=trace.safeSummary(4728533,42);
        check(safe.contains("objects=42"),"safe object count");
        check(!safe.contains("/storage/"),"no absolute paths");
        System.out.println("PASS: diagnostic trace stages and timings");
    }
}
