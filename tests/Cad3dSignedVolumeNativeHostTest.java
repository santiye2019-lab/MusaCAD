import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dNativeMeshMass;
import java.lang.reflect.Field;
/** Force real native Android pathway on the JVM test host. */
public final class Cad3dSignedVolumeNativeHostTest {
    private static void require(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[] args)throws Exception {
        System.setProperty("java.vm.name","Dalvik");
        Field active=Cad3dSignedVolume.class.getDeclaredField("ANDROID_RUNTIME");
        active.setAccessible(true);
        require(active.getBoolean(null),"Signed mass JNI was bypassed");
        Cad3dSignedVolumeTest.main(args);
        float[] tetra={0,0,0,1,0,0,0,1,0,0,0,1};
        int[] outward={0,2,1,0,1,3,0,3,2,1,2,3};
        Cad3dMeshTopology.Result accepted=Cad3dMeshTopology.inspect(tetra,outward);
        Cad3dNativeMeshMass.Comparison comparison=
            Cad3dNativeMeshMass.compare(tetra,outward,accepted);
        require(comparison.javaResult.available&&!comparison.agreesWithJava&&
                !comparison.nativeAvailable&&
                "native_authoritative_no_independent_oracle".equals(comparison.status),
                "Native vs native is NOT independent equivalence evidence");
        int[] duplicate={0,2,1,0,1,3,0,3,2,0,2,1};
        Cad3dSignedVolume.Result denied=Cad3dSignedVolume.measure(
                tetra,duplicate,accepted);
        require(!denied.available&&Double.isNaN(denied.centroidX),
                "Same face-count but stale topology cannot certify volume");
        System.out.println("PASS: actual JNI topology + volume and stale-approval rejection");
    }
}
