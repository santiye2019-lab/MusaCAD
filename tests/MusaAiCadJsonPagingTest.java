import com.musa.cad.MusaAiCadJson;
import com.musa.cad.MusaAiDrawingIndex;
import java.util.*;

public final class MusaAiCadJsonPagingTest {
    private static void check(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    public static void main(String[]args){
        ArrayList<MusaAiDrawingIndex.Item> items=new ArrayList<>();
        for(int i=0;i<12001;i++)
            items.add(new MusaAiDrawingIndex.Item(i+1,"TEXT","MEK","item "+(i+1)));
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",12001,0,
            Arrays.asList("MEK"),Arrays.asList("MEK"),items,"mm");
        String legacy=MusaAiCadJson.build(index,"sample.dwg",2);
        check(!legacy.contains("\"pageOffset\""),"legacy JSON schema changed");
        check(legacy.contains("\"sourceId\":1"),"legacy first item missing");
        check(!legacy.contains("\"sourceId\":3"),"legacy limit changed");
        int offset=0;
        for(int page=0;page<3;page++){
            String json=MusaAiCadJson.buildPage(index,"sample.dwg",offset,5000);
            int next=Math.min(12001,offset+5000);
            check(json.contains("\"pageOffset\":"+offset),"wrong offset");
            check(json.contains("\"nextOffset\":"+next),"wrong next offset");
            check(json.contains("\"sourceId\":"+(offset+1)),"first item missing");
            check(json.contains("\"sourceId\":"+next),"last item missing");
            check(json.contains("\"truncated\":"+(next<12001)),"wrong truncation");
            offset=next;
        }
        check(offset==12001,"not all items traversed");
        System.out.println("MusaAiCadJsonPagingTest OK");
    }
}
