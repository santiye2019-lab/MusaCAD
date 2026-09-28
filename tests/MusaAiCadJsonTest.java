import com.musa.cad.MusaAiCadJson;
import com.musa.cad.MusaAiDrawingIndex;
import java.util.*;

public final class MusaAiCadJsonTest {
    private static void has(String source,String wanted){
        if(!source.contains(wanted))throw new AssertionError("missing "+wanted+" in "+source);
    }
    public static void main(String[]args){
        MusaAiDrawingIndex index=new MusaAiDrawingIndex(
            "Model",12,1,
            Arrays.asList("PIS_SU","HAVALANDIRMA","NOTLAR"),
            Arrays.asList("PIS_SU","HAVALANDIRMA"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",12.5,Double.NaN,false,false,"",5,6),
                new MusaAiDrawingIndex.Item(2,"TEXT","PIS_SU","Ø100 %2",Double.NaN,Double.NaN,false,false,"",8,9),
                new MusaAiDrawingIndex.Item(3,"LWPOLYLINE","HAVALANDIRMA","600x300",22,120,true,false,"",10,11)
            ),"mm"
        );
        String json=MusaAiCadJson.build(index,"mekanik.dwg",2);
        has(json,"\"schema\":\"musacad-cad-json/v1\"");
        has(json,"\"fileName\":\"mekanik.dwg\"");
        has(json,"\"rawDrawingIncluded\":false");
        has(json,"\"automaticEditsAllowed\":false");
        has(json,"\"editActionsRequireUserApproval\":true");
        has(json,"\"sourceId\":1");
        has(json,"\"sourceId\":2");
        has(json,"\"truncated\":true");
        if(json.contains("\"sourceId\":3"))throw new AssertionError("item limit ignored");
        if(json.contains("geometryKey"))throw new AssertionError("internal geometry signature leaked");
        System.out.println("MusaAiCadJsonTest OK");
    }
}
