import com.musa.cad.Cad3dMeshTopology;
import java.lang.reflect.Field;
/** Runs the SAME prior mesh topology regression through actual native JNI. */
public final class Cad3dMeshTopologyNativeHostTest {
    private static void need(boolean ok,String message){
        if(!ok)throw new AssertionError(message);
    }
    public static void main(String[] args)throws Exception{
        System.setProperty("java.vm.name","Dalvik");
        Field flag=Cad3dMeshTopology.class.getDeclaredField("ANDROID_RUNTIME");
        flag.setAccessible(true);
        need(flag.getBoolean(null),"Native topology path was bypassed");
        Cad3dMeshTopologyTest.main(args);
        Cad3dMeshTopology.Result empty=Cad3dMeshTopology.inspect(new float[0],new int[0]);
        need(empty.checked&&!empty.closedOrientedByIndex&&empty.triangles==0,
                "empty document must not be an indexed closed solid");
        float[] xyz={0,0,0,1,0,0,0,1,0,0,0,1};
        Cad3dMeshTopology.Result tetra=Cad3dMeshTopology.inspect(xyz,
                new int[]{0,2,1,0,1,3,0,3,2,1,2,3});
        need(tetra.closedOrientedByIndex&&tetra.boundaryEdges==0&&
             tetra.duplicateFaces==0,"closed tetra topology");
        System.out.println("PASS: actual Android JNI mesh topology and JVM oracle cases");
    }
}
