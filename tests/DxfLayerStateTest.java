import com.musa.cad.DxfLayerState;
import java.util.*;

public class DxfLayerStateTest {
    public static void main(String[] args){
        if(!DxfLayerState.tableVisible(7,0))throw new AssertionError("normal layer visible");
        if(DxfLayerState.tableVisible(-7,0))throw new AssertionError("negative ACI means off");
        if(DxfLayerState.tableVisible(7,1))throw new AssertionError("frozen bit");
        if(!DxfLayerState.tableVisible(7,4))throw new AssertionError("locked is still visible");

        String[] chain=DxfLayerState.addGate(new String[0],"A");
        chain=DxfLayerState.addGate(chain,"0".equals("A")?"0":"C");
        chain=DxfLayerState.addGate(chain,"a");
        if(chain.length!=2||!"A".equals(chain[0])||!"C".equals(chain[1]))throw new AssertionError("gate chain");

        Set<String> both=DxfLayerState.normalized(new LinkedHashSet<>(Arrays.asList("a","c")));
        if(!DxfLayerState.allVisible(chain,both))throw new AssertionError("both gates visible");
        Set<String> childOnly=DxfLayerState.normalized(Collections.singleton("C"));
        if(DxfLayerState.allVisible(chain,childOnly))throw new AssertionError("hidden insert must hide child");
        Set<String> parentOnly=DxfLayerState.normalized(Collections.singleton("A"));
        if(DxfLayerState.allVisible(chain,parentOnly))throw new AssertionError("hidden child layer must hide child");

        System.out.println("DXF layer visibility cases passed");
    }
}
