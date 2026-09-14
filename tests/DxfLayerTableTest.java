import com.musa.cad.DxfLayerTable;
import java.util.*;

public class DxfLayerTableTest {
    public static void main(String[] args)throws Exception{
        List<String> tags=Arrays.asList(
            "0","SECTION","2","TABLES",
            "0","TABLE","2","LAYER",
            "0","LAYER","2","Visible","70","0","62","3",
            "0","LAYER","2","OffLayer","70","0","62","-1",
            "0","LAYER","2","FrozenLayer","70","1","62","5",
            "0","LAYER","2","LockedLayer","70","4","62","2",
            "0","ENDTAB","0","ENDSEC","0","EOF"
        );
        DxfLayerTable.Table t=DxfLayerTable.parse(tags);
        if(t.names.size()!=4)throw new AssertionError("all layers");
        if(!t.visible.contains("Visible")||!t.visible.contains("LockedLayer"))throw new AssertionError("visible layers");
        if(t.visible.contains("OffLayer")||!t.off.contains("OffLayer"))throw new AssertionError("off layer");
        if(t.visible.contains("FrozenLayer")||!t.frozen.contains("FrozenLayer"))throw new AssertionError("frozen layer");
        if(!t.locked.contains("LockedLayer"))throw new AssertionError("locked layer");
        if(t.colors.get("VISIBLE")!=0xFF00FF00)throw new AssertionError("layer color");
        System.out.println("DXF layer table states passed");
    }
}
