import com.musa.cad.DxfComplexLineText;
import com.musa.cad.DxfLineStyle;
import java.util.*;

public final class DxfComplexLineTextTest {
    public static void main(String[] args){
        DxfLineStyle.ComplexElement relative=new DxfLineStyle.ComplexElement(1,2,0,"GAS",2,10,1,.5);
        DxfLineStyle.Pattern p=new DxfLineStyle.Pattern("GAS",new double[]{10,-2},true,Collections.singletonList(relative));
        List<DxfComplexLineText.Placement> placements=DxfComplexLineText.placements(p,0,0,60,0,1);
        require(!placements.isEmpty(),"relative placements");
        DxfComplexLineText.Placement first=placements.get(0);
        require(close(first.x,12)&&close(first.y,.5)&&close(first.angleDegrees,10)&&close(first.textSize,2),"relative placement values");
        DxfLineStyle.ComplexElement absolute=new DxfLineStyle.ComplexElement(0,3,0,"A",1,30,0,0);
        DxfLineStyle.Pattern abs=new DxfLineStyle.Pattern("ABS",new double[]{10,-2},true,Collections.singletonList(absolute));
        List<DxfComplexLineText.Placement> vertical=DxfComplexLineText.placements(abs,0,0,0,60,1);
        require(!vertical.isEmpty()&&close(vertical.get(0).angleDegrees,30),"absolute rotation");
        DxfLineStyle.ComplexElement rel2=new DxfLineStyle.ComplexElement(0,2,0,"R",1,5,0,0);
        DxfLineStyle.Pattern relPattern=new DxfLineStyle.Pattern("REL",new double[]{10,-2},true,Collections.singletonList(rel2));
        List<DxfComplexLineText.Placement> relativeVertical=DxfComplexLineText.placements(relPattern,0,0,0,60,1);
        require(!relativeVertical.isEmpty()&&close(relativeVertical.get(0).angleDegrees,95),"relative rotation");
        System.out.println("DxfComplexLineTextTest OK");
    }
    private static boolean close(double a,double b){return Math.abs(a-b)<1e-6;}
    private static void require(boolean ok,String name){if(!ok)throw new AssertionError(name);}
}
