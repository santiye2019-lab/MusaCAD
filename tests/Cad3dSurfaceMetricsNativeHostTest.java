import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Cad3dNativeMeshSurface;
import java.lang.reflect.Field;
/** Force Android VM detection before class loading; exercise actual native JNI. */
public final class Cad3dSurfaceMetricsNativeHostTest {
    private static void need(boolean yes,String why){if(!yes)throw new AssertionError(why);}
    public static void main(String[] args)throws Exception{
        System.setProperty("java.vm.name","Dalvik");
        Field active=Cad3dSurfaceMetrics.class.getDeclaredField("ANDROID_RUNTIME");
        active.setAccessible(true);
        need(active.getBoolean(null),"Native CAD surface math bypassed");
        Cad3dSurfaceMetricsTest.main(args);
        Cad3dNativeMeshSurface.Comparison c=Cad3dNativeMeshSurface.compare(
            new float[]{0,0,0,3,0,0,0,4,0},new int[]{0,1,2});
        need(c.javaResult.area==6 && !c.nativeAvailable &&
            !c.agreesWithJava &&
            "native_authoritative_no_independent_oracle".equals(c.status),
            "Native vs native MUST NOT be reported as independent parity");
        System.out.println("PASS: true JNI surface area, empty, huge WCS and no false equivalence");
    }
}
