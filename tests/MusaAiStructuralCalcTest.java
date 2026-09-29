import com.musa.cad.*;
import java.util.*;

public final class MusaAiStructuralCalcTest {
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}

    public static void main(String[]args)throws Exception{
        String report=
            "BETONARME HESAP RAPORU\n"+
            "Beton sınıfı C30 Donatı B420C\n"+
            "K1 30x70\n"+
            "K2 25/50\n"+
            "RADYE TEMEL\n"+
            "SDS = 1.10 SD1 = 0.55 DTS=1 BYS=3 Zemin Sınıfı ZC\n"+
            "Hareketli yük 5.00 kN/m2\n";

        MusaAiStructuralCalc.Model model=MusaAiStructuralCalc.parse("hesap.pdf",report);
        yes(model.concreteGrades.contains("C30"),"C30 missing");
        yes(model.rebarGrades.contains("B420C"),"B420C missing");
        yes("30x70".equals(model.taggedSections.get("K1")),"K1 section missing");
        yes(model.foundationTypes.contains("RADYE"),"raft missing");
        yes(!model.seismicCues.isEmpty(),"seismic cues missing");

        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Zemin",3,0,
            Arrays.asList("S_KIRIS","S_TEMEL"),
            Arrays.asList("S_KIRIS","S_TEMEL"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(11,"TEXT","S_KIRIS","K1 30x60 C30 B420C",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(12,"TEXT","S_KIRIS","K2 25x50 C30 B420C",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(13,"TEXT","S_TEMEL","RADYE TEMEL C30",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison cmp=MusaAiStructuralCalc.compare(index,model);
        yes(cmp.matched,"comparison not matched");
        yes(cmp.differentTaggedSections==1,"expected one tagged section mismatch");
        yes(cmp.sameTaggedSections==1,"expected one equal tagged section");
        yes(cmp.sourceIds.contains(11),"mismatch source should be highlighted");
        has(cmp.text,"K1: rapor 30x70 • DWG 30x60");
        has(cmp.text,"YÜKSEK");
        has(cmp.text,"RADYE");

        yes(MusaAiStructuralCalc.isEngineeringTextExtension("model.e2k"),"e2k should be accepted");
        System.out.println("MusaAiStructuralCalcTest OK");
    }
}