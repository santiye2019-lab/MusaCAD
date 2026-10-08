import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiPanelContractTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("MusaCAD AI panel regression in "+area+": missing "+needle);
    }

    public static void main(String[]args)throws Exception{
        String layout=read("app/src/main/res/layout/activity_main.xml");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String icon=read("app/src/main/res/drawable/ic_ai.xml");

        require(layout,"@+id/aiButton","editor AI entry");
        require(layout,"@drawable/ic_ai","AI icon binding");
        require(main,"findViewById(R.id.aiButton).setOnClickListener","AI button wiring");
        require(main,"showMusaAi()","AI sheet entry");
        require(main,"musaAiContextLabel()","drawing context bridge");
        require(main,"handleMusaAiPrompt","AI host callback");
        require(main,"activeDxf.entityCount","DXF entity context");
        require(main,"activeDxf.layerCount","DXF layer context");

        require(panel,"BottomSheetDialog","AI bottom sheet");
        require(panel,"MusaCAD AI","AI title");
        require(panel,"Gandalf'a yazın veya sesli komut verin","chat composer");
        require(panel,"interface Host","future model host abstraction");
        require(panel,"interface Reply","async reply abstraction");
        require(panel,"SOFT_INPUT_ADJUST_RESIZE","keyboard-safe AI panel");
        require(panel,"PDF olarak görüntüle","PDF preview top button");
        require(panel,"onViewPdf","PDF preview host callback");
        require(panel,"previousTurns.toString()","conversation context");
        require(panel,"recentTurns.addLast","bounded chat turns");
        require(panel,"Yerel + bulut","honest local/cloud status");
        if(panel.contains("addQuickPromptAuto")||panel.contains("String[][] prompts"))
            throw new AssertionError("Legacy command chips must not replace natural conversation");
        require(panel,"Gandalf dinliyor","Gandalf voice status");
        require(icon,"<vector","AI vector icon");
        require(panel,"timeoutHandler.postDelayed(timeout,75_000L)","Gandalf request never stays processing indefinitely");
        require(panel,"completed.compareAndSet(false,true)","only first terminal reply can update request");
        require(panel,"@Override public void progress","intermediate cloud state visible to user");

        System.out.println("MusaCAD AI panel contract OK.");
    }
}
