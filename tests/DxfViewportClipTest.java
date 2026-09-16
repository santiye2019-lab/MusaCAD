import com.musa.cad.DxfViewportClip;
import java.util.Arrays;

public class DxfViewportClipTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void no(boolean v,String m){if(v)throw new AssertionError(m);}
    private static void close(double a,double b,String m){if(Math.abs(a-b)>1e-9)throw new AssertionError(m+": "+a+" != "+b);}
    public static void main(String[] args){
        double[] square={0,0,10,0,10,10,0,10,0,0};
        double[] clean=DxfViewportClip.polygon(square);if(clean.length!=8)throw new AssertionError(Arrays.toString(clean));
        yes(DxfViewportClip.contains(clean,5,5,0),"square inside");
        no(DxfViewportClip.contains(clean,12,5,0),"square outside");
        yes(DxfViewportClip.contains(clean,10.2,5,.25),"edge tolerance");
        no(DxfViewportClip.contains(clean,10.3,5,.25),"beyond tolerance");
        double[] concave={0,0,8,0,8,3,3,3,3,8,0,8};
        yes(DxfViewportClip.contains(concave,1,7,0),"concave arm");
        yes(DxfViewportClip.contains(concave,7,1,0),"concave base");
        no(DxfViewportClip.contains(concave,6,6,0),"concave cutout");
        double[] bounds=DxfViewportClip.bounds(concave);close(bounds[0],0,"left");close(bounds[1],0,"bottom");close(bounds[2],8,"right");close(bounds[3],8,"top");
        if(DxfViewportClip.polygon(new double[]{0,0,1,1,2,2}).length!=0)throw new AssertionError("collinear accepted");
        if(DxfViewportClip.polygon(new double[]{0,0,Double.NaN,1,2,0}).length!=0)throw new AssertionError("NaN accepted");
        System.out.println("12 nonrectangular viewport clip cases passed");
    }
}
