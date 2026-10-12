import java.nio.file.Files;
import java.nio.file.Path;

/** Guard that partial reads are disclosed instead of silently called complete. */
public final class CadReadCompletenessStatusContractTest {
    private static void required(boolean ok,String note){if(!ok)throw new AssertionError(note);}
    public static void main(String[]args)throws Exception{
        String main=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"));
        String parser=Files.readString(Path.of("app/src/main/java/com/musa/cad/DxfParser.java"));
        required(main.contains("Atlanan / çizilemeyen CAD kaydı: "), "drawing info lacks skipped count");
        required(main.contains("activeDxf.skippedCount>0"),"partial drawing warning is not conditional");
        required(main.contains("CAD kaydı"),"project status lacks unsupported entity label");
        required(parser.contains("public final int entityCount,layerCount,skippedCount"),
            "2D parser no longer reports skipped count");
        required(main.contains("nativeScene.truncated"),"truncated native preview is not disclosed");
        System.out.println("CAD skipped-object and partial-scene warnings are visible");
    }
}
