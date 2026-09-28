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
        require(panel,"MusaCAD AI'ya yazın","chat composer");
        require(panel,"interface Host","future model host abstraction");
        require(panel,"interface Reply","async reply abstraction");
        require(panel,"SOFT_INPUT_ADJUST_RESIZE","keyboard-safe AI panel");
        require(panel,"Çizime sor","drawing-question quick prompt");
        require(panel,"Metraj","takeoff quick prompt");
        require(panel,"{\"Mimari\",\"ARKAI_FULL\"}","architecture quick prompt");
        require(panel,"{\"Statik\",\"STATIKAI_FULL\"}","structural quick prompt");
        require(panel,"{\"Mekanik\",\"MEKAI_FULL\"}","mechanical expert quick prompt");
        require(panel,"{\"ProjAI\",\"PROJAI_FULL\"}","multidisciplinary quick prompt");
        require(panel,"\"G-MEKAI\"","developer mechanical expert quick prompt");
        require(panel,"\"G-ProjAI\"","developer multidisciplinary quick prompt");
        require(panel,"Kontrol","inspection quick prompt");
        require(icon,"<vector","AI vector icon");

        System.out.println("MusaCAD AI panel contract OK.");
    }
}
