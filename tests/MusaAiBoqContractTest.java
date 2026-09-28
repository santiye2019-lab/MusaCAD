import java.nio.charset.StandardCharsets;
import java.nio.file.*;

public final class MusaAiBoqContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiBoq.java");
        require(main,"PICK_BOQ","BOQ picker request");
        require(main,"pickBoqDocument","BOQ picker");
        require(main,"handleBoqPicked","BOQ import");
        require(main,"MusaAiBoq.generate","drawing-generated BOQ");
        require(main,"MusaAiBoq.compare","project/BOQ compare");
        require(main,"PDF sayısal","PDF review warning");
        require(main,"MusaAiBoq.Model boqModel","per-project BOQ state");
        require(panel,"{\"Keşif\",\"Keşif yükle\"}","AI panel BOQ shortcut");
        require(engine,"PROJE – KEŞİF KARŞILAŞTIRMASI","comparison output");
        require(engine,"%1 yalnız raporlama eşiğidir","tolerance disclaimer");
        System.out.println("MusaAiBoqContractTest OK");
    }
}
