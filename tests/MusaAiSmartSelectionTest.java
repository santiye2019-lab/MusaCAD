import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiSmartSelection;
import java.util.*;

public final class MusaAiSmartSelectionTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",8,0,
            Arrays.asList("PIS_SU","SIHHI_CIHAZ","NOTLAR"),
            Arrays.asList("PIS_SU","SIHHI_CIHAZ","NOTLAR"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"CIRCLE","SIHHI_CIHAZ",""),
                new MusaAiDrawingIndex.Item(2,"CIRCLE","SIHHI_CIHAZ",""),
                new MusaAiDrawingIndex.Item(3,"LINE","PIS_SU","Ø100 PİS SU"),
                new MusaAiDrawingIndex.Item(4,"LINE","PIS_SU",""),
                new MusaAiDrawingIndex.Item(5,"TEXT","NOTLAR","LAVABO"),
                new MusaAiDrawingIndex.Item(-1,"TEXT","NOTLAR","LAVABO BLOK İÇİ"),
                new MusaAiDrawingIndex.Item(6,"LWPOLYLINE","PIS_SU",""),
                new MusaAiDrawingIndex.Item(7,"HATCH","PIS_SU","")
            ));
    }
    private static void expect(String q,String type,String layer,int total,int selectable){
        MusaAiSmartSelection.Result r=MusaAiSmartSelection.plan(index(),q,type,layer);
        if(!r.matched)throw new AssertionError("not matched: "+q);
        if(r.totalMatches!=total)throw new AssertionError(q+" total "+r.totalMatches+" != "+total);
        if(r.sourceIds.size()!=selectable)throw new AssertionError(q+" selectable "+r.sourceIds.size()+" != "+selectable);
    }
    public static void main(String[]args){
        expect("tüm daireleri seç",null,null,2,2);
        expect("PIS_SU katmanındakileri seç",null,null,4,4);
        expect("lavabo geçenleri vurgula",null,null,2,1);
        expect("bununla aynı olanları seç","CIRCLE","SIHHI_CIHAZ",2,2);
        expect("aynı türdekileri seç","LINE","PIS_SU",2,2);
        MusaAiSmartSelection.Result missing=MusaAiSmartSelection.plan(index(),"bununla aynı olanları seç",null,null);
        if(!missing.matched||!missing.description.startsWith("Önce "))throw new AssertionError("missing reference guidance");
        System.out.println("MusaAiSmartSelectionTest OK");
    }
}
