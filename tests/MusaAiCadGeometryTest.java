import com.musa.cad.MusaAiCadGeometry;
import java.util.Arrays;

public final class MusaAiCadGeometryTest {
    private static void near(double a,double b){if(Math.abs(a-b)>1e-6)throw new AssertionError(a+" != "+b);}
    private static void arr(double[] a,double...b){
        if(a==null||a.length!=b.length)throw new AssertionError("array "+Arrays.toString(a));
        for(int i=0;i<a.length;i++)near(a[i],b[i]);
    }

    public static void main(String[]args){
        double[] target={0,0,10,0}, boundary={5,-5,5,5};
        arr(MusaAiCadGeometry.trimLine(target,boundary,1,0),5,0,10,0);
        arr(MusaAiCadGeometry.trimLine(target,boundary,9,0),0,0,5,0);

        arr(MusaAiCadGeometry.extendLine(new double[]{0,0,4,0},boundary),0,0,5,0);
        arr(MusaAiCadGeometry.extendLine(new double[]{6,0,10,0},boundary),5,0,10,0);
        if(MusaAiCadGeometry.extendLine(target,boundary)!=null)throw new AssertionError("already crossing line must not extend");

        arr(MusaAiCadGeometry.continuePath(new double[]{0,0,10,0},false,new double[]{15,0,20,5}),
            0,0,10,0,15,0,20,5);
        arr(MusaAiCadGeometry.continuePath(new double[]{0,0,10,0},true,new double[]{-5,0,-10,5}),
            -10,5,-5,0,0,0,10,0);

        if(!MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0},false))throw new AssertionError("open polyline");
        if(!MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0,1,1},true))throw new AssertionError("closed polyline");
        if(MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0},true))throw new AssertionError("closed needs 3 points");

        System.out.println("MusaAiCadGeometryTest OK");
    }
}
