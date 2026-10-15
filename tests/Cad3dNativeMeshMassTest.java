import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dNativeMeshMass;

public final class Cad3dNativeMeshMassTest {
    private static void need(boolean ok,String label){
        if(!ok)throw new AssertionError(label);
    }
    public static void main(String[] args){
        float[] tetra={0,0,0,1,0,0,0,1,0,0,0,1};
        int[] outward={0,2,1,0,1,3,0,3,2,1,2,3};
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(tetra,outward);
        Cad3dNativeMeshMass.Comparison result=
            Cad3dNativeMeshMass.compare(tetra,outward,topology);
        need(result.javaResult.available,"trusted Java result survives");
        need(Math.abs(result.javaResult.signedVolume-1d/6)<1e-12,"volume unchanged");
        need(!result.agreesWithJava,"no manufactured Android-native PASS in JVM");
        need(result.status.equals("native_unavailable"),"host JVM requires explicit unavailable");
        Cad3dSignedVolume.Result java=result.javaResult;
        need(Cad3dNativeMeshMass.agrees(java,new double[]{1d/6,.25,.25,.25}),
             "matching synthetic candidate agrees numerically only");
        need(!Cad3dNativeMeshMass.agrees(java,new double[]{1d/6,.25,.25,.26}),
             "real semantic discrepancy rejected");
        need(!Cad3dNativeMeshMass.agrees(java,new double[]{1d/6,Double.NaN,.25,.25}),
             "non-finite candidate rejected");
        need(!Cad3dNativeMeshMass.agrees(java,new double[]{1d/6,.25,.25}),
             "incomplete candidate rejected");

        int[] open={0,2,1};
        Cad3dNativeMeshMass.Comparison unsupported=Cad3dNativeMeshMass.compare(
            tetra,open,Cad3dMeshTopology.inspect(tetra,open));
        need(!unsupported.javaResult.available,"open mesh never certified");
        need(unsupported.status.equals("mesh_not_accepted"),"native not run on open geometry");
        System.out.println("MusaCAD JNI 3D mass optional compare/fallback safety PASS (host JVM)");
    }
}
