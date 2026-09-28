import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiMechanicalControlContractTest {
    private static String read(String p)throws Exception{
        return Files.readString(Path.of(p),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError(area+" missing "+needle);
    }

    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiMechanicalControl.java");

        require(main,"MusaAiMechanicalControl.analyze","mechanical AI bridge");
        require(main,"setAiHighlightedSources(mechanical.sourceIds)","mechanical finding highlights");
        require(main,"Mekanik tesisat proje kontrolü","AI help");
        require(panel,"{\"Mekanik\",\"AI_MEKANIK_KONTROL\"}","mechanical quick action");

        require(engine,"Mekanik tesisat AI kontrolü","mechanical report title");
        require(engine,"Pis su / atık su","waste-water system");
        require(engine,"Yangın / sprinkler","fire system");
        require(engine,"Doğalgaz","gas system");
        require(engine,"Açık hat polyline","continuity review");
        require(engine,"çap veya DN etiketi bulunamadı","diameter annotation review");
        require(engine,"eğim etiketi bulunamadı","slope annotation review");
        require(engine,"Hidrolik hesap","engineering disclaimer");

        System.out.println("MusaAiMechanicalControlContractTest OK");
    }
}
