import com.musa.cad.*;
import java.util.*;

public class DxfLineTypesTest {
    public static void main(String[] args)throws Exception{
        List<String> tags=Arrays.asList(
            "0","SECTION","2","HEADER","9","$LTSCALE","40","2.0","0","ENDSEC",
            "0","SECTION","2","TABLES","0","LTYPE","2","DASHED","73","2","49","0.5","49","-0.25","0","ENDSEC","0","EOF");
        DxfLineTypes.Table table=DxfLineTypes.parse(tags);
        if(Math.abs(table.globalScale-2.0)>1e-9)throw new AssertionError("ltscale");
        DxfLineTypes.Pattern p=table.get("dashed");if(p==null||p.elements.length!=2)throw new AssertionError("pattern");
        float[] d=DxfLineTypes.dashIntervals(p,table.globalScale,10);
        if(d==null||d.length!=2||d[0]<=d[1])throw new AssertionError("dash intervals");
        System.out.println("DXF linetype cases passed");
    }
}
