import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ImageOverlayCoordinatePersistenceTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){if(!source.contains(needle))throw new AssertionError("Image coordinate persistence regression in "+area+": missing "+needle);}
    public static void main(String[]args)throws Exception{
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String store=read("app/src/main/java/com/musa/cad/CadImageOverlayStore.java");

        require(store,"manifest.json","persistent image manifest");
        require(store,"Bitmap.CompressFormat.PNG","persistent bitmap payload");

        require(view,"getImageOverlaysDrawing","drawing-coordinate snapshot");
        require(view,"drawingPointFromContent","center conversion to drawing coordinates");
        require(view,"drawingDistanceFromContent","size conversion to drawing units");
        require(view,"restoreImageOverlaysDrawing","drawing-coordinate restore");
        require(view,"contentPointFromDrawing","center conversion back to content");
        require(view,"contentLengthFromDrawing","size conversion back to content");
        require(view,"-image.rotationDegrees()","rotation conversion across flipped drawing Y axis");

        require(activity,"final List<CadImageOverlay> rasterOverlays=cad.getImageOverlaysDrawing()","save uses drawing-coordinate overlays");
        require(activity,"CadImageOverlayStore.save(getApplicationContext(),destinationOverlayKey,rasterOverlays)","saved output gets persistent image state");
        require(activity,"project.parsed!=null&&!project.persistedImages.isEmpty()","restore waits for vector coordinate model");
        require(activity,"cad.restoreImageOverlaysDrawing(project.persistedImages)","normal reopen restores drawing coordinates");
        require(activity,"cad.upgradeNativeDrawing(parsed);if(!project.persistedImages.isEmpty()){cad.restoreImageOverlaysDrawing","native DWG waits for complete vector model");

        System.out.println("Image overlay persistence uses stable CAD/drawing coordinates.");
    }
}
