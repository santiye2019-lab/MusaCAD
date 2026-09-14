import com.musa.cad.MeasurementMath;
public class MeasurementMathTest {
    private static void area(double expected,float... p){if(Math.abs(MeasurementMath.area(p)-expected)>1e-10)throw new AssertionError("Area");}
    public static void main(String[] args){
        area(1,2000,2000,2001,2000,2001,2001,2000,2001);
        area(.015625,2000,2000,2000,2000.125f,2000.125f,2000.125f,2000.125f,2000);
        area(3,0,0,2,0,2,1,1,1,1,2,0,2);
        area(0,0,0,1,1,2,2);area(0,0,0,1,1);
        if(MeasurementMath.distance(0,0,3,4)!=5)throw new AssertionError("Distance");
        if(!Double.isFinite(MeasurementMath.distance(Float.MAX_VALUE,0,-Float.MAX_VALUE,0)))throw new AssertionError("Float subtraction overflow");
        System.out.println("7 measurement cases passed");
    }
}
