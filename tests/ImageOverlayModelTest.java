import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ImageOverlayModelTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){if(!source.contains(needle))throw new AssertionError("Image overlay regression in "+area+": missing "+needle);}
    public static void main(String[]args)throws Exception{
        String store=read("app/src/main/java/com/musa/cad/CadImageStore.java");
        String overlay=read("app/src/main/java/com/musa/cad/CadImageOverlay.java");
        String metadata=read("app/src/main/java/com/musa/cad/CadImageMetadata.java");
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String writer=read("app/src/main/java/com/musa/cad/DxfWriter.java");

        require(store,"image/jpeg","JPG/JPEG import");
        require(store,"image/png","PNG import");
        require(store,"image/webp","WebP import");
        require(store,"BitmapFactory.decodeFile","actual bitmap validation");
        require(store,"cad_images","persistent internal image store");

        require(overlay,"movedTo","image move geometry");
        require(overlay,"scaled","image scale geometry");
        require(overlay,"rotated","image rotation geometry");
        require(overlay,"contains","image selection hit testing");

        require(view,"drawImageOverlays","raster overlay drawing");
        require(view,"findImageAt","image selection");
        require(view,"selectedImageOverlay().movedTo","image move command");
        require(view,"image.rotated(90f)","image rotate command");
        require(view,"image.scaled(factor)","image scale command");
        require(view,"imageOverlays.remove(selectedImage)","image delete command");
        require(view,"SessionState","tab/session persistence");
        require(view,"getImageOverlaysDrawing","drawing-coordinate persistence");
        require(view,"setImageOverlaysDrawing","drawing-coordinate restore");

        require(activity,"CadImageStore.importImage","image import flow");
        require(activity,"cad.addImageOverlay","image placed as drawing object");
        require(activity,"CadImageMetadata.read(loaded.file)","saved placement restore");
        require(activity,"cad.setImageOverlaysDrawing(project.imageOverlaysDrawing)","project reopen placement restore");
        require(activity,"cad.getImageOverlaysDrawing()","save placement capture");

        require(writer,"CadImageMetadata.writeComments","DXF placement metadata write");
        require(writer,"CadImageMetadata.isMetadata","stale metadata replacement");
        require(metadata,"MUSACAD_IMAGE_V1","versioned private DXF metadata");
        System.out.println("Image overlay model OK: JPG/PNG/WebP import, placement, edit tools and DXF reopen persistence are wired.");
    }
}
