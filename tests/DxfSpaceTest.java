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
        System.out.println("13 DXF model/paper-space cases passed");
    }
}
