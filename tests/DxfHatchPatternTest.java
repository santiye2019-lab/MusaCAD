import com.musa.cad.DxfHatchPattern;
import java.util.Arrays;

public class DxfHatchPatternTest {
    private static void eq(int[] actual,int... expected){if(!Arrays.equals(actual,expected))throw new AssertionError(Arrays.toString(actual)+" != "+Arrays.toString(expected));}
    private static void near(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        DxfHatchPattern.Line horizontal=new DxfHatchPattern.Line(0,0,0,0,10,new double[0]);
        eq(DxfHatchPattern.familyRange(horizontal,0,0,100,100),-1,11);

        DxfHatchPattern.Line ansi31=new DxfHatchPattern.Line(45,-45110.60586052452,-7638.433636080381,-22.45064030267288,22.45064030267288,new double[0]);
        int[] range=DxfHatchPattern.familyRange(ansi31,-45200,-7700,-45000,-7500);
        if(range[1]<range[0]||range[1]-range[0]>30)throw new AssertionError(Arrays.toString(range));

        DxfHatchPattern.Line dashed=new DxfHatchPattern.Line(45,0,0,-4.490128060534577,4.490128060534577,new double[]{3.175,-1.5875});
        near(DxfHatchPattern.dashCycle(dashed),4.7625);

        DxfHatchPattern.Line dots=new DxfHatchPattern.Line(45,0,0,-22.45064030267288,67.35192090801866,new double[]{0,-63.5});
        near(DxfHatchPattern.dashCycle(dots),63.5);

        System.out.println("4 HATCH pattern geometry cases passed");
    }
}
