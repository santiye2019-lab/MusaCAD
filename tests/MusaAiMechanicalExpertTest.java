import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiMechanicalExpert;
import java.util.*;

public final class MusaAiMechanicalExpertTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",40,0,
            Arrays.asList("PIS_SU","YAGMUR","TEMIZ_SU","ISITMA","VRF","HAVALANDIRMA","YANGIN","DOGALGAZ","MEKANIK_EKIPMAN"),
            Arrays.asList("PIS_SU","YAGMUR","TEMIZ_SU","ISITMA","VRF","HAVALANDIRMA","YANGIN","DOGALGAZ","MEKANIK_EKIPMAN"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","PIS_SU","DN100",12,Double.NaN,true,false,"w1"),
                new MusaAiDrawingIndex.Item(2,"TEXT","PIS_SU","eğim %2 havalık temizleme",Double.NaN,Double.NaN,false,false,"w2"),
                new MusaAiDrawingIndex.Item(3,"LINE","YAGMUR","DN100",8,Double.NaN,false,false,"r1"),
                new MusaAiDrawingIndex.Item(4,"TEXT","YAGMUR","çatı süzgeci ve overflow",Double.NaN,Double.NaN,false,false,"r2"),
                new MusaAiDrawingIndex.Item(5,"LINE","TEMIZ_SU","DN25",10,Double.NaN,false,false,"d1"),
                new MusaAiDrawingIndex.Item(6,"TEXT","TEMIZ_SU","vana hidrofor sayaç",Double.NaN,Double.NaN,false,false,"d2"),
                new MusaAiDrawingIndex.Item(7,"LINE","ISITMA","DN32",15,Double.NaN,false,false,"h1"),
                new MusaAiDrawingIndex.Item(8,"TEXT","ISITMA","gidiş dönüş balans vanası kazan pompa kollektör",Double.NaN,Double.NaN,false,false,"h2"),
                new MusaAiDrawingIndex.Item(9,"LINE","VRF","",18,Double.NaN,false,false,"c1"),
                new MusaAiDrawingIndex.Item(10,"TEXT","VRF","VRF dış ünite iç ünite refnet kondens drenaj",Double.NaN,Double.NaN,false,false,"c2"),
                new MusaAiDrawingIndex.Item(11,"LINE","HAVALANDIRMA","",20,Double.NaN,false,false,"v1"),
                new MusaAiDrawingIndex.Item(12,"TEXT","HAVALANDIRMA","600x300 13500 m3/h menfez AHU damper taze hava",Double.NaN,Double.NaN,false,false,"v2"),
                new MusaAiDrawingIndex.Item(13,"LINE","YANGIN","DN80",25,Double.NaN,false,false,"f1"),
                new MusaAiDrawingIndex.Item(14,"TEXT","YANGIN","sprinkler hidrant pompa jokey depo itfaiye bağlantısı alarm vana test drenaj",Double.NaN,Double.NaN,false,false,"f2"),
                new MusaAiDrawingIndex.Item(15,"LINE","DOGALGAZ","DN32",9,Double.NaN,false,false,"g1"),
                new MusaAiDrawingIndex.Item(16,"TEXT","DOGALGAZ","vana sayaç regülatör solenoid gaz dedektör",Double.NaN,Double.NaN,false,false,"g2"),
                new MusaAiDrawingIndex.Item(17,"TEXT","MEKANIK_EKIPMAN","P-01 pompa 20 m3/h 5.5 kW 300 kPa",Double.NaN,Double.NaN,false,false,"e1")
            ),"m");
    }

    private static void has(String source,String wanted){
        if(!source.contains(wanted))throw new AssertionError("missing "+wanted+" in\n"+source);
    }

    public static void main(String[]args){
        if(MusaAiMechanicalExpert.detect("MEKAI_FIRE")!=MusaAiMechanicalExpert.Profile.FIRE)
            throw new AssertionError("MEKAI_FIRE profile");
        if(MusaAiMechanicalExpert.detect("GMEKAI_VRF projeyi incele")!=MusaAiMechanicalExpert.Profile.COOLING)
            throw new AssertionError("GMEKAI_VRF profile");
        if(!MusaAiMechanicalExpert.isCloudExpertCommand("GMEKAI_VENT"))
            throw new AssertionError("GMEKAI must route cloud");
        if(!"ventilation".equals(MusaAiMechanicalExpert.cloudProfile("GMEKAI_VENT")))
            throw new AssertionError("cloud profile");

        MusaAiMechanicalExpert.Result fire=MusaAiMechanicalExpert.analyze(index(),"MEKAI_FIRE");
        if(!fire.matched||fire.profile!=MusaAiMechanicalExpert.Profile.FIRE)throw new AssertionError("fire expert");
        has(fire.text,"MEKAI Uzman Kontrol");
        has(fire.text,"Yangın / sprinkler");
        has(fire.text,"Sprinkler/hidrant/dolap");
        if(fire.text.contains("itfaiye bağlantı ağzı/FDC referansı görünmüyor"))
            throw new AssertionError("existing fire department marker not detected");

        MusaAiMechanicalExpert.Result vent=MusaAiMechanicalExpert.analyze(index(),"MEKAI_VENT");
        if(!vent.matched)throw new AssertionError("vent expert");
        has(vent.text,"Havalandırma");
        if(vent.text.contains("görünür kanal ölçüsü"))throw new AssertionError("duct size should be detected");
        if(vent.text.contains("görünür debi etiketi"))throw new AssertionError("airflow should be detected");

        MusaAiMechanicalExpert.Result vrf=MusaAiMechanicalExpert.analyze(index(),"MEKAI_VRF");
        if(!vrf.matched)throw new AssertionError("vrf expert");
        if(vrf.text.contains("kondens drenaj referansı görünmüyor"))throw new AssertionError("drain marker should be detected");
        if(vrf.text.contains("branşman/refnet"))throw new AssertionError("refnet should be detected");

        MusaAiMechanicalExpert.Result full=MusaAiMechanicalExpert.analyze(index(),"MEKAI_FULL");
        if(!full.matched)throw new AssertionError("full expert");
        has(full.text,"Tüm mekanik tesisat");
        has(full.text,"Pis su / atık su");
        has(full.text,"Doğalgaz");

        if(MusaAiMechanicalExpert.analyze(index(),"GMEKAI_FIRE").matched)
            throw new AssertionError("cloud command must not execute local expert directly");
        if(MusaAiMechanicalExpert.analyze(index(),"yangın kontrol").matched)
            throw new AssertionError("ordinary query must stay with existing mechanical control");

        System.out.println("MusaAiMechanicalExpertTest OK");
    }
}
