import com.musa.cad.Cad3dSurfaceMetrics;
/** Production Android must reject absent first-party native, never Java fallback. */
public final class Cad3dSurfaceMetricsMissingCoreTest{
    public static void main(String[] args){
        System.setProperty("java.vm.name","Dalvik");
        boolean rejected=false;
        try {
            Cad3dSurfaceMetrics.measure(new float[]{0,0,0,1,0,0,0,1,0},
                                        new int[]{0,1,2});
        }catch(LinkageError expected){rejected=true;}
        if(!rejected)throw new AssertionError("Missing C engine silently fell back to Java");
        System.out.println("PASS: CAD native mesh surface missing-library safe rejection");
    }
}
