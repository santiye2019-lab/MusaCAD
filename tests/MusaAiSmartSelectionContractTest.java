import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiSmartSelectionContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        require(view,"aiHighlightedSourceIds","multi-highlight state");
        require(view,"setAiHighlightedSources","AI highlight setter");
        require(view,"drawAiHighlights","AI highlight rendering");
        require(view,"selectedSourceType()","same-object reference type");
        require(main,"MusaAiSmartSelection.plan","AI smart-selection bridge");
        require(main,"clearAiHighlights","AI highlight clear");
        require(parser,"layer.sourceId,layer.sourceType","AI source id indexing");
        System.out.println("MusaAiSmartSelectionContractTest OK");
    }
}
