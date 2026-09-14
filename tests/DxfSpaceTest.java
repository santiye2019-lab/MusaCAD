import com.musa.cad.DxfSpace;
import java.util.*;

public class DxfSpaceTest {
    private static void eq(String expected,String actual){if(!expected.equals(actual))throw new AssertionError(actual+" != "+expected);}
    public static void main(String[] args){
        eq("Model",DxfSpace.layout(0,""));
        eq("Model",DxfSpace.layout(0,"*MODEL_SPACE"));
        eq("Layout1",DxfSpace.layout(1,"Layout1"));
        eq("Layout2",DxfSpace.layout(0,"Layout2"));
        eq("Paper",DxfSpace.layout(1,"Model"));

        List<String> layoutTags=Arrays.asList("100","AcDbPlotSettings","1","","330","ROOT","100","AcDbLayout","1","A4","330","7");
        eq("A4",DxfSpace.layoutObjectName(layoutTags,0,layoutTags.size()));
        eq("7",DxfSpace.layoutObjectOwner(layoutTags,0,layoutTags.size()));
        Map<String,String> ownerMap=new HashMap<>();ownerMap.put("7","A4");
        eq("A4",DxfSpace.layout(1,"","7",ownerMap));
        eq("Paper",DxfSpace.layout(1,"","8",ownerMap));
        String unresolved=DxfSpace.unresolvedPaper("7");
        if(!DxfSpace.isUnresolvedPaper(unresolved))throw new AssertionError("unresolved marker");
        eq("7",DxfSpace.unresolvedOwner(unresolved));
        eq("A4",DxfSpace.resolveUnresolved(unresolved,ownerMap));

        LinkedHashMap<String,List<Integer>> spaces=new LinkedHashMap<>();
        spaces.put("Layout1",new ArrayList<>(Arrays.asList(1)));
        spaces.put("Model",new ArrayList<>(Arrays.asList(2)));
        eq("Model",DxfSpace.chooseActive(spaces));
        eq("Layout1",DxfSpace.chooseActive(spaces,"Layout1"));
        eq("Model",DxfSpace.chooseActive(spaces,"model"));
        eq("Model",DxfSpace.chooseActive(spaces,"Missing"));
        spaces.get("Model").clear();
        eq("Layout1",DxfSpace.chooseActive(spaces));
        eq("Layout1",DxfSpace.chooseActive(spaces,"Model"));
        spaces.clear();
        eq("Model",DxfSpace.chooseActive(spaces));
        System.out.println("22 DXF model/paper/layout-owner cases passed");
    }
}
