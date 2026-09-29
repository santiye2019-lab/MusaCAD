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
        has(ask("bu projede metraj çıkar"),"Metraj raporu");
        has(ask("bu projede metraj çıkar"),"ÇİZGİSEL METRAJ");
        has(ask("bu projede metraj çıkar"),"Pis su • PIS_SU");
        has(ask("bu projede metraj çıkar"),"22 m");
        has(ask("bu projede metraj çıkar"),"Ø100: 12 m");
        has(ask("bu projede metraj çıkar"),"Lavabo • LAVABO_60x50: 3 adet");
        has(ask("PIS_SU katmanının toplam uzunluğu nedir"),"22");
        has(ask("SIHHI_CIHAZ katmanında kaç daire var"),"Adet: 2");
        has(ask("kapalı polylinelerin toplam alanı"),"20");
        has(ask("lavabo geçenlerin adedi"),"Adet: 3");
        System.out.println("MusaAiQuantityTakeoffTest OK");
    }
}
