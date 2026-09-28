import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiMechanicalControl;
import java.util.*;

public final class MusaAiMechanicalControlTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",20,0,
            Arrays.asList("PIS_SU","TEMIZ_SU","HAVALANDIRMA","YANGIN","DOGALGAZ","NOTLAR"),
            Arrays.asList("PIS_SU","TEMIZ_SU","HAVALANDIRMA","YANGIN","DOGALGAZ","NOTLAR"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",10,Double.NaN,false,false,"g1"),
                new MusaAiDrawingIndex.Item(2,"LWPOLYLINE","PIS_SU","",12,Double.NaN,true,false,"g2"),
                new MusaAiDrawingIndex.Item(3,"TEXT","PIS_SU","Ø100",Double.NaN,Double.NaN,false,false,"g3"),
                new MusaAiDrawingIndex.Item(4,"TEXT","PIS_SU","TODO eğim kontrol",Double.NaN,Double.NaN,false,false,"g4"),

                new MusaAiDrawingIndex.Item(5,"LINE","TEMIZ_SU","",8,Double.NaN,false,false,"g5"),
                new MusaAiDrawingIndex.Item(6,"TEXT","TEMIZ_SU","DN25",Double.NaN,Double.NaN,false,false,"g6"),

                new MusaAiDrawingIndex.Item(7,"LINE","HAVALANDIRMA","",20,Double.NaN,false,false,"g7"),
                new MusaAiDrawingIndex.Item(8,"TEXT","HAVALANDIRMA","600x300 menfez",Double.NaN,Double.NaN,false,false,"g8"),

                new MusaAiDrawingIndex.Item(9,"LINE","YANGIN","",15,Double.NaN,false,false,"g9"),
                new MusaAiDrawingIndex.Item(10,"TEXT","YANGIN","SPRINKLER DN80",Double.NaN,Double.NaN,false,false,"g10"),

                new MusaAiDrawingIndex.Item(11,"LINE","DOGALGAZ","",9,Double.NaN,false,false,"g11"),
                new MusaAiDrawingIndex.Item(12,"LINE","DOGALGAZ","",0,Double.NaN,false,false,"g12"),
                new MusaAiDrawingIndex.Item(13,"TEXT","DOGALGAZ","DN32",Double.NaN,Double.NaN,false,false,"g13")
            ),"m");
    }

    private static void has(String source,String wanted){
        if(!source.contains(wanted))throw new AssertionError("missing "+wanted+" in\n"+source);
    }

    public static void main(String[]args){
        MusaAiMechanicalControl.Result full=MusaAiMechanicalControl.analyze(index(),"AI_MEKANIK_KONTROL");
        if(!full.matched)throw new AssertionError("full mechanical command not matched");
        has(full.text,"Mekanik tesisat AI kontrolü");
        has(full.text,"Pis su / atık su");
        has(full.text,"Temiz / sıcak-soğuk su");
        has(full.text,"Havalandırma");
        has(full.text,"Yangın / sprinkler");
        has(full.text,"Doğalgaz");
        has(full.text,"Açık hat polyline: 1");
        has(full.text,"Sıfır uzunluk/dejenere mekanik geometri: 1");
        has(full.text,"TODO / EKSİK / REVİZE benzeri mekanik not: 1");
        has(full.text,"Yangın sistemi: pompa/depo/jokey/hidrant/itfaiye bağlantısı etiketi bulunamadı");
        has(full.text,"Doğalgaz sistemi: vana/regülatör/sayaç etiketi bulunamadı");
        if(full.findingCount<5)throw new AssertionError("too few findings: "+full.findingCount);
        if(!full.sourceIds.contains(2)||!full.sourceIds.contains(4)||!full.sourceIds.contains(12))
            throw new AssertionError("expected highlight ids missing: "+full.sourceIds);

        MusaAiMechanicalControl.Result fire=MusaAiMechanicalControl.analyze(index(),"yangın tesisatını kontrol et");
        if(!fire.matched)throw new AssertionError("fire control not matched");
        has(fire.text,"Yangın / sprinkler");
        if(fire.text.contains("Doğalgaz:"))throw new AssertionError("fire mode leaked gas section");

        MusaAiMechanicalControl.Result mekai=MusaAiMechanicalControl.analyze(index(),"MEKAI");
        if(!mekai.matched)throw new AssertionError("MEKAI alias not matched");

        if(MusaAiMechanicalControl.analyze(index(),"kaç daire var").matched)
            throw new AssertionError("unrelated query matched");

        System.out.println("MusaAiMechanicalControlTest OK");
    }
}
