import com.musa.cad.*;
import java.util.*;

public final class MusaAiDisciplineExpertTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",30,0,
            Arrays.asList("MIM_RAMP","MIM_KAPI","S_KIRIS","S_TEMEL","E_PANO","E_TOPRAK","PEYZAJ_SULAMA","ALTYAPI_ROGAR","ASANSOR_KUYU","ASANSOR_KAPI","YANGIN_ALARM","YANGIN_DUMAN"),
            Arrays.asList("MIM_RAMP","MIM_KAPI","S_KIRIS","S_TEMEL","E_PANO","E_TOPRAK","PEYZAJ_SULAMA","ALTYAPI_ROGAR","ASANSOR_KUYU","ASANSOR_KAPI","YANGIN_ALARM","YANGIN_DUMAN"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","MIM_RAMP","ENGELLİ RAMPASI 1.20 m",8,Double.NaN,false,false,"a1"),
                new MusaAiDrawingIndex.Item(2,"TEXT","MIM_KAPI","KAPI 90 cm MAHAL A101",Double.NaN,Double.NaN,false,false,"a2"),
                new MusaAiDrawingIndex.Item(3,"LINE","S_KIRIS","KİRİŞ REZERVASYON DELİK",6,Double.NaN,false,false,"s1"),
                new MusaAiDrawingIndex.Item(4,"LWPOLYLINE","S_TEMEL","RADYE TEMEL",12,6,true,true,"s2"),
                new MusaAiDrawingIndex.Item(5,"TEXT","E_PANO","ANA PANO KABLO TAVASI POMPA BESLEME",Double.NaN,Double.NaN,false,false,"e1"),
                new MusaAiDrawingIndex.Item(6,"LINE","E_TOPRAK","TOPRAKLAMA",9,Double.NaN,false,false,"e2"),
                new MusaAiDrawingIndex.Item(7,"LWPOLYLINE","PEYZAJ_SULAMA","SULAMA VANA",18,Double.NaN,true,false,"p1"),
                new MusaAiDrawingIndex.Item(8,"TEXT","ALTYAPI_ROGAR","RÖGAR KOT +12.30 EĞİM %1 BAĞLANTI",Double.NaN,Double.NaN,false,false,"i1"),
                new MusaAiDrawingIndex.Item(9,"TEXT","ASANSOR_KUYU","ASANSÖR KUYU 210x220 KUYU DİBİ",Double.NaN,Double.NaN,false,false,"l1"),
                new MusaAiDrawingIndex.Item(10,"TEXT","ASANSOR_KAPI","ASANSÖR KAPI 90 cm ELEKTRİK PANO YANGIN",Double.NaN,Double.NaN,false,false,"l2"),
                new MusaAiDrawingIndex.Item(11,"TEXT","YANGIN_ALARM","YANGIN ALGILAMA DEDEKTÖR ALARM",Double.NaN,Double.NaN,false,false,"f1"),
                new MusaAiDrawingIndex.Item(12,"TEXT","YANGIN_DUMAN","DUMAN BASINÇLANDIRMA FAN",Double.NaN,Double.NaN,false,false,"f2")
            ),"m");
    }
    private static void has(String source,String wanted){if(!source.contains(wanted))throw new AssertionError("missing "+wanted+" in\n"+source);}
    public static void main(String[]args){
        if(MusaAiDisciplineExpert.detect("STATIKAI_OPENINGS")!=MusaAiDisciplineExpert.Profile.STRUCT_OPENINGS)
            throw new AssertionError("struct openings");
        if(MusaAiDisciplineExpert.detect("GELKAI_GROUNDING")!=MusaAiDisciplineExpert.Profile.ELEC_GROUNDING)
            throw new AssertionError("electrical grounding");
        if(!MusaAiDisciplineExpert.isCloudExpertCommand("GYANGAI_SMOKE"))throw new AssertionError("fire cloud");
        if(!"fire_smoke".equals(MusaAiDisciplineExpert.cloudProfile("GYANGAI_SMOKE")))throw new AssertionError("fire profile");
        if(!MusaAiDisciplineExpert.isHelpCommand("STATIKAI_HELP"))throw new AssertionError("help");
        has(MusaAiDisciplineExpert.commandHelp(),"GSTATIKAI_FULL");

        MusaAiDisciplineExpert.Result structural=MusaAiDisciplineExpert.analyze(index(),"STATIKAI_OPENINGS");
        if(!structural.matched)throw new AssertionError("local structural");
        has(structural.text,"STATIKAI Uzman Kontrol");
        has(structural.text,"rezervasyon/delik/geçiş koordinasyonu");
        if(structural.sourceIds.isEmpty())throw new AssertionError("structural highlights");

        MusaAiDisciplineExpert.Result electrical=MusaAiDisciplineExpert.analyze(index(),"ELKAI_POWER");
        if(!electrical.matched)throw new AssertionError("electrical");
        has(electrical.text,"mekanik/asansör ekipman beslemeleri");

        MusaAiDisciplineExpert.Result irrigation=MusaAiDisciplineExpert.analyze(index(),"PEYAI_IRRIGATION");
        if(!irrigation.matched)throw new AssertionError("landscape");
        has(irrigation.text,"Peyzaj • Sulama");

        MusaAiDisciplineExpert.Result infra=MusaAiDisciplineExpert.analyze(index(),"ALTYAPIAI_LEVELS");
        if(!infra.matched)throw new AssertionError("infra");
        has(infra.text,"Altyapı • Kot / eğim / rögar");
        if(infra.text.contains("görünür kot/level bilgisi bulunamadı"))throw new AssertionError("level should be detected");

        MusaAiDisciplineExpert.Result elevator=MusaAiDisciplineExpert.analyze(index(),"ASNAI_FULL");
        if(!elevator.matched)throw new AssertionError("elevator");
        has(elevator.text,"Asansör • Tüm proje");

        MusaAiDisciplineExpert.Result fire=MusaAiDisciplineExpert.analyze(index(),"YANGAI_DETECTION");
        if(!fire.matched)throw new AssertionError("fire");
        has(fire.text,"Yangın • Algılama / ihbar");

        if(MusaAiDisciplineExpert.analyze(index(),"GSTATIKAI_FULL").matched)
            throw new AssertionError("cloud command must not run local");
        if(MusaAiDisciplineExpert.analyze(index(),"statik projeyi kontrol et").matched)
            throw new AssertionError("ordinary query must remain generic");

        System.out.println("MusaAiDisciplineExpertTest OK");
    }
}
