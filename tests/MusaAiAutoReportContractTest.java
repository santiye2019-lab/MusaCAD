import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public final class MusaAiAutoReportContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiAutoReport.java");
        require(main,"MusaAiAutoReport.generate","automatic report bridge");
        require(main,"lastAiReport","last report state");
        require(main,"shareAiReport","report TXT sharing");
        require(main,"setAiHighlightedSources(report.sourceIds)","report finding highlights");
        require(panel,"{\"Rapor\",\"Proje raporu oluştur\"}","AI report quick action");
        require(engine,"MusaAiQuantityTakeoff.answer","takeoff section");
        require(engine,"MusaAiProjectControl.analyze","CAD quality section");
        require(engine,"MusaAiRevisionCompare.compare","revision section");
        require(engine,"MusaAiTableOleAnalysis.answer","embedded document section");
        System.out.println("MusaAiAutoReportContractTest OK");
    }
}
