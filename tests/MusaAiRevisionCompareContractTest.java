import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiRevisionCompareContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String index=read("app/src/main/java/com/musa/cad/MusaAiDrawingIndex.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiRevisionCompare.java");
        require(main,"aiRevisionBaseline","revision baseline state");
        require(main,"MusaAiRevisionCompare.compare","revision compare bridge");
        require(main,"findOtherRevisionCandidate","two-open-project fallback");
        require(main,"setAiHighlightedSources(revision.sourceIds)","revision highlights");
        require(parser,"measure==null?Double.NaN:measure.centerX()","revision center indexing");
        require(index,"centerX,centerY","revision center metadata");
        require(engine,"Değişen / taşınan","revision report");
        require(engine,"Yeni katman","layer delta");
        System.out.println("MusaAiRevisionCompareContractTest OK");
    }
}
