import com.musa.cad.DxfPolylineWidth;

public class DxfPolylineWidthTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        close(DxfPolylineWidth.constant(-20),20);
        close(DxfPolylineWidth.constant(Double.NaN),0);
        close(DxfPolylineWidth.uniform(5,5.0000001),5.00000005);
        close(DxfPolylineWidth.uniform(5,7),0);
        close(DxfPolylineWidth.uniform(5,0),0);
        close(DxfPolylineWidth.device(3,2,2),6);
        close(DxfPolylineWidth.device(20,1.5,.5),20);
        close(DxfPolylineWidth.device(0,2,2),0);
        close(DxfPolylineWidth.device(1,0,0),0);
        System.out.println("DXF geometric polyline width cases passed");
    }
}
