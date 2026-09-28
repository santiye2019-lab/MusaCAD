import com.musa.cad.*;
import java.util.*;

public final class MusaAiEstimateTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",4,0,
            Arrays.asList("PIS_SU","E_KABLO","S_KOLON"),Arrays.asList("PIS_SU","E_KABLO","S_KOLON"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",10,Double.NaN),
                new MusaAiDrawingIndex.Item(2,"LINE","PIS_SU","",5,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"LINE","E_KABLO","",20,Double.NaN),
                new MusaAiDrawingIndex.Item(4,"LWPOLYLINE","S_KOLON","KOLON",8,2)
            ),"m");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        String xlsx="— Sayfa 1 —\nA1\tPoz No\nB1\tAçıklama\nC1\tBirim\nD1\tMiktar\nA2\t25.1\nB2\tPIS_SU\nC2\tm\nD2\t12\nA3\t26.1\nB3\tE_KABLO\nC3\tm\nD3\t20";
        MusaAiEstimate.Document loaded=MusaAiEstimate.parseExtracted("kesif.xlsx","XLSX",xlsx);
        if(loaded.items.size()!=2)throw new AssertionError("parse count "+loaded.items.size());

        MusaAiEstimate.Document draft=MusaAiEstimate.fromDrawing(index(),"proje.dwg");
        if(draft.items.size()<3)throw new AssertionError("draft count "+draft.items.size());

        MusaAiEstimate.CompareResult c=MusaAiEstimate.compare(draft,loaded);
        has(c.text,"PROJE–KEŞİF UYGUNLUK KONTROLÜ");
        if(c.quantityDiff<1)throw new AssertionError("expected quantity diff");
        if(c.missing<1)throw new AssertionError("expected missing item");
        if(!MusaAiEstimate.isLoadCommand("Keşif yükle"))throw new AssertionError("load command");
        if(!MusaAiEstimate.isBuildCommand("Projeden keşif oluştur"))throw new AssertionError("build command");
        if(!MusaAiEstimate.isCompareCommand("Keşif karşılaştır"))throw new AssertionError("compare command");
        System.out.println("MusaAiEstimateTest OK");
    }
}
