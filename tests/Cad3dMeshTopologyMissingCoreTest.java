import com.musa.cad.Cad3dMeshTopology;
/** When Android native core cannot load, NEVER fall back to Java CAD math. */
public final class Cad3dMeshTopologyMissingCoreTest {
    public static void main(String[] args){
        System.setProperty("java.vm.name","Dalvik");
        boolean blocked=false;
        try{
            Cad3dMeshTopology.inspect(new float[]{0,0,0,1,0,0,0,1,0},
                                      new int[]{0,1,2});
        }catch(LinkageError expected){
            blocked=true;
        }
        if(!blocked)throw new AssertionError("Android silently used Java CAD topology");
        System.out.println("PASS: missing native CAD engine fails closed");
    }
}
