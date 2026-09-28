import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiDrawingIntegrationContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        require(parser,"public MusaAiDrawingIndex aiDrawingIndex()","DXF AI index");
        require(parser,"analysisText(layer.entity)","DXF text extraction");
        require(parser,"raw instanceof Label","TEXT extraction");
        require(parser,"raw instanceof MTextLabel","MTEXT extraction");
        require(main,"currentAiDrawingIndex()","AI index cache");
        require(main,"MusaAiDrawingQuestions.answer","drawing question bridge");
        System.out.println("MusaAiDrawingIntegrationContractTest OK");
    }
}
