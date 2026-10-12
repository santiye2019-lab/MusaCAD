import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class DxfSkippedIntegrationContractTest {
    static void check(boolean ok,String text){if(!ok)throw new AssertionError(text);}
    public static void main(String[]args)throws Exception{
        String parser=Files.readString(Path.of("app/src/main/java/com/musa/cad/DxfParser.java"),StandardCharsets.UTF_8);
        String main=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        check(parser.contains("meta.noteSkipped(item.record,item.layer,\"Orphan POLYLINE VERTEX\")"),"orphan vertex");
        check(parser.contains("meta.noteSkipped(item.record,item.layer,\"Unsupported CAD type or invalid geometry\")"),"unsupported geometry evidence");
        check(parser.contains("rendered.skippedSamples=Collections.unmodifiableList"),"renderer copies sample list");
        check(parser.contains("rendered.skippedByType=Collections.unmodifiableMap"),"renderer copies type counts");
        check(parser.contains("r.skippedSamples=skippedSamples;r.skippedByType=skippedByType;return r;"),"layer/layout toggles preserve evidence");
        check(main.contains("CadSkippedSummary.describe(activeDxf.skippedCount,activeDxf.skippedSamples,activeDxf.skippedByType)"),
            "user can inspect skipped object evidence");
        check(main.contains("activeDxf.skippedCount>0?"),"status warning remains");
        System.out.println("Skipped object diagnostics Android UI and layout persistence contract passed");
    }
}
