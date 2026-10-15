import com.musa.cad.Cad3dNativeMeshSurface;
import com.musa.cad.Cad3dSurfaceMetrics;

public final class Cad3dNativeMeshSurfaceTest {
    private static void need(boolean ok,String message){
        if(!ok)throw new AssertionError(message);
    }
    public static void main(String[] args){
        float[] triangle={0,0,0,3,0,0,0,4,0};
        int[] faces={0,1,2};
        Cad3dNativeMeshSurface.Comparison c=
            Cad3dNativeMeshSurface.compare(triangle,faces);
        need(c.javaResult.area==6d,"working Java area unchanged");
        need(!c.nativeAvailable&&!c.agreesWithJava,
             "host JVM does not fabricate Android JNI success");
        need("native_unavailable".equals(c.status),"missing JNI reported");
        need(Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,1,4d/3,0,1,0}),"matching numerical result");
        need(!Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,1,1.2,0,1,0}),"reject centroid drift");
        need(!Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,1,4d/3,0,2,0}),"reject missing faces");
        need(!Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,1,4d/3,0,1,1}),"reject fabricated degeneracy");
        need(!Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,Double.NaN,4d/3,0,1,0}),"reject NaN area center");
        need(!Cad3dNativeMeshSurface.agrees(c.javaResult,
            new double[]{6,1,4d/3,0,1}),"reject incomplete native payload");

        int[] zero={0,0,2};
        Cad3dNativeMeshSurface.Comparison deg=
            Cad3dNativeMeshSurface.compare(triangle,zero);
        need(deg.javaResult.area==0d&&deg.javaResult.degenerateCount==1,
            "Java zero-area behavior preserved");
        need(Cad3dNativeMeshSurface.agrees(deg.javaResult,
            new double[]{0,Double.NaN,Double.NaN,Double.NaN,1,1}),
            "zero area must have unavailable NaN centroid");
        need(!Cad3dNativeMeshSurface.agrees(deg.javaResult,
            new double[]{0,0,0,0,1,1}),"no fabricated origin centroid");
        Cad3dNativeMeshSurface.Comparison empty=
            Cad3dNativeMeshSurface.compare(new float[0],new int[0]);
        need(empty.javaResult.triangleCount==0 &&
             "empty_mesh_java_only".equals(empty.status),
             "empty geometry retains old safe semantics");
        float[] georeferenced={1000000,1000000,1000000,
                               1000003,1000000,1000000,
                               1000000,1000004,1000000};
        Cad3dSurfaceMetrics.Result world=
            Cad3dSurfaceMetrics.measure(georeferenced,faces);
        need(Cad3dNativeMeshSurface.agrees(world,
            new double[]{6,1000001,1000000+4d/3,1000000,1,0}),
            "WCS precision reference");
        try {
            Cad3dNativeMeshSurface.compare(triangle,new int[]{0,1,9});
            throw new AssertionError("invalid mesh silently accepted");
        }catch(IllegalArgumentException expected){}
        System.out.println("MusaCAD optional surface JNI Java fallback and parity guards PASS");
    }
}
