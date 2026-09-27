import com.musa.cad.DxfAttributeVisibility;
import java.util.*;

public class DxfAttributeVisibilityTest {
    private static List<String> tags(Object... values){ArrayList<String> out=new ArrayList<>();for(Object v:values)out.add(String.valueOf(v));return out;}
    private static void expect(boolean expected,boolean actual,String name){if(expected!=actual)throw new AssertionError(name+" expected "+expected+" got "+actual);}
    public static void main(String[] args){
        List<String> attribHidden=tags(100,"AcDbText",71,0,100,"AcDbAttribute",70,1,100,"AcDbXrecord",70,0);
        List<String> attribVisible=tags(100,"AcDbText",71,2,100,"AcDbAttribute",70,2,100,"AcDbXrecord",70,1);
        List<String> attdefHidden=tags(100,"AcDbText",100,"AcDbAttributeDefinition",70,9,100,"AcDbXrecord",70,0);
        expect(true,DxfAttributeVisibility.isInvisible("ATTRIB",attribHidden,0,attribHidden.size()),"scoped ATTRIB invisible");
        expect(false,DxfAttributeVisibility.isInvisible("ATTRIB",attribVisible,0,attribVisible.size()),"constant ATTRIB stays visible");
        expect(true,DxfAttributeVisibility.isInvisible("ATTDEF",attdefHidden,0,attdefHidden.size()),"scoped ATTDEF invisible");
        List<String> legacy=tags(70,1,10,0,20,0);expect(true,DxfAttributeVisibility.isInvisible("ATTRIB",legacy,0,legacy.size()),"legacy flag fallback");
        expect(false,DxfAttributeVisibility.isInvisible("TEXT",tags(70,1),0,2),"TEXT group 70 is not attribute visibility");
        System.out.println("DXF ATTRIB/ATTDEF visibility cases passed");
    }
}
