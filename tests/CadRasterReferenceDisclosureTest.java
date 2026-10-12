import java.nio.file.Files;
import java.nio.file.Path;

/** External IMAGE raster references are not silently reported as decoded pictures. */
public final class CadRasterReferenceDisclosureTest {
    private static void require(boolean b,String m){if(!b)throw new AssertionError(m);}
    public static void main(String[]args)throws Exception {
        String parser=Files.readString(Path.of("app/src/main/java/com/musa/cad/DxfParser.java"));
        String ui=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"));
        require(parser.contains("rasterReferences++"),"image entities not counted in streamed model");
        require(parser.contains("ready.externalRasterImageCount=collector.rasterReferences"),
            "raster warning not propagated to result");
        require(parser.contains("r.externalRasterImageCount=externalRasterImageCount"),
            "raster warning lost when changing layout");
        require(ui.contains("görüntü verisi değil, yalnız çerçeve gösteriliyor"),
            "drawing information hides missing external raster content");
        System.out.println("External DXF IMAGE boundary-only warning contract passed");
    }
}
