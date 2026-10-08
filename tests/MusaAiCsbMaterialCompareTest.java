import com.musa.cad.*;
import java.util.*;

public final class MusaAiCsbMaterialCompareTest {
    private static void check(boolean b,String text){if(!b)throw new AssertionError(text);}
    private static MusaAiDrawingIndex.Item pipe(int id,String layer,double length){
        return new MusaAiDrawingIndex.Item(id,"LINE",layer,"",length,Double.NaN);
    }
    private static MusaAiDrawingIndex.Item block(int id,String text,int qty){
        return new MusaAiDrawingIndex.Item(id,"BLOCK","SIHHI_CIHAZ",text,
            Double.NaN,Double.NaN,false,false,"",50,50,qty);
    }
    public static void main(String[]args){
        MusaAiDrawingIndex dwg=new MusaAiDrawingIndex("Model",5,0,
            Arrays.asList("PIS_SU_PVC_DN100","TEMIZ_SU_PPRC_DN25","SIHHI_CIHAZ"),
            Arrays.asList("PIS_SU_PVC_DN100","TEMIZ_SU_PPRC_DN25","SIHHI_CIHAZ"),
            Arrays.asList(
                pipe(1,"PIS_SU_PVC_DN100",10000),
                pipe(2,"PIS_SU_PVC_DN100",15000),
                pipe(3,"TEMIZ_SU_PPRC_DN25",5000),
                block(4,"Lavabo",3)
            ),"mm");
        MusaAiCsbEstimate.Result measured=MusaAiCsbEstimate.analyze(dwg,Collections.emptyList());
        check(measured.rows.size()==3,"pipe and fixture groups exist");
        MusaAiBoq.Model schedule=MusaAiBoq.of("Mekanik keşif ve malzeme listesi",Arrays.asList(
            new MusaAiBoq.Row("TEST-PVC-100","PVC pis su borusu DN100","m",20d,"liste A"),
            new MusaAiBoq.Row("TEST-PPR-25","PPRC temiz su borusu DN25","m",5d,"liste A"),
            new MusaAiBoq.Row("TEST-LAVABO","Seramik lavabo","Ad",3d,"liste B"),
            new MusaAiBoq.Row("TEST-UNMATCH","Pis su PE DN150","m",6d,"liste C")
        ));
        MusaAiCsbMaterialCompare.Result comparison=MusaAiCsbMaterialCompare.compare(schedule,measured);
        check(comparison.matched==3,"exact DN/material/system and unit matching");
        check(comparison.differences==1,"20 m schedule vs 25 m measured");
        check(comparison.unmatchedSchedule==1,"unavailable PE DN150 excluded");
        check(comparison.unlistedDrawing==0,"matched DWG group count");
        check(comparison.report.contains("fark 5 m"),"delta and units reported");
        check(comparison.report.contains("[kaynak 1]"),"source linkage to drawing");
        MusaAiBoq.Model unsafe=MusaAiBoq.of("Mismatched",Arrays.asList(
            new MusaAiBoq.Row("","PVC temiz su borusu DN100","m",25,""),
            new MusaAiBoq.Row("","PVC pis su borusu DN100","Ad",25,""),
            new MusaAiBoq.Row("","PVC pis su borusu DN110","m",25,""),
            new MusaAiBoq.Row("","PVC pis su borusu DN100","m",-5,"")
        ));
        MusaAiCsbMaterialCompare.Result noMatch=MusaAiCsbMaterialCompare.compare(unsafe,measured);
        check(noMatch.matched==0&&noMatch.unmatchedSchedule==4,
            "wrong system, unit, DN and negative values never matched");
        MusaAiBoq.Model duplicate=MusaAiBoq.of("repeated",Arrays.asList(
            new MusaAiBoq.Row("","PVC pis su borusu DN100","m",25,""),
            new MusaAiBoq.Row("","PVC pis su borusu DN100","m",25,"")
        ));
        MusaAiCsbMaterialCompare.Result same=MusaAiCsbMaterialCompare.compare(duplicate,measured);
        check(same.matched==1&&same.ambiguous==1,
            "duplicate list line not counted as two independent matched quantities");
        check(MusaAiCsbMaterialCompare.compare(null,measured).report.contains("Keşif yükle"),
            "missing schedule must not imply verification");
        System.out.println("MusaAiCsbMaterialCompareTest OK");
    }
}
