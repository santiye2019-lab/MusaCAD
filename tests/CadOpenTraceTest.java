import com.musa.cad.CadOpenTrace;

public final class CadOpenTraceTest {
    public static void main(String[]args){
        CadOpenTrace trace=new CadOpenTrace();
        trace.mark(CadOpenTrace.Phase.COPY_DONE);
        trace.mark(CadOpenTrace.Phase.NATIVE_OPEN);
        trace.mark(CadOpenTrace.Phase.NATIVE_SCENE_READY);
        trace.mark(CadOpenTrace.Phase.DXF_EXPORT);
        trace.mark(CadOpenTrace.Phase.FINAL_DXF_PARSE);
        trace.mark(CadOpenTrace.Phase.VIEW_HANDOFF);
        String summary=trace.safeSummary(16L*1024*1024,120000);
        if(!summary.contains("inputMiB=16.0")||!summary.contains("objects=120000")
            ||!summary.contains("FINAL_DXF_PARSEMs=")||!summary.contains("VIEW_HANDOFFMs=")
            ||!summary.contains("sampledHeapMiB="))
            throw new AssertionError(summary);
        if(summary.contains("path=")||summary.contains(".dwg")||summary.contains("filename"))
            throw new AssertionError("private drawing path in metrics");
        System.out.println("Native-to-DXF load stages / anonymized performance trace passed");
    }
}
