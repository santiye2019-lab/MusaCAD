import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Keeps the Back key equivalent to CANCEL in multi-project close confirmation. */
public final class CadProjectCloseBackContractTest {
    private static void verify(boolean yes,String why){if(!yes)throw new AssertionError(why);}
    public static void main(String[]args)throws Exception{
        String source=Files.readString(Path.of("app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        int blockStart=source.indexOf("projectCloseDialog.setOnKeyListener(");
        int blockEnd=source.indexOf("projectCloseDialog.setOnDismissListener(",blockStart);
        verify(blockStart>=0&&blockEnd>blockStart,"close confirmation key handler exists");
        String handler=source.substring(blockStart,blockEnd);
        verify(handler.contains("KeyEvent.KEYCODE_BACK"),"Back specifically handled");
        verify(handler.contains("dialog.dismiss()"),"Back dismisses modal");
        verify(!handler.contains("closeProjectNow("),"Back must never dispose the active project");
        verify(!handler.contains("clearRecovery("),"Back must never erase recovery snapshot");
        verify(source.contains("setPositiveButton(\"KAYDET VE KAPAT\""),"save and close explicit action retained");
        verify(source.contains("setNeutralButton(\"KAYDETMEDEN KAPAT\""),"discard explicit action retained");
        verify(source.contains("setNegativeButton(\"İPTAL\",null)"),"explicit cancel button retained");
        verify(source.contains("if(dirty)"),"dirty state distinguished");
        verify(source.contains("projectCloseDialog.setOnDismissListener("),"cancel/back cleans modal refs");
        System.out.println("CAD multi-project close: Back cancels and does not discard unsaved work");
    }
}
