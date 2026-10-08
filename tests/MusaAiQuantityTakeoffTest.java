import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiQuantityTakeoff;
import java.util.*;

public final class MusaAiQuantityTakeoffTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",7,0,
            Arrays.asList("PIS_SU","PIS_SU_DN100","SIHHI_CIHAZ","NOTLAR"),
            Arrays.asList("PIS_SU","PIS_SU_DN100","SIHHI_CIHAZ","NOTLAR"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",5,Double.NaN),
                new MusaAiDrawingIndex.Item(2,"LINE","PIS_SU","",7,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"CIRCLE","SIHHI_CIHAZ","",6.283185,3.141593),
                new MusaAiDrawingIndex.Item(4,"CIRCLE","SIHHI_CIHAZ","",12.56637,12.56637),
                new MusaAiDrawingIndex.Item(5,"LWPOLYLINE","PIS_SU","",10,20),
                new MusaAiDrawingIndex.Item(6,"LINE","PIS_SU_DN100","",12,Double.NaN),
                new MusaAiDrawingIndex.Item(7,"TEXT","NOTLAR","LAVABO",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(-1,"BLOCK","SIHHI_CIHAZ","LAVABO_60x50",Double.NaN,Double.NaN,false,false,"",Double.NaN,Double.NaN,3)
            ),"m");
    }
    private static String ask(String q){
        MusaAiQuantityTakeoff.Answer a=MusaAiQuantityTakeoff.answer(index(),q);
        if(!a.matched)throw new AssertionError("not matched: "+q);
        return a.text;
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        has(ask("bu projede metraj çıkar"),"TESİSAT ÖN METRAJ RAPORU");
        has(ask("bu projede metraj çıkar"),"GÜZERGÂH UZUNLUĞU ADAYLARI");
        has(ask("bu projede metraj çıkar"),"Pis su • PIS_SU");
        has(ask("bu projede metraj çıkar"),"22 m");
        has(ask("bu projede metraj çıkar"),"Ø100: 12 m");
        has(ask("bu projede metraj çıkar"),"Lavabo • LAVABO_60x50: 3 adet");
        has(ask("PIS_SU katmanının toplam uzunluğu nedir"),"22");
        has(ask("SIHHI_CIHAZ katmanında kaç daire var"),"Adet: 2");
        has(ask("kapalı polylinelerin toplam alanı"),"20");
        has(ask("lavabo geçenlerin adedi"),"Adet: 3");

        // Unknown AutoCAD dynamic/anonymous blocks and unrelated architecture
        // must never appear as confirmed mechanical material quantities.
        MusaAiDrawingIndex noisy=new MusaAiDrawingIndex("Zemin",12,0,
            Arrays.asList("0","MEK-S-S-SÜZGEÇ1","VRF","KLOZET","MIMARI","PIS_SU_PVC_DN100"),
            Arrays.asList("0","MEK-S-S-SÜZGEÇ1","VRF","KLOZET","MIMARI","PIS_SU_PVC_DN100"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(40,"BLOCK","0","A$Cabad0764",
                    Double.NaN,Double.NaN,false,false,"",0,0,4),
                new MusaAiDrawingIndex.Item(41,"INSERT","MEK-S-S-SÜZGEÇ1","*U184",
                    Double.NaN,Double.NaN,false,false,"",0,0,2),
                new MusaAiDrawingIndex.Item(42,"INSERT","VRF","A$C25DF68C2",
                    Double.NaN,Double.NaN,false,false,"",0,0,2),
                new MusaAiDrawingIndex.Item(43,"BLOCK","KLOZET","Toilet-002",
                    Double.NaN,Double.NaN,false,false,"",0,0,4),
                new MusaAiDrawingIndex.Item(44,"BLOCK","MIMARI","alüminyum ayarlı kapı",
                    Double.NaN,Double.NaN,false,false,"",0,0,2),
                new MusaAiDrawingIndex.Item(45,"LINE","MIMARI","",14000,Double.NaN),
                new MusaAiDrawingIndex.Item(46,"LINE","PIS_SU_PVC_DN100","",
                    12000,Double.NaN)
            ),"mm");
        String report=MusaAiQuantityTakeoff.answer(noisy,"metraj").text;
        has(report,"Klozet • Toilet-002: 4 adet");
        has(report,"Türü tanımlanamayan blok: 6 adet");
        has(report,"Yalnız katman adına göre tahmin edilebilen cihaz bloğu: 4 adet");
        has(report,"12 m");
        if(report.contains("A$Cabad0764:")||report.contains("*U184:")||
           report.contains("alüminyum ayarlı kapı:")||report.contains("14 m"))
            throw new AssertionError("Unnamed/architectural entities leaked into takeoff: "+report);
        has(report,"Onaylı keşif veya resmi poz listesi değildir");

        System.out.println("MusaAiQuantityTakeoffTest OK");
    }
}
