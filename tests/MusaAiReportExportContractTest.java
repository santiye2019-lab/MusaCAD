import java.nio.file.*;

public final class MusaAiReportExportContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String export=read("app/src/main/java/com/musa/cad/MusaAiReportExport.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        has(export,"public static File pdf");
        has(export,"public static File docx");
        has(export,"word/document.xml");
        has(main,"PICK_ESTIMATE");
        has(main,"Keşif dosyası seçicisi açıldı");
        has(main,"MusaAiEstimate.compare");
        has(main,"MusaAiDetailedReport.generate");
        has(main,"Raporu Word olarak çıkar");
        has(main,"Raporu PDF olarak çıkar");
        System.out.println("MusaAiReportExportContractTest OK");
    }
}
