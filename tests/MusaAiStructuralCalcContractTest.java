import java.nio.file.*;

public final class MusaAiStructuralCalcContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String report=read("app/src/main/java/com/musa/cad/MusaAiDetailedReport.java");
        has(main,"PICK_STRUCT_CALC");
        has(main,"pickStructuralCalcDocument");
        has(main,"handleStructuralCalcPicked");
        has(main,"MusaAiPdfBoxTextExtractor.extract");
        has(main,"MusaAiStructuralCalc.compare");
        has(main,"structuralCalcModel");
        has(main,"Statik Hesap–Proje Karşılaştırma Raporu");
        has(report,"3B. STATİK HESAP RAPORU ↔ DWG ÇAPRAZ KONTROLÜ");
        has(report,"MusaAiStructuralCalc.compare");
        System.out.println("MusaAiStructuralCalcContractTest OK");
    }
}