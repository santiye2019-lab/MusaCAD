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

        String memberReport=
            "1. KAT\n"+
            "AKS A-1 K10 30x60 C30 B420C ALT 4Ø16 ÜST 2Ø14 İLAVE 2Ø16 ETRİYE Ø8/20\n"+
            "AKS B-2 S10 40x40 C30 B420C 8Ø18 ETRİYE Ø8/15\n"+
            "AKS C-3 P10 25x200 C30 B420C DÜŞEY Ø12/20 YATAY Ø10/20\n"+
            "AKS D-4 D10 15x300 C30 B420C ALT Ø12/20 ÜST Ø10/20\n";
        MusaAiStructuralCalc.Model memberModel=MusaAiStructuralCalc.parse("tip-donati.pdf",memberReport);
        yes(memberModel.elements.size()==4,"four member-specific reinforcement rows expected");
        yes(memberModel.elements.get(0).memberType==MusaAiStructuralCalc.MemberType.BEAM,"K10 must be beam");
        yes(memberModel.elements.get(1).memberType==MusaAiStructuralCalc.MemberType.COLUMN,"S10 must be column");
        yes(memberModel.elements.get(2).memberType==MusaAiStructuralCalc.MemberType.WALL,"P10 must be wall");
        yes(memberModel.elements.get(3).memberType==MusaAiStructuralCalc.MemberType.SLAB,"D10 must be slab");
        yes(memberModel.elements.get(0).memberRebar.contains("KİRİŞ:İLAVE:2Ø16"),"beam additional reinforcement missing");
        yes(memberModel.elements.get(1).memberRebar.contains("KOLON:BOYUNA:8Ø18"),"column longitudinal reinforcement missing");
        yes(memberModel.elements.get(2).memberRebar.contains("PERDE:DÜŞEY:Ø12/20"),"wall vertical reinforcement missing");
        yes(memberModel.elements.get(2).memberRebar.contains("PERDE:YATAY:Ø10/20"),"wall horizontal reinforcement missing");
        yes(memberModel.elements.get(3).memberRebar.contains("DÖŞEME:ALT:Ø12/20"),"slab bottom distributed reinforcement missing");
        yes(memberModel.elements.get(3).memberRebar.contains("DÖŞEME:ÜST:Ø10/20"),"slab top distributed reinforcement missing");
        yes(memberModel.elements.get(2).stirrups.isEmpty(),"wall distributed reinforcement must not be treated as stirrup");
        yes(memberModel.elements.get(3).stirrups.isEmpty(),"slab distributed reinforcement must not be treated as stirrup");

        MusaAiDrawingIndex memberIndex=new MusaAiDrawingIndex("Statik",4,0,
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE","S_DOSEME"),
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE","S_DOSEME"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(41,"TEXT","S_KIRIS","1. KAT AKS A-1 K10 30x60 C30 B420C ALT 4Ø16 ÜST 2Ø14 İLAVE 2Ø16 ETRİYE Ø8/20",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(42,"TEXT","S_KOLON","1. KAT AKS B-2 S10 40x40 C30 B420C 8Ø18 ETRİYE Ø8/20",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(43,"TEXT","S_PERDE","1. KAT AKS C-3 P10 25x200 C30 B420C DÜŞEY Ø12/20 YATAY Ø10/25",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(44,"TEXT","S_DOSEME","1. KAT AKS D-4 D10 15x300 C30 B420C ALT Ø12/20 ÜST Ø10/20",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison memberCmp=MusaAiStructuralCalc.compare(memberIndex,memberModel);
        yes(memberCmp.sameElements==2,"beam and slab should match");
        yes(memberCmp.differentElements==2,"column stirrup and wall horizontal spacing should differ");
        yes(memberCmp.sourceIds.contains(42)&&memberCmp.sourceIds.contains(43),"member-specific mismatches should be highlighted");
        has(memberCmp.text,"Tip bazlı donatı");
        has(memberCmp.text,"KOLON:ETRİYE:Ø8/15");
        has(memberCmp.text,"KOLON:ETRİYE:Ø8/20");
        has(memberCmp.text,"PERDE:YATAY:Ø10/20");
        has(memberCmp.text,"PERDE:YATAY:Ø10/25");

        String detailingReport=
            "1. KAT\n"+
            "AKS A-1 K20 30x60 C30 B420C ALT 4Ø16 ETRİYE Ø8/20 BİNDİRME 60 CM ANKRAJ 40Ø PAS PAYI 25 MM SIKLAŞTIRMA BOYU 600 MM ALT DONATI SÜREKLİ\n"+
            "AKS B-2 S20 40x40 C30 B420C 8Ø18 ETRİYE Ø8/15 BİNDİRME 50Ø ANKRAJ 700 MM PAS PAYI 30 MM SIKLAŞTIRMA 500 MM DONATI SÜREKLİ\n"+
            "AKS C-3 P20 25x200 C30 B420C DÜŞEY Ø12/20 YATAY Ø10/20 PAS PAYI 30 MM DÜŞEY DONATI SÜREKLİ\n";
        MusaAiStructuralCalc.Model detailingModel=MusaAiStructuralCalc.parse("detay-hesap.pdf",detailingReport);
        yes(detailingModel.elements.size()==3,"three detailing-aware elements expected");
        MusaAiStructuralCalc.Element k20=detailingModel.elements.get(0);
        yes(k20.lapSplices.contains("BİNDİRME:600mm"),"lap splice cm-to-mm normalization missing");
        yes(k20.anchorage.contains("ANKRAJ:40Ø"),"anchorage diameter-multiple missing");
        yes(k20.cover.contains("PAS PAYI:25mm"),"cover missing");
        yes(k20.confinement.contains("SIKLAŞTIRMA:600mm"),"confinement zone missing");
        yes(k20.continuity.contains("ALT:SÜREKLİ"),"bottom reinforcement continuity missing");
        yes(detailingModel.elements.get(1).continuity.contains("GENEL:SÜREKLİ"),"general continuity missing");

        MusaAiDrawingIndex detailingIndex=new MusaAiDrawingIndex("Statik",3,0,
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE"),
            Arrays.asList("S_KIRIS","S_KOLON","S_PERDE"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(51,"TEXT","S_KIRIS","1. KAT AKS A-1 K20 30x60 C30 B420C ALT 4Ø16 ETRİYE Ø8/20 BİNDİRME 600 MM ANKRAJ 40Ø PAS PAYI 30 MM SIKLAŞTIRMA 600 MM ALT DONATI SÜREKLİ",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(52,"TEXT","S_KOLON","1. KAT AKS B-2 S20 40x40 C30 B420C 8Ø18 ETRİYE Ø8/15 BİNDİRME 50Ø ANKRAJ 700 MM PAS PAYI 30 MM SIKLAŞTIRMA 450 MM DONATI SONLANIR",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(53,"TEXT","S_PERDE","1. KAT AKS C-3 P20 25x200 C30 B420C DÜŞEY Ø12/20 YATAY Ø10/20 PAS PAYI 30 MM DÜŞEY DONATI SÜREKLİ",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison detailingCmp=MusaAiStructuralCalc.compare(detailingIndex,detailingModel);
        yes(detailingCmp.sameElements==1,"P20 detailing should match");
        yes(detailingCmp.differentElements==2,"K20 cover and S20 confinement/continuity should differ");
        yes(detailingCmp.sourceIds.contains(51)&&detailingCmp.sourceIds.contains(52),"detailing mismatches should be highlighted");
        has(detailingCmp.text,"Pas payı: rapor PAS PAYI:25mm • DWG PAS PAYI:30mm");
        has(detailingCmp.text,"Sıklaştırma bölgesi: rapor SIKLAŞTIRMA:500mm • DWG SIKLAŞTIRMA:450mm");
        has(detailingCmp.text,"Donatı sürekliliği: rapor GENEL:SÜREKLİ • DWG GENEL:SONLANIR");
        has(detailingCmp.text,"Bindirme, ankraj, pas payı, sıklaştırma ve süreklilik");

        String foundationReport=
            "1. KAT\n"+
            "AKS A-1 T30 RADYE TEMEL 80x800 C35 B420C RADYE KALINLIK 80 CM KAZIK ÇAPI 80 CM KAZIK BOYU 18 M KAZIK AKS ARALIĞI 250 CM KAZIK ADEDİ 24\n"+
            "AKS B-2 D30 DÖŞEME 20x300 C35 B420C ZIMBALAMA DONATISI Ø12/10 ZIMBALAMA ÇEVRESİ 180 CM REZERVASYON R1 60x80 CM\n"+
            "AKS C-3 D31 DÖŞEME 18x300 C35 B420C ZIMBALAMA DONATISI Ø10/10 ZIMBALAMA ÇEVRESİ 160 CM ŞAFT R2 100x120 CM\n";
        MusaAiStructuralCalc.Model foundationModel=MusaAiStructuralCalc.parse("temel-zimbalama.pdf",foundationReport);
        yes(foundationModel.elements.size()==3,"three foundation/punching elements expected");
        MusaAiStructuralCalc.Element t30=foundationModel.elements.get(0);
        yes(t30.foundationDetails.contains("TEMEL KALINLIĞI:800mm"),"raft thickness missing");
        yes(t30.foundationDetails.contains("KAZIK ÇAPI:800mm"),"pile diameter missing");
        yes(t30.foundationDetails.contains("KAZIK BOYU:18000mm"),"pile length missing");
        yes(t30.foundationDetails.contains("KAZIK ARALIĞI:2500mm"),"pile spacing missing");
        yes(t30.foundationDetails.contains("KAZIK ADEDİ:24"),"pile count missing");
        yes(foundationModel.elements.get(1).punchingDetails.contains("ZIMBALAMA DONATISI:Ø12/10"),"punching reinforcement missing");
        yes(foundationModel.elements.get(1).punchingDetails.contains("ZIMBALAMA ÇEVRESİ:1800mm"),"punching perimeter missing");
        yes(foundationModel.elements.get(1).openings.contains("REZERVASYON:R1:600x800mm"),"reservation size missing");
        yes(foundationModel.elements.get(2).openings.contains("ŞAFT:R2:1000x1200mm"),"shaft size missing");
        yes(!foundationModel.sections.contains("12x10")&&!foundationModel.sections.contains("60x80")&&!foundationModel.sections.contains("100x120"),"rebar/opening sizes must not pollute structural section inventory");

        MusaAiDrawingIndex foundationIndex=new MusaAiDrawingIndex("Statik",3,0,
            Arrays.asList("S_TEMEL","S_DOSEME"),
            Arrays.asList("S_TEMEL","S_DOSEME"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(61,"TEXT","S_TEMEL","1. KAT AKS A-1 T30 RADYE TEMEL 80x800 C35 B420C RADYE KALINLIK 800 MM KAZIK ÇAPI 800 MM KAZIK BOYU 18 M KAZIK AKS ARALIĞI 240 CM KAZIK ADEDİ 24",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(62,"TEXT","S_DOSEME","1. KAT AKS B-2 D30 DÖŞEME 20x300 C35 B420C ZIMBALAMA DONATISI Ø12/10 ZIMBALAMA ÇEVRESİ 1800 MM REZERVASYON R1 60x90 CM",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(63,"TEXT","S_DOSEME","1. KAT AKS C-3 D31 DÖŞEME 18x300 C35 B420C ZIMBALAMA DONATISI Ø10/10 ZIMBALAMA ÇEVRESİ 1600 MM ŞAFT R2 1000x1200 MM",Double.NaN,Double.NaN)
            ),"cm");

        MusaAiStructuralCalc.Comparison foundationCmp=MusaAiStructuralCalc.compare(foundationIndex,foundationModel);
        yes(foundationCmp.sameElements==1,"D31 foundation coordination should match");
        yes(foundationCmp.differentElements==2,"T30 pile spacing and D30 reservation should differ");
        yes(foundationCmp.sourceIds.contains(61)&&foundationCmp.sourceIds.contains(62),"foundation/opening mismatches should be highlighted");
        has(foundationCmp.text,"Temel / radye / kazık detayı");
        has(foundationCmp.text,"KAZIK ARALIĞI:2500mm");
        has(foundationCmp.text,"KAZIK ARALIĞI:2400mm");
        has(foundationCmp.text,"Boşluk / rezervasyon");
        has(foundationCmp.text,"REZERVASYON:R1:600x800mm");
        has(foundationCmp.text,"REZERVASYON:R1:600x900mm");
        has(foundationCmp.text,"zımbalama kapasitesi veya temel taşıma gücü hesaplanmaz");

        yes(MusaAiStructuralCalc.isEngineeringTextExtension("model.e2k"),"e2k should be accepted");
        System.out.println("MusaAiStructuralCalcTest OK");
    }
}