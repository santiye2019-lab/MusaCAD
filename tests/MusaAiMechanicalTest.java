import com.musa.cad.*;
import java.util.*;

public final class MusaAiMechanicalTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",10,0,
            Arrays.asList("M_PIS_SU","M_SPRINKLER","M_VRF","M_HAVALANDIRMA","M_DOGALGAZ"),
            Arrays.asList("M_PIS_SU","M_SPRINKLER","M_VRF","M_HAVALANDIRMA","M_DOGALGAZ"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","M_PIS_SU","",12,Double.NaN,true,false,"P1"),
                new MusaAiDrawingIndex.Item(2,"TEXT","M_PIS_SU","PİS SU KOLONU",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"LINE","M_SPRINKLER","",20,Double.NaN,false,false,"F1"),
                new MusaAiDrawingIndex.Item(4,"TEXT","M_SPRINKLER","REVİZE ET",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(5,"INSERT","M_VRF","VRF-01",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(6,"LINE","M_HAVALANDIRMA","",30,Double.NaN,false,false,"V1"),
                new MusaAiDrawingIndex.Item(7,"TEXT","M_HAVALANDIRMA","TAZE HAVA",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(8,"LINE","M_DOGALGAZ","",15,Double.NaN,false,false,"G1")
            ),"m");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiMechanical.Result a=MusaAiMechanical.analyze(index(),"mekanik tesisat özeti");
        if(!a.matched)throw new AssertionError("overview not matched");
        has(a.text,"Pis / atık su");has(a.text,"Yangın / sprinkler");has(a.text,"VRF / klima / soğutma");has(a.text,"Havalandırma");has(a.text,"Doğalgaz");

        MusaAiMechanical.Result waste=MusaAiMechanical.analyze(index(),"pis su tesisatını kontrol et");
        has(waste.text,"Açık polyline: 1");
        if(waste.issueCount<1||!waste.sourceIds.contains(1))throw new AssertionError("waste control");

        MusaAiMechanical.Result fire=MusaAiMechanical.analyze(index(),"yangın tesisatı metrajı");
        has(fire.text,"20");has(fire.text,"Yangın / sprinkler");

        MusaAiMechanical.Result vrf=MusaAiMechanical.analyze(index(),"VRF cihazlarını say");
        has(vrf.text,"Algılanan nesne: 1");

        MusaAiMechanical.Result none=MusaAiMechanical.analyze(index(),"kaç daire var");
        if(none.matched)throw new AssertionError("unrelated matched");

        String report=MusaAiMechanical.reportSection(index());
        has(report,"Algılanan mekanik sistem");has(report,"Havalandırma");
        System.out.println("MusaAiMechanicalTest OK");
    }
}
