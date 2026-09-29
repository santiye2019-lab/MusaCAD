import com.musa.cad.*;
import java.util.*;

public final class MusaAiStructuralAdvancedTest {
    public static void main(String[] args){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            new MusaAiDrawingIndex.Item(1,"TEXT","STATIK","PERDE P08 ZEMIN KAT AKS C/7 25x350"),
            new MusaAiDrawingIndex.Item(2,"TEXT","STATIK","KIRIS K01 1. KAT AKS A/1 30x60"),
            new MusaAiDrawingIndex.Item(3,"TEXT","STATIK","PERDE P08 2. KAT AKS D/7 25x180"),
            new MusaAiDrawingIndex.Item(4,"TEXT","STATIK","KIRIS K145 REZERVASYON R01 20x30 CM"),
            new MusaAiDrawingIndex.Item(5,"TEXT","STATIK","ASANSOR KUYU 220x250 CM"),
            new MusaAiDrawingIndex.Item(6,"TEXT","MIMARI","ASANSOR KUYU 210x250 CM"),
            new MusaAiDrawingIndex.Item(7,"TEXT","STATIK","MERDIVEN M02"),
            new MusaAiDrawingIndex.Item(8,"TEXT","STATIK","5 CM DILATASYON"),
            new MusaAiDrawingIndex.Item(9,"TEXT","STATIK","KONSOL K20"),
            new MusaAiDrawingIndex.Item(10,"TEXT","STATIK","TRANSFER KIRIS K30"),
            new MusaAiDrawingIndex.Item(11,"TEXT","STATIK","DOSEME D08")
        );
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("KALIP",items.size(),0,
            Arrays.asList("STATIK","MIMARI"),Arrays.asList("STATIK","MIMARI"),items,"cm");

        MusaAiStructuralCalc.Model calc=MusaAiStructuralCalc.parse("hesap.txt","C35 B420C SDS=1.10");
        MusaAiStructuralAdvanced.Result result=MusaAiStructuralAdvanced.analyze(index,calc);
        require(result.matched,"advanced structural result must match");
        Set<String> ids=new LinkedHashSet<>();
        for(MusaAiStructuralAdvanced.Finding f:result.findings)ids.add(f.id);

        require(ids.contains("ST-03"),"wall continuity candidate missing");
        require(ids.contains("ST-04"),"axis-change candidate missing");
        require(ids.contains("ST-05"),"section-change candidate missing");
        require(ids.contains("ST-08"),"beam reservation candidate missing");
        require(ids.contains("ST-10"),"elevator coordination candidate missing");
        require(ids.contains("ST-11"),"stair coordination candidate missing");
        require(ids.contains("ST-12"),"dilatation review candidate missing");
        require(ids.contains("ST-16"),"cantilever detail verification missing");
        require(ids.contains("ST-17"),"transfer-system review missing");
        require(ids.contains("ST-18"),"slab thickness verification missing");
        require(ids.contains("ST-20"),"seismic parameter verification missing");
        require(ids.contains("ST-21"),"load assumption verification missing");

        require(result.text.contains("DOĞRULANAMADI"),"status taxonomy missing");
        require(result.text.contains("hesap sonucu"),"conservative safety note missing");
        System.out.println("MusaAiStructuralAdvancedTest OK");
    }

    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
}
