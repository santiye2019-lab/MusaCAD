import com.musa.cad.MusaAiCsbEstimate;
import com.musa.cad.MusaAiDrawingIndex;
import java.util.*;

public final class MusaAiCsbEstimateTest {
    private static void check(boolean b,String s){if(!b)throw new AssertionError(s);}
    private static MusaAiDrawingIndex.Item pipe(int id,String layer,double length) {
        return new MusaAiDrawingIndex.Item(id,"LINE",layer,"",length,Double.NaN);
    }
    public static void main(String[]args){
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",10,0,
            Arrays.asList("PIS_SU_PVC_DN100","TEMIZ_SU_PPRC_DN25","SIHHI_CIHAZ","NOT"),
            Arrays.asList("PIS_SU_PVC_DN100","TEMIZ_SU_PPRC_DN25","SIHHI_CIHAZ","NOT"),
            Arrays.asList(
                pipe(10,"PIS_SU_PVC_DN100",12000),
                pipe(11,"PIS_SU_PVC_DN100",8000),
                pipe(11,"PIS_SU_PVC_DN100",8000),
                pipe(20,"TEMIZ_SU_PPRC_DN25",16000),
                new MusaAiDrawingIndex.Item(30,"BLOCK","SIHHI_CIHAZ","Lavabo",
                    Double.NaN,Double.NaN,false,false,"",100,200,3),
                new MusaAiDrawingIndex.Item(31,"TEXT","NOT","DN100 - görünüş etiketi",
                    Double.NaN,Double.NaN),
                pipe(40,"NOT",10000)
            ),"mm");
        MusaAiCsbEstimate.Result raw=MusaAiCsbEstimate.analyze(index,Collections.emptyList());
        check(raw.rows.size()==3,"two pipe groups and one fixture");
        check(raw.report.contains("20 m"),"metric conversion / no duplicated source");
        check(raw.report.contains("16 m"),"clean water pipe measured");
        check(raw.report.contains("Lavabo = 3 Ad"),"countable block");
        check(raw.unpricedRows==3&&Double.isNaN(raw.completeTotal),"no fabricated official prices");
        check(raw.report.contains("yinelenen kaynak"),"source duplicate warning");
        check(raw.report.contains("GENEL TOPLAM HESAPLANMADI"),"partial pricing cannot imply full estimate");
        String key=null;
        for(MusaAiCsbEstimate.Row row:raw.rows)if(row.description.contains("DN100"))key=row.itemKey;
        check(key!=null,"pipe technical key generated");
        MusaAiCsbEstimate.Rate fixtureWrong=new MusaAiCsbEstimate.Rate(
            key,"FAKE.001","invalid price without endorsement","m",500,"2026-10","CSB",false);
        MusaAiCsbEstimate.Rate confirmed=new MusaAiCsbEstimate.Rate(
            key,"TEST-ONLY-POS","Example installed sanitary pipe","m",500,
            "2026-10","Synthetic test catalog, not official",true);
        MusaAiCsbEstimate.Result prices=MusaAiCsbEstimate.analyze(index,Arrays.asList(
            fixtureWrong,confirmed));
        check(prices.pricedRows==1,"only one specific verified rate");
        check(Math.abs(prices.pricedSubtotal-10000)<0.001,"20 m * illustrative unit rate");
        check(Double.isNaN(prices.completeTotal),"total withheld while other line prices missing");
        MusaAiCsbEstimate.Rate wrongUnit=new MusaAiCsbEstimate.Rate(
            key,"TEST-MISMATCH","Wrong meter vs per-piece", "Ad",800,
            "2026-10","synthetic",true);
        check(MusaAiCsbEstimate.analyze(index,Arrays.asList(wrongUnit)).pricedRows==0,
            "wrong measurement unit must not price");
        MusaAiDrawingIndex unitless=new MusaAiDrawingIndex("Model",1,0,
            Arrays.asList("PIS_SU_PVC_DN100"),Arrays.asList("PIS_SU_PVC_DN100"),
            Arrays.asList(pipe(100,"PIS_SU_PVC_DN100",20000)),"");
        MusaAiCsbEstimate.Result missing=MusaAiCsbEstimate.analyze(unitless,Collections.emptyList());
        check(missing.rows.isEmpty()&&missing.report.contains("birimi/ölçek"),
            "unknown units cannot produce meters or prices");
        System.out.println("MusaAiCsbEstimateTest OK");
    }
}
