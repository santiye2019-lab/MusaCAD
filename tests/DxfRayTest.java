import com.musa.cad.DxfRay;
import java.util.*;

public class DxfRayTest {
    private static List<String> tags(Object... values){ArrayList<String> out=new ArrayList<>();for(Object v:values)out.add(String.valueOf(v));return out;}
    private static void near(double actual,double expected,String name){if(Math.abs(actual-expected)>1e-9)throw new AssertionError(name+" "+actual+" != "+expected);}
    public static void main(String[] args){
        DxfRay.Data r=DxfRay.parse(tags(10,5,20,-2,11,3,21,4),0,8);
        if(r==null)throw new AssertionError("valid ray missing");
        near(r.x,5,"x");near(r.y,-2,"y");near(r.dx,.6,"dx");near(r.dy,.8,"dy");
        if(DxfRay.parse(tags(10,0,20,0,11,0,21,0),0,8)!=null)throw new AssertionError("zero direction");
        if(DxfRay.parse(tags(10,0,20,0,11,1),0,6)!=null)throw new AssertionError("missing direction component");
        if(DxfRay.parse(tags(10,"bad",20,0,11,1,21,0),0,8)!=null)throw new AssertionError("invalid numeric value");
        System.out.println("DXF RAY geometry cases passed");
    }
}
