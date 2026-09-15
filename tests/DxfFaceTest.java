import com.musa.cad.DxfFace;
import java.util.Arrays;

public class DxfFaceTest {
    private static void same(double[] actual,double... expected){
        if(actual.length!=expected.length)throw new AssertionError("length: "+Arrays.toString(actual));
        for(int i=0;i<actual.length;i++)if(Math.abs(actual[i]-expected[i])>1e-9)throw new AssertionError(Arrays.toString(actual));
    }
    public static void main(String[] args){
        double[] quad={0,0,10,0,10,5,0,5};
        same(DxfFace.visibleEdges(quad,0),0,0,10,0,10,0,10,5,10,5,0,5,0,5,0,0);
        same(DxfFace.visibleEdges(quad,1),10,0,10,5,10,5,0,5,0,5,0,0);
        same(DxfFace.visibleEdges(quad,2|8),0,0,10,0,10,5,0,5);
        same(DxfFace.visibleEdges(quad,1|2|4|8));

        // DXF triangles commonly duplicate vertex 3 as vertex 4. The zero-length edge is omitted.
        double[] tri={0,0,10,0,4,5,4,5};
        same(DxfFace.visibleEdges(tri,0),0,0,10,0,10,0,4,5,4,5,0,0);
        same(DxfFace.visibleEdges(tri,8),0,0,10,0,10,0,4,5);
        same(DxfFace.visibleEdges(new double[]{0,0,10,0,4,5},0),0,0,10,0,10,0,4,5,4,5,0,0);
        if(DxfFace.visibleEdges(new double[]{0,0,1,1},0).length!=0)throw new AssertionError("short face");
        System.out.println("8 3DFACE edge visibility cases passed");
    }
}
