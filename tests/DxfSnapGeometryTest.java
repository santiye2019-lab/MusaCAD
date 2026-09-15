import com.musa.cad.DxfSnapGeometry;
import java.util.Arrays;

public class DxfSnapGeometryTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-6)throw new AssertionError(a+" != "+b);}
    private static void length(double[] a,int n){if(a.length!=n)throw new AssertionError("length "+a.length+" != "+n+" "+Arrays.toString(a));}
    public static void main(String[] args){
        double[] line=DxfSnapGeometry.line(0,0,10,4);length(line,6);close(line[4],5);close(line[5],2);
        double[] poly=DxfSnapGeometry.poly(new double[]{0,0,10,0,10,10},false);length(poly,10);close(poly[6],5);close(poly[7],0);close(poly[8],10);close(poly[9],5);
        double[] closed=DxfSnapGeometry.poly(new double[]{0,0,10,0,10,10},true);length(closed,12);close(closed[10],5);close(closed[11],5);
        double[] circle=DxfSnapGeometry.circle(2,3,5,0,360);length(circle,10);close(circle[2],7);close(circle[3],3);close(circle[4],2);close(circle[5],8);
        double[] arc=DxfSnapGeometry.circle(0,0,10,0,90);length(arc,8);close(arc[2],10);close(arc[3],0);close(arc[4],0);close(arc[5],10);close(arc[6],Math.sqrt(50));close(arc[7],Math.sqrt(50));
        double[] ellipse=DxfSnapGeometry.ellipse(0,0,10,0,.5,0,Math.PI*2);length(ellipse,10);close(ellipse[2],10);close(ellipse[3],0);close(ellipse[4],0);close(ellipse[5],5);
        System.out.println("6 DXF snap geometry cases passed");
    }
}
