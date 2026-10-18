import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
/** Android may never run a duplicate Java volume engine on core failure. */
public final class Cad3dSignedVolumeMissingCoreTest {
    public static void main(String[] args){
        System.setProperty("java.vm.name","Dalvik");
        boolean blocked=false;
        try {
            float[] tetra={0,0,0,1,0,0,0,1,0,0,0,1};
            int[] faces={0,2,1,0,1,3,0,3,2,1,2,3};
            Cad3dSignedVolume.measure(tetra,faces,
                    Cad3dMeshTopology.inspect(tetra,faces));
        } catch(LinkageError expected){blocked=true;}
        if(!blocked)throw new AssertionError("Java mass calculation used without native core");
        System.out.println("PASS: absent C engine prevents Android Java volume fallback");
    }
}
