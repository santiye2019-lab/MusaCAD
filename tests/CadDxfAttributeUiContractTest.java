import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class CadDxfAttributeUiContractTest {
    private static void check(boolean condition,String message) {
        if (!condition) throw new AssertionError(message);
    }
    public static void main(String[] args)throws Exception {
        String main=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        String writer=Files.readString(Path.of("app/src/main/java/com/musa/cad/DxfWriter.java"),StandardCharsets.UTF_8);
        check(main.contains("tool(\"Blok niteliği (DXF)\""),"block attribute tool must be exposed");
        check(main.contains("SAVE_ATTR_DXF=40"),"independent SAF request code required");
        check(main.contains("if(r==SAVE_ATTR_DXF&&c!=RESULT_OK){clearPendingAttributePatch();return;}"),"cancel must discard sensitive pending change");
        check(main.contains("else if(r==SAVE_ATTR_DXF)saveAttributePatchedDxf(data.getData());"),"SAF callback must route to patch export");
        check(main.contains("currentProject!=project"),"project switch must reject stale change");
        check(main.contains("project.sourceUri.equals(uri)"),"original-source destination is rejected");
        check(main.contains("CadDxfSaveStage.prepare(getCacheDir(),out->"),"attribute patch requires private cache staging");
        check(main.contains("CadDxfAttributePatch.write(base,out,DxfWriter.charset(base)"),"DXF native code page used");
        check(main.contains("CadDxfSaveStage.publish(staged,out);"),"only staged validated file may be exported");
        check(main.indexOf("CadDxfSaveStage.prepare(getCacheDir(),out->", main.indexOf("private void saveAttributePatchedDxf("))
                  < main.indexOf("getContentResolver().openOutputStream(uri,\"wt\")", main.indexOf("private void saveAttributePatchedDxf(")),
              "staging must precede opening truncating SAF destination");
        check(main.contains("cad.editCount()>0||cad.sourceModifiedCount()>0"),"other pending CAD edits require separate save");
        check(writer.contains("static Charset charset(File file)throws IOException"),"DXF charset detection accessible");
        System.out.println("DXF attribute UI SAF flow and source protection contract passed");
    }
}
