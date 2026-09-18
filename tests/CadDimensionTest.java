import com.musa.cad.*;
import java.util.*;

public class CadDimensionTest {
    public static void main(String[] args){
        CadDimension.Style style=new CadDimension.Style(10f,4f,2);
        List<CadEdit> linear=CadDimension.linear(0,0,100,0,50,20,1d,style);
        if(linear.size()!=8)throw new AssertionError("linear entity count "+linear.size());
        CadEdit text=linear.get(linear.size()-1);
        if(text.type!=CadEdit.Type.TEXT||!"100.00".equals(text.text))throw new AssertionError("linear label "+text.text);

        List<CadEdit> vertical=CadDimension.linear(0,0,0,50,20,25,2d,style);
        CadEdit vtext=vertical.get(vertical.size()-1);
        if(!"100.00".equals(vtext.text))throw new AssertionError("vertical units "+vtext.text);

        List<CadEdit> aligned=CadDimension.aligned(0,0,30,40,10,20,1d,style);
        if(aligned.size()!=8)throw new AssertionError("aligned entity count "+aligned.size());
        CadEdit atext=aligned.get(aligned.size()-1);
        if(!"50.00".equals(atext.text))throw new AssertionError("aligned label "+atext.text);
        if(Math.abs(atext.rotationDegrees-53.1301f)>.02f)throw new AssertionError("aligned rotation "+atext.rotationDegrees);
        System.out.println("Dimension geometry/style cases passed");
    }
}
