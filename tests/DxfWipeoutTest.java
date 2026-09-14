import com.musa.cad.*;
import java.util.*;

public class DxfWipeoutTest {
    public static void main(String[] args){
        List<double[]> clip=Arrays.asList(new double[]{-.5,-.5},new double[]{9.5,-.5},new double[]{9.5,4.5},new double[]{-.5,4.5});
        double[] p=DxfWipeout.boundary(100,200,2,0,0,3,10,5,clip);
        if(p.length!=8)throw new AssertionError("point count");
        if(Math.abs(p[0]-100)>1e-9||Math.abs(p[1]-200)>1e-9)throw new AssertionError("origin");
        if(Math.abs(p[2]-120)>1e-9||Math.abs(p[5]-215)>1e-9)throw new AssertionError("basis transform");
        System.out.println("WIPEOUT geometry cases passed");
    }
}
