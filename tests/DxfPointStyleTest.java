import com.musa.cad.DxfPointStyle;
import java.util.*;

public class DxfPointStyleTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        List<String> tags=Arrays.asList("9","$PDMODE","70","66","9","$PDSIZE","40","-3.0");
        DxfPointStyle.Style s=DxfPointStyle.parse(tags);
        if(s.mode!=66||s.base()!=2||!s.square()||s.circle()||s.hidden())throw new AssertionError("mode");
        close(s.size,-3);close(DxfPointStyle.deviceHalfSize(s,10,1000),15);
        close(DxfPointStyle.deviceHalfSize(new DxfPointStyle.Style(2,2.5),4,1000),5);
        close(DxfPointStyle.deviceHalfSize(new DxfPointStyle.Style(0,0),4,1000),25);
        DxfPointStyle.Style hidden=new DxfPointStyle.Style(1,0);if(!hidden.hidden())throw new AssertionError("hidden");
        DxfPointStyle.Style combined=new DxfPointStyle.Style(96,0);if(!combined.circle()||!combined.square()||combined.base()!=0)throw new AssertionError("circle+square");
        System.out.println("DXF PDMODE/PDSIZE cases passed");
    }
}
