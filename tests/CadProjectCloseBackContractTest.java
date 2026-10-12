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
        int close=source.indexOf("private void requestCloseProject(");
        int click=source.indexOf("private void closeProjectNow(",close);
        verify(close>=0&&click>close,"close request method exists");
        String request=source.substring(close,click);
        verify(request.contains("!projects.contains(project)"),"stale/closed project blocked");
        verify(request.contains("if(activeLoad!=null)"),"busy CAD session cannot switch tabs for close");
        verify(request.indexOf("projectCloseDialog!=null")<request.indexOf("activateProject(project)"),
            "another confirmation modal prevents background tab switch");
        verify(request.contains("if(currentProject!=project)"),"failed activation cannot close another tab");
        verify(request.contains("project!=currentProject||activeLoad!=null||!canEdit()"),
            "save-close verifies same editable project");
        verify(request.contains("Telefonun Geri tuşu yalnızca bu uyarıyı kapatır."),
            "unsaved close warning must not promise that Back discards work");
        int requestSave=source.indexOf("private void requestEditedDxfSave(");
        int publish=source.indexOf("private void saveEditedDxf(",requestSave);
        verify(requestSave>=0&&publish>requestSave,"DXF export workflow exists");
        String picker=source.substring(requestSave,publish);
        verify(picker.contains("catch(Exception failure){pendingCloseAfterSave=null;"),
            "document picker errors clear pending close");
        String save=source.substring(publish,source.indexOf("private void showShare(",publish));
        verify(save.contains("currentProject!=pendingCloseAfterSave"),
            "stale picker result may not save another project's edits");
        verify(save.contains("currentProject.sourceUri.equals(uri)"),
            "original CAD source URI must not be truncated");
        verify(save.contains("pendingCloseAfterSave=null;return;"),
            "failure before export resets pending close flag");
        verify(source.contains("currentProject,pendingCloseAfterSave,pendingDxfSaveProject"),
            "DXF picker must retain source project identity");
        verify(source.contains("pendingDxfSaveProject=currentProject;"),
            "save document picker captures originating tab");
        verify(source.contains("final ProjectSession selectedProject=pendingDxfSaveProject;"),
            "save callback restores captured project identity");
        verify(source.contains("selectedProject!=currentProject||!projects.contains(selectedProject)"),
            "picker callback must reject a stale or different active tab");
        verify(source.contains("pendingDxfSaveProject=null;return;"),
            "cancelling Android document selection clears project identity");
        System.out.println("CAD multi-project close: Back cancels and does not discard unsaved work");
    }
}
