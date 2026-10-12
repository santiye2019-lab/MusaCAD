import java.nio.file.Files;
import java.nio.file.Path;

public final class CadNativeDwgWriteContractTest {
    private static String get(String file)throws Exception{
        return Files.readString(Path.of(file));
    }
    private static void check(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[]args)throws Exception {
        String cmake=get("app/src/main/cpp/CMakeLists.txt");
        String nativeCode=get("app/src/main/cpp/dwg_bridge.c");
        String javaCode=get("app/src/main/java/com/musa/cad/NativeDwgWrite.java");
        String ui=get("app/src/main/java/com/musa/cad/MainActivity.java");
        check(cmake.contains("MUSACAD_EXPERIMENTAL_DWG_WRITE"),"opt-in flag missing");
        check(cmake.contains("not enabled in production APK"),"unsafe default not explained");
        check(cmake.contains("MUSACAD_EXPERIMENTAL_DWG_WRITE \"Unsafe legacy-r2004 DXF to DWG writer; not enabled in production APK\" OFF"),
            "production build must explicitly disable writer");
        check(nativeCode.contains("doc.header.version>R_2004")&&
            nativeCode.contains("doc.header.version<R_2000"),
            "unsafe later DWG formats are not rejected");
        check(nativeCode.contains("dwg_write_file(output,&doc)")&&
            nativeCode.contains("dwg_read_file(output,&verified)"),
            "export lacks native writer or read-back verification");
        check(nativeCode.contains("verified.num_objects!=expected"),
            "object-count discrepancy accepted");
        check(javaCode.contains("if(!temp.delete())"),
            "LibreDWG refuses existing output: unlink before writing");
        check(javaCode.contains("CadDxfSaveStage.validate(stagedDxf)"),
            "source DXF not checked before native write");
        check(ui.contains("NativeDwgWrite.available()")&&
            ui.contains("requestEditedDwgSave()"),
            "safe opt-in DWG user flow missing");
        check(ui.contains("Orijinal dosya değiştirilmeyecek"),
            "DWG unsupported source must remain intact");
        System.out.println("Experimental legacy DWG writing opt-in and read-back safety contract passed");
    }
}
