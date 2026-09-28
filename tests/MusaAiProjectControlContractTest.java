import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiProjectControlContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String index=read("app/src/main/java/com/musa/cad/MusaAiDrawingIndex.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiProjectControl.java");
        require(main,"MusaAiProjectControl.analyze","project-control bridge");
        require(main,"setAiHighlightedSources(control.sourceIds)","finding highlights");
        require(parser,"aiGeometryKey(measure,layer.sourceType)","duplicate geometry signature");
        require(index,"closedKnown,closed","polyline topology metadata");
        require(index,"geometryKey","geometry identity metadata");
        require(engine,"Mükerrer geometri","duplicate check");
        require(engine,"Sıfır uzunluk/dejenere geometri","degenerate check");
        require(engine,"Boş TEXT/MTEXT/ATTRIB","empty text check");
        System.out.println("MusaAiProjectControlContractTest OK");
    }
}
