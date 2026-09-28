import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public final class MusaAiMechanicalContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String report=read("app/src/main/java/com/musa/cad/MusaAiAutoReport.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiMechanical.java");
        require(main,"MusaAiMechanical.analyze","mechanical AI bridge");
        require(main,"Mekanik çizimde vurgulanan","mechanical highlights");
        require(panel,"{\"Mekanik\",\"Mekanik tesisat özeti\"}","mechanical quick action");
        require(report,"MusaAiMechanical.reportSection","mechanical report section");
        require(engine,"Pis / atık su","waste-water system");
        require(engine,"Yangın / sprinkler","fire system");
        require(engine,"VRF / klima / soğutma","VRF system");
        require(engine,"Havalandırma","ventilation system");
        require(engine,"Doğalgaz","natural-gas system");
        require(engine,"yönetmelik uygunluğu","safety disclaimer");
        System.out.println("MusaAiMechanicalContractTest OK");
    }
}
