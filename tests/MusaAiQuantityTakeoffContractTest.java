import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiQuantityTakeoffContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String index=read("app/src/main/java/com/musa/cad/MusaAiDrawingIndex.java");
        require(main,"MusaAiQuantityTakeoff.answer","AI takeoff bridge");
        require(parser,"aiLength(measure)","DXF length indexing");
        require(parser,"aiArea(measure)","DXF area indexing");
        require(index,"public final double length,area","measurement metadata");
        require(index,"unitName","drawing unit metadata");
        require(parser,"blockInsertions","block insertion takeoff bridge");
        require(parser,"\"BLOCK\"","block names indexed for equipment count");
        require(index,"quantity","block-array quantity metadata");
        System.out.println("MusaAiQuantityTakeoffContractTest OK");
    }
}
