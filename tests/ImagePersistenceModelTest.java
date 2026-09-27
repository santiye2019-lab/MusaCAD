import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ImagePersistenceModelTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){if(!source.contains(needle))throw new AssertionError("Raster persistence regression in "+area+": missing "+needle);}
    private static void reject(String source,String needle,String area){if(source.contains(needle))throw new AssertionError("Raster persistence regression in "+area+": stale "+needle);}
    public static void main(String[]args)throws Exception{
        String overlay=read("app/src/main/java/com/musa/cad/CadImageOverlay.java");
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String writer=read("app/src/main/java/com/musa/cad/DxfWriter.java");
        String metadata=read("app/src/main/java/com/musa/cad/CadImageMetadata.java");

        require(overlay,"public final String uri","persistable image URI carried by overlay");
        require(overlay,"moveTo","move");
        require(overlay,"scaleBy","scale");
        require(overlay,"rotateBy","rotation");
        require(overlay,"hit","selection");

        require(view,"getImagePlacementsDrawing","content to drawing-coordinate save");
        require(view,"drawingPointFromContent","saved center in drawing coordinates");
        require(view,"drawingDistanceFromContent","saved size in drawing units");
        require(view,"restoreImagePlacement","drawing-coordinate reopen");
        require(view,"contentPointFromDrawing","restored center in content coordinates");
        require(view,"contentLengthFromDrawing","restored size in content coordinates");

        require(metadata,"MUSACAD_IMAGE_V1","versioned private metadata");
        require(writer,"CadImageMetadata.writeComments","write placement metadata");
        require(writer,"CadImageMetadata.isMetadata","replace stale metadata");

        require(activity,"takePersistableUriPermission","durable source URI permission");
        require(activity,"loaded.restoredRasters.addAll(readRasterPlacements(loaded.file))","read saved placements on DXF open");
        require(activity,"attachRestoredRasters(project,loaded.restoredRasters)","restore images after vector model activation");
        require(activity,"cad.getImagePlacementsDrawing()","capture current placement on save");
        require(activity,"DxfWriter.write(base,out,drawing,additions,replacements,removals,blocks,defaultLayer,imagePlacements)","save metadata in edited DXF");
        require(activity,"new ArrayList<>(cad.getImagePlacementsDrawing())","recovery snapshot includes raster placement");
        require(activity,"attachRestoredRasters(project,recoveredRasters)","recovery reopen includes raster placement");
        require(activity,"currentProject.savedFingerprint=cad.editFingerprint()","raster save becomes clean baseline");
        reject(activity,"raster görüntü DXF içine gömülmedi","old non-persistent raster warning");
        reject(activity,"görüntü katmanı yalnız proje görünümünde","old raster-only session warning");

        System.out.println("Raster persistence model OK: save, recovery and DXF reopen placement are wired.");
    }
}
