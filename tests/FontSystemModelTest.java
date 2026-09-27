import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class FontSystemModelTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Font system regression in "+area+": missing "+needle);
    }
    public static void main(String[]args)throws Exception{
        String manager=read("app/src/main/java/com/musa/cad/CadFontManager.java");
        String policy=read("app/src/main/java/com/musa/cad/CadFontPolicy.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String writer=read("app/src/main/java/com/musa/cad/DxfWriter.java");
        String app=read("app/src/main/java/com/musa/cad/MusaCadApp.java");

        require(manager,"/system/fonts","system TTF/OTF discovery");
        require(manager,"/vendor/fonts","vendor font discovery");
        require(manager,"Typeface.createFromFile","actual TTF/OTF loading");
        require(manager,"Yalnız TTF veya OTF","safe user font import");
        require(manager,"COMMON_SHX","common SHX catalog");
        require(manager,"findSubstituteByBase","same-name TTF/OTF substitute for SHX");
        require(manager,"CadFontPolicy.shxFallbackFamily","nearest SHX fallback");
        require(policy,"dxfStyleName","stable DXF style names");

        require(parser,"CadFontManager.resolveTypeface","drawing renderer font resolution");
        require(parser,"CadFontManager.isAvailable","missing-font detection");
        require(parser,"style.fontFile.isEmpty()","original font-file metadata retention");

        require(view,"addTextEdit(float x,float y,String text,CadFontManager.Choice font)","new text font selection");
        require(view,"updateSelectedTextFont(CadFontManager.Choice font)","existing TEXT font replacement");
        require(view,"CadFontManager.resolveTypeface(edit.textFamilyHint","edited text rendering");

        require(activity,"PICK_FONT=33","font picker request");
        require(activity,"Font Yöneticisi","font manager UI");
        require(activity,"TTF/OTF YÜKLE","user font import action");
        require(activity,"CadFontManager.importFont","font import handling");
        require(activity,"Eksik / fallback","missing-font user report");

        require(writer,"collectTextStyles","DXF style collection");
        require(writer,"writeMissingTextStyles","DXF STYLE record injection");
        require(writer,"CadFontPolicy.dxfFontFile","DXF font filename persistence");
        require(writer,"tag(out,7,edit.textStyleName)","TEXT style reference persistence");

        require(app,"CadFontManager.init(this)","application font-manager initialization");
        System.out.println("Font system model OK: selection, replacement, system discovery, SHX fallback, missing-font report, import and DXF persistence are wired.");
    }
}
