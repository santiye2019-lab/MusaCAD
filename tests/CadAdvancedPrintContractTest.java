import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class CadAdvancedPrintContractTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){if(!source.contains(needle))throw new AssertionError("Advanced print regression in "+area+": missing "+needle);}
    private static int count(String source,String needle){int n=0,at=0;while((at=source.indexOf(needle,at))>=0){n++;at+=needle.length();}return n;}

    public static void main(String[]args)throws Exception{
        String print=read("app/src/main/java/com/musa/cad/CadPrint.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");

        require(print,"Extents • Tüm çizim","Extents mode");
        require(print,"Display • Ekrandaki görünüm","Display mode");
        require(print,"Window • Seçili alan","Window mode");
        require(print,"Sayfaya sığdır","fit-to-page");
        require(print,"\"1:1\"","1:1 scale");
        require(print,"\"1:200\"","1:200 scale");
        require(print,"Özel ölçek","custom scale");
        require(print,"ÖNİZLEME","print preview");
        require(print,"FİZİKSEL YAZICI veya PDF","physical printer / PDF target");
        require(print,"drawImageOverlays","placed raster images");
        require(print,"CadFontManager.resolveTypeface","font parity");
        require(print,"drawVectorForPrint","vector PDF/print path");
        require(print,"drawSourceReplacement","edited source entities");
        require(print,"drawEdits","annotations/tables/blocks");
        require(print,"private static void renderPage","shared WYSIWYG renderer");
        if(count(print,"renderPage(")<3)throw new AssertionError("Preview and print must both call shared renderPage");

        require(main,"cad.visibleContentBounds()","Display viewport handoff");
        require(main,"cad.selectedAreaContentBounds()","Window handoff");
        require(main,"pendingPrintWindowSelection","interactive Window selection");
        require(view,"public RectF visibleContentBounds()","Display bounds");
        require(view,"public RectF selectedAreaContentBounds()","Window bounds");
        require(parser,"printMatrix(RectF target,int denominator,RectF requestedSource)","cropped print matrix");
        require(parser,"printableContentBounds()","Extents bounds");

        System.out.println("Advanced print contract OK: Extents/Display/Window, fit/custom scale, WYSIWYG preview, images/fonts and printer/PDF handoff are guarded.");
    }
}
