import com.musa.cad.*;
import java.util.*;

public class DxfStyleTest {
    public static void main(String[] args){
        if(!DxfStyle.BYLAYER.equals(DxfStyle.resolveLineType("BYBLOCK",null)))throw new AssertionError("BYBLOCK root");
        if(!"DASHED".equals(DxfStyle.resolveLineType("BYBLOCK","DASHED")))throw new AssertionError("BYBLOCK inherit");
        Map<String,String> lt=new HashMap<>();lt.put("BORU","CENTER");
        if(!"CENTER".equals(DxfStyle.effectiveLineType("BYLAYER","boru",lt)))throw new AssertionError("layer linetype");
        Map<String,Integer> lw=new HashMap<>();lw.put("BORU",35);
        if(DxfStyle.effectiveLineWeight(-1,"boru",lw)!=35)throw new AssertionError("layer lineweight");
        if(DxfStyle.strokeWidthPx(100)<=DxfStyle.strokeWidthPx(25))throw new AssertionError("lineweight width");
        System.out.println("DXF style cases passed");
    }
}
