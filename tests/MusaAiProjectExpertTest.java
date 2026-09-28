import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiProjectExpert;
import java.util.*;

public final class MusaAiProjectExpertTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",50,0,
            Arrays.asList("MIMARI_DUVAR","MIMARI_WC","STATIK_KOLON","STATIK_KIRIS","MEKANIK_PIS_SU"),
            Arrays.asList("MIMARI_DUVAR","MIMARI_WC","STATIK_KOLON","STATIK_KIRIS","MEKANIK_PIS_SU"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","MIMARI_DUVAR","DUVAR",10,Double.NaN,false,false,"same-1"),
                new MusaAiDrawingIndex.Item(2,"TEXT","MIMARI_WC","WC",Double.NaN,Double.NaN,false,false,"a2"),
                new MusaAiDrawingIndex.Item(3,"LINE","STATIK_KOLON","KOLON",5,Double.NaN,false,false,"same-1"),
                new MusaAiDrawingIndex.Item(4,"LINE","STATIK_KIRIS","KIRIS",8,Double.NaN,false,false,"s4"),
                new MusaAiDrawingIndex.Item(5,"LINE","MEKANIK_PIS_SU","DN100",12,Double.NaN,false,false,"same-2"),
                new MusaAiDrawingIndex.Item(6,"LINE","STATIK_KIRIS","",12,Double.NaN,false,false,"same-2"),
                new MusaAiDrawingIndex.Item(7,"TEXT","MIMARI_DUVAR","TODO kapı kontrol",Double.NaN,Double.NaN,false,false,"a7"),
                new MusaAiDrawingIndex.Item(8,"TEXT","STATIK_KOLON","Ø12/20",Double.NaN,Double.NaN,false,false,"s8")
            ),"m");
    }

    private static void has(String source,String wanted){
        if(!source.contains(wanted))throw new AssertionError("missing "+wanted+" in\n"+source);
    }

    public static void main(String[]args){
        if(MusaAiProjectExpert.detect("ARKAI_FULL")!=MusaAiProjectExpert.Profile.ARCHITECTURE)
            throw new AssertionError("ARKAI profile");
        if(MusaAiProjectExpert.detect("GSTATIKAI_FULL")!=MusaAiProjectExpert.Profile.STRUCTURAL)
            throw new AssertionError("GSTATIKAI profile");
        if(MusaAiProjectExpert.detect("PROJAI_COORD")!=MusaAiProjectExpert.Profile.COORDINATION)
            throw new AssertionError("PROJAI coordination profile");
        if(!"project_full".equals(MusaAiProjectExpert.cloudProfile("GPROJAI_FULL")))
            throw new AssertionError("project cloud profile");
        if(!MusaAiProjectExpert.isCloudExpertCommand("GARKAI_FULL"))
            throw new AssertionError("GARKAI cloud route");
        if(!MusaAiProjectExpert.isHelpCommand("PROJAI_HELP"))
            throw new AssertionError("PROJAI help");

        MusaAiProjectExpert.Result arch=MusaAiProjectExpert.analyze(index(),"ARKAI_FULL");
        if(!arch.matched)throw new AssertionError("architecture expert");
        has(arch.text,"Mimari kontrol");
        has(arch.text,"Mahal/oda");
        has(arch.text,"şaft");

        MusaAiProjectExpert.Result structural=MusaAiProjectExpert.analyze(index(),"STATIKAI_FULL");
        if(!structural.matched)throw new AssertionError("structural expert");
        has(structural.text,"Statik kontrol");
        has(structural.text,"aks/grid");
        has(structural.text,"kesit/ebat");
        if(structural.text.contains("görünür donatı/etriye referansı bulunamadı"))
            throw new AssertionError("Ø reinforcement marker should be detected");

        MusaAiProjectExpert.Result coordination=MusaAiProjectExpert.analyze(index(),"PROJAI_COORD");
        if(!coordination.matched)throw new AssertionError("coordination expert");
        has(coordination.text,"Koordinasyon kontrolü");
        has(coordination.text,"birebir aynı geometri");

        MusaAiProjectExpert.Result full=MusaAiProjectExpert.analyze(index(),"PROJAI_FULL");
        if(!full.matched)throw new AssertionError("full project expert");
        has(full.text,"Mimari kontrol");
        has(full.text,"Statik kontrol");
        has(full.text,"Mekanik uzman alt raporu");

        if(MusaAiProjectExpert.analyze(index(),"GPROJAI_FULL").matched)
            throw new AssertionError("cloud command must not execute local expert");

        System.out.println("MusaAiProjectExpertTest OK");
    }
}
