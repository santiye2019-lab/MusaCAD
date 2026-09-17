import com.musa.cad.DxfLineStyle;
import com.musa.cad.DxfLineTypeParser;
import java.util.*;

public final class DxfComplexLineTypeTest {
    public static void main(String[] args){
        List<String> tags=Arrays.asList(
            "49","12.0",
            "49","-3.0",
            "74","3",
            "9","GAS",
            "46","1.5",
            "50","0.2617993877991494",
            "44","0.25",
            "45","-0.5",
            "49","-3.0",
            "74","4",
            "75","132",
            "46","2.0"
        );
        DxfLineStyle.Pattern p=DxfLineTypeParser.parse("GAS_LINE",tags,0,tags.size());
        require(p.complex,"complex flag");
        require(p.elements.length==3,"element count");
        require(p.complexElements.size()==2,"complex element count");
        DxfLineStyle.ComplexElement text=p.complexElements.get(0);
        require(text.elementIndex==1,"text index");
        require(text.hasText()&&"GAS".equals(text.text),"text payload");
        require((text.flags&1)!=0,"absolute rotation flag");
        require(close(text.scale,1.5)&&close(text.rotationDegrees,15)&&close(text.xOffset,.25)&&close(text.yOffset,-.5),"text transforms");
        DxfLineStyle.ComplexElement shape=p.complexElements.get(1);
        require(shape.elementIndex==2&&shape.hasShape()&&shape.shapeNumber==132,"shape metadata");
        require(close(p.cycleLength(),18d),"cycle length");
        require(close(p.elementCenterDistance(1),13.5d),"element center");
        System.out.println("DxfComplexLineTypeTest OK");
    }
    private static boolean close(double a,double b){return Math.abs(a-b)<1e-6;}
    private static void require(boolean ok,String name){if(!ok)throw new AssertionError(name);}
}
