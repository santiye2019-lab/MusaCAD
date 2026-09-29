import com.musa.cad.*;
import java.util.*;

public final class MusaAiStructuralCalcTest {
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}

    public static void main(String[]args)throws Exception{
        String report=
            "BETONARME HESAP RAPORU\n"+
            "Beton sınıfı C30 Donatı B420C\n"+
            "K1 30x70 Ø16\n"+
            "K2 25/50 Ø12\n"+
            "RADYE TEMEL\n"+
            "SDS = 1.10 SD1 = 0.55 DTS=1 BYS=3 Zemin Sınıfı ZC\n"+
            "Hareketli yük 5.00 kN/m2\n";

        MusaAiStructuralCalc.Model model=MusaAiStructuralCalc.parse("hesap.pdf",report);
        yes(model.concreteGrades.contains("C30"),"C30 missing");
        yes(model.rebarGrades.contains("B420C"),"B420C missing");
        yes("30x70".equals(model.taggedSections.get("K1")),"K1 section missing");
        yes(model.foundationTypes.contains("RADYE"),"raft missing");
        yes(model.rebarDiameters.contains("Ø16")&&model.rebarDiameters.contains("Ø12"),"rebar diameters missing");
        yes("1.10".equals(model.designParameters.get("SDS")),"SDS missing");
        yes("0.55".equals(model.designParameters.get("SD1")),"SD1 missing");
        yes("1".equals(model.designParameters.get("DTS")),"DTS missing");
        yes("3".equals(model.designParameters.get("BYS")),"BYS missing");
        yes("ZC".equals(model.designParameters.get("Zemin Sınıfı")),"soil class missing");
        yes(!model.seismicCues.isEmpty(),"seismic cues missing");
        yes(model.elements.size()==2,"K1/K2 element rows should be parsed");

        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Zemin",3,0,
            Arrays.asList("S_KIRIS","S_TEMEL"),
            Arrays.asList("S_KIRIS","S_TEMEL"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(11,"TEXT","S_KIRIS","K1 30x60 C30 B420C Ø16",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(12,"TEXT","S_KIRIS","K2 25x50 C30 B420C Ø12",Double.NaN,Double.NaN),
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
        has(cmp.text,"Donatı çapı");
        has(cmp.text,"SDS = 1.10");
        has(cmp.text,"Zemin Sınıfı = ZC");
        has(cmp.text,"KAT / AKS / ELEMAN BAZLI KARŞILAŞTIRMA");

        String floorReport=
            "1. KAT\n"+
            "AKS A-3 K12 30x70 C30 B420C Ø16\n"+
            "AKS B-4 S05 40x40 C30 B420C Ø18\n"+
            "2. KAT\n"+
            "AKS A-3 K12 30x60 C30 B420C Ø14\n"+
            "AKS C-2 P3 25x100 C30 B420C Ø16\n";
        MusaAiStructuralCalc.Model floorModel=MusaAiStructuralCalc.parse("katli-hesap.pdf",floorReport);
        yes(floorModel.elements.size()==4,"four floor-aware report elements expected");
        yes(floorModel.elements.get(0).floor.equals("1.KAT"),"first floor missing");
        yes(floorModel.elements.get(0).axis.equals("A-3"),"axis A-3 missing");
        yes(floorModel.elements.get(2).floor.equals("2.KAT"),"duplicate K12 must retain second floor");

        MusaAiDrawingIndex floorIndex=new MusaAiDrawingIndex("Statik",4,0,
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE"),
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(21,"TEXT","S_KIRIS","1. KAT AKS A-3 K12 30x70 C30 B420C Ø16",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(22,"TEXT","S_KOLON","1. KAT AKS B-5 S05 40x40 C30 B420C Ø18",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(23,"TEXT","S_KIRIS","2. KAT AKS A-3 K12 30x55 C30 B420C Ø14",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(24,"TEXT","S_KIRIS","2. KAT AKS D-1 K99 25x50 C30 B420C Ø12",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison floorCmp=MusaAiStructuralCalc.compare(floorIndex,floorModel);
        yes(floorCmp.sameElements==1,"one exact floor/axis element match expected");
        yes(floorCmp.differentElements==2,"axis and section mismatches should be explicit");
        yes(floorCmp.reportOnlyElements==1,"P3 should exist only in report");
        yes(floorCmp.drawingOnlyElements==1,"K99 should exist only in DWG");
        yes(floorCmp.sourceIds.contains(22)&&floorCmp.sourceIds.contains(23),"mismatching DWG elements should be highlighted");
        has(floorCmp.text,"UYUMLU: 1");
        has(floorCmp.text,"FARKLI: 2");
        has(floorCmp.text,"SADECE RAPOR: 1");
        has(floorCmp.text,"SADECE DWG: 1");
        has(floorCmp.text,"1.KAT • AKS A-3 • K12");
        has(floorCmp.text,"Aks: rapor B-4 • DWG B-5");
        has(floorCmp.text,"Kesit: rapor 30x60 • DWG 30x55");

        String rebarReport=
            "1. KAT\n"+
            "AKS A-1 K7 30x60 C30 B420C ALT 4Ø16 ÜST 2Ø14 ETRİYE Ø8/20\n"+
            "AKS B-2 S7 40x40 C30 B420C 8Ø18 ETRİYE Ø8/15\n";
        MusaAiStructuralCalc.Model rebarModel=MusaAiStructuralCalc.parse("donati-hesap.pdf",rebarReport);
        yes(rebarModel.elements.size()==2,"two reinforcement-aware elements expected");
        yes(rebarModel.elements.get(0).longitudinalRebar.contains("ALT:4Ø16"),"bottom longitudinal reinforcement missing");
        yes(rebarModel.elements.get(0).longitudinalRebar.contains("ÜST:2Ø14"),"top longitudinal reinforcement missing");
        yes(rebarModel.elements.get(0).stirrups.contains("ETRİYE:Ø8/20"),"stirrup spacing missing");
        yes(rebarModel.elements.get(1).longitudinalRebar.contains("GENEL:8Ø18"),"general longitudinal reinforcement missing");

        MusaAiDrawingIndex rebarIndex=new MusaAiDrawingIndex("Statik",2,0,
            Arrays.asList("S_KIRIS","S_KOLON"),
            Arrays.asList("S_KIRIS","S_KOLON"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(31,"TEXT","S_KIRIS","1. KAT AKS A-1 K7 30x60 C30 B420C ALT 4Ø16 ÜST 2Ø14 ETRİYE Ø8/20",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(32,"TEXT","S_KOLON","1. KAT AKS B-2 S7 40x40 C30 B420C 6Ø18 ETRİYE Ø8/20",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison rebarCmp=MusaAiStructuralCalc.compare(rebarIndex,rebarModel);
        yes(rebarCmp.sameElements==1,"K7 reinforcement should match exactly");
        yes(rebarCmp.differentElements==1,"S7 reinforcement should differ");
        yes(rebarCmp.sourceIds.contains(32),"rebar mismatch source should be highlighted");
        has(rebarCmp.text,"Boyuna donatı: rapor GENEL:8Ø18 • DWG GENEL:6Ø18");
        has(rebarCmp.text,"Etriye: rapor ETRİYE:Ø8/15 • DWG ETRİYE:Ø8/20");

        yes(MusaAiStructuralCalc.isEngineeringTextExtension("model.e2k"),"e2k should be accepted");
        System.out.println("MusaAiStructuralCalcTest OK");
    }
}