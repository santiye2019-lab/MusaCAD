import com.musa.cad.DxfOcs;
import java.util.Arrays;

public class DxfOcsTest {
    private static void eq(double[] a,double... expected){
        if(a.length!=expected.length)throw new AssertionError(Arrays.toString(a));
        for(int i=0;i<a.length;i++)if(Math.abs(a[i]-expected[i])>1e-6)
            throw new AssertionError(Arrays.toString(a)+" != "+Arrays.toString(expected));
    }
    private static double[] point(double[] m,double x,double y){return new double[]{m[0]*x+m[2]*y+m[4],m[1]*x+m[3]*y+m[5]};}
    private static void pointEq(double[] m,double x,double y,double ex,double ey){
        double[] p=point(m,x,y);if(Math.abs(p[0]-ex)>1e-6||Math.abs(p[1]-ey)>1e-6)throw new AssertionError(Arrays.toString(p));
    }
    public static void main(String[] args){
        double[] normal=DxfOcs.insert2d(1,2,0,1,1,1,0,10,20,0,0,0,1,0,0);
        eq(normal,1,0,0,1,9,18);pointEq(normal,3,4,12,22);

        double[] rotated=DxfOcs.insert2d(1,2,0,2,3,1,90,10,20,0,0,0,1,0,0);
        pointEq(rotated,3,4,4,24);

        // Negative OCS Z mirrors the OCS X axis when projected into WCS XY.
        double[] reverse=DxfOcs.insert2d(1,2,0,1,1,1,0,10,20,0,0,0,-1,0,0);
        pointEq(reverse,3,4,-12,22);

        // Oblique extrusion is projected onto the WCS XY plane using AutoCAD's arbitrary axes.
        double[] oblique=DxfOcs.insert2d(1,2,0,1,1,1,0,10,20,0,0,1,1,0,0);
        double root=Math.sqrt(.5);pointEq(oblique,3,4,-12,-22*root);

        // Array spacing moves successive block references independently of block scale.
        double[] array=DxfOcs.insert2d(1,2,0,2,3,1,0,10,20,0,0,0,1,5,7);
        pointEq(array,3,4,19,33);
        System.out.println("5 OCS transform cases passed");
    }
}
