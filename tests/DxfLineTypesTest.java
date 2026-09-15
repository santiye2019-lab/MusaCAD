import com.musa.cad.*;
import java.util.*;

public class DxfLineTypesTest {
    public static void main(String[] args)throws Exception{
        List<String> tags=Arrays.asList(
            "0","SECTION","2","HEADER","9","$LTSCALE","40","2.0","0","ENDSEC",
            "0","SECTION","2","TABLES",
            "0","LTYPE","2","DASHED","73","2","49","0.5","49","-0.25",
            "0","LTYPE","2","GAS","73","3","49","0.5","74","0",
                "49","-0.2","74","2","9","GAS","46","0.1","50","15","44","-0.05","45","0.02","340","ABCD",
                "49","-0.3","74","0",
            "0","ENDSEC","0","EOF");
        DxfLineTypes.Table table=DxfLineTypes.parse(tags);
        if(Math.abs(table.globalScale-2.0)>1e-9)throw new AssertionError("ltscale");
        DxfLineTypes.Pattern p=table.get("dashed");if(p==null||p.elements.length!=2)throw new AssertionError("pattern");
        float[] d=DxfLineTypes.dashIntervals(p,table.globalScale,10);
        if(d==null||d.length!=2||d[0]<=d[1])throw new AssertionError("dash intervals");

        DxfLineTypes.Pattern gas=table.get("gas");
        if(gas==null||gas.sequence.length!=3||!gas.complex())throw new AssertionError("complex pattern");
        DxfLineTypes.Element text=gas.sequence[1];
        if(text.kind()!=DxfLineTypes.KIND_TEXT||!"GAS".equals(text.text)||Math.abs(text.scale-.1)>1e-9||
           Math.abs(text.rotation-15)>1e-9||Math.abs(text.xOffset+.05)>1e-9||Math.abs(text.yOffset-.02)>1e-9||!"ABCD".equals(text.styleHandle))
            throw new AssertionError("complex text metadata");
        List<DxfLineTypes.Placement> placements=DxfLineTypes.decorations(gas,1,10,26);
        if(placements.size()!=3||Math.abs(placements.get(0).distance-5)>1e-9||Math.abs(placements.get(1).distance-15)>1e-9||
           Math.abs(placements.get(0).xOffsetPixels+.5)>1e-9||Math.abs(placements.get(0).yOffsetPixels-.2)>1e-9)
            throw new AssertionError("complex text placements");

        DxfLineTypes.Table streamed=new DxfLineTypes.Table();
        streamed.addRecord(Arrays.asList("2","FENCE","49","0.4","49","-0.1","74","1","75","132","46","0.2"));
        DxfLineTypes.Pattern fence=streamed.get("FENCE");
        if(fence==null||!fence.complex()||fence.sequence[1].kind()!=DxfLineTypes.KIND_SHAPE||fence.sequence[1].shapeNumber!=132)
            throw new AssertionError("complex shape metadata");
        System.out.println("DXF simple and complex linetype cases passed");
    }
}
