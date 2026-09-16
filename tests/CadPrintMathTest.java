import com.musa.cad.CadPrintMath;

public class CadPrintMathTest {
    private static void near(double actual,double expected){if(Math.abs(actual-expected)>1e-9*Math.max(1,Math.abs(expected)))throw new AssertionError(actual+" != "+expected);}
    public static void main(String[] args){
        near(CadPrintMath.millimetersPerUnit(4),1.0);
        near(CadPrintMath.millimetersPerUnit(5),10.0);
        near(CadPrintMath.millimetersPerUnit(6),1000.0);
        near(CadPrintMath.millimetersPerUnit(1),25.4);
        near(CadPrintMath.millimetersPerUnit(2),304.8);
        if(!Double.isNaN(CadPrintMath.millimetersPerUnit(0)))throw new AssertionError("unitless must be unknown");
        near(CadPrintMath.pointsPerDrawingUnit(1.0,100),72.0/2540.0);
        near(CadPrintMath.pointsPerDrawingUnit(1000.0,100),72.0/2.54);
        if(!Double.isNaN(CadPrintMath.pointsPerDrawingUnit(Double.NaN,50)))throw new AssertionError("unknown scale");
        System.out.println("DXF physical print scale cases passed");
    }
}
