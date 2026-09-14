import com.musa.cad.DxfViewport;
import java.util.Arrays;

public class DxfViewportTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-6)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        DxfViewport.Spec v=DxfViewport.of(100,50,80,40,10,20,20,0,2,1,0,0,1);
        if(!v.supported())throw new AssertionError("top viewport");
        double[] p=DxfViewport.point(v,10,20);close(p[0],100);close(p[1],50);
        p=DxfViewport.point(v,30,20);close(p[0],140);close(p[1],50);
        close(v.left(),60);close(v.right(),140);close(v.bottom(),30);close(v.top(),70);

        DxfViewport.Spec rotated=DxfViewport.of(0,0,100,100,0,0,100,Math.PI/2,3,1,0,0,1);
        p=DxfViewport.point(rotated,10,0);close(p[0],0);close(p[1],-10);

        if(DxfViewport.of(0,0,10,10,0,0,10,0,1,1,0,0,1).supported())throw new AssertionError("overall paper viewport");
        if(DxfViewport.of(0,0,10,10,0,0,10,0,2,0,0,0,1).supported())throw new AssertionError("off viewport");
        if(DxfViewport.of(0,0,10,10,0,0,10,0,2,1,1,0,1).supported())throw new AssertionError("3D view not yet 2D-safe");
        System.out.println("10 DXF viewport geometry cases passed");
    }
}
