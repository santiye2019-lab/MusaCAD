import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ImageOverlayPersistenceModelTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){if(!source.contains(needle))throw new AssertionError("Image persistence regression in "+area+": missing "+needle);}
    private static void reject(String source,String needle,String area){if(source.contains(needle))throw new AssertionError("Image persistence regression in "+area+": stale "+needle);}
    public static void main(String[]args)throws Exception{
        String overlay=read("app/src/main/java/com/musa/cad/CadImageOverlay.java");
        String store=read("app/src/main/java/com/musa/cad/CadImageOverlayStore.java");
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");

        require(overlay,"public final String uri","persistable source URI");
        require(overlay,"moveTo","move support");
        require(overlay,"scaleBy","scale support");
        require(overlay,"rotateBy","rotation support");

        require(store,"cad_image_overlays","internal persistent store");
        require(store,"manifest.json","placement manifest");
        require(store,"Bitmap.CompressFormat.PNG","image bytes persisted independently of source document");
        require(store,"item.put(\"cx\"","center persisted");
        require(store,"item.put(\"rot\"","rotation persisted");

        require(view,"getImageOverlaysDrawing","save in CAD/drawing coordinates");
        require(view,"drawingPointFromContent","center content-to-drawing conversion");
        require(view,"drawingDistanceFromContent","size content-to-drawing conversion");
        require(view,"restoreImageOverlaysDrawing","restore from CAD/drawing coordinates");
        require(view,"contentPointFromDrawing","center drawing-to-content conversion");
        require(view,"contentLengthFromDrawing","size drawing-to-content conversion");

        require(activity,"takePersistableUriPermission","source URI permission");
        require(activity,"CadImageOverlayStore.load(getApplicationContext(),imageStoreKey(loaded.sourceUri))","reopen saved image state");
        require(activity,"attachStoredImages(project,loaded.persistedImages)","restore overlays after DXF activation");
        require(activity,"cad.getImageOverlaysDrawing()","capture placement before save");
        require(activity,"CadImageOverlayStore.save(getApplicationContext(),imageStoreKey(uri),imageOverlays)","save image bytes and placement for output DXF");
        require(activity,"recoveryImageStoreKey(id)","recovery-specific image store");
        require(activity,"CadImageOverlayStore.save(getApplicationContext(),recoveryImageStoreKey(id),imageOverlays)","recovery saves images");
        require(activity,"attachStoredImages(project,recoveredImages)","recovery restores images");
        require(activity,"currentProject.savedFingerprint=cad.editFingerprint()","successful raster save becomes clean baseline");
        reject(activity,"raster görüntü DXF içine gömülmedi","old non-persistent warning");
        reject(activity,"Görüntü katmanı yalnız proje görünümünde","old session-only warning");

        System.out.println("Raster persistence model OK: image bytes, CAD placement, save/reopen and recovery are connected.");
    }
}
