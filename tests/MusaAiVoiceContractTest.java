import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiVoiceContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String voice=read("app/src/main/java/com/musa/cad/MusaAiVoiceInput.java");
        require(panel,"MusaAiVoiceInput.launch","AI panel voice button");
        require(panel,"submit.run()","voice-to-AI submit");
        require(main,"MusaAiVoiceInput.handleActivityResult","voice activity result bridge");
        require(voice,"RecognizerIntent.ACTION_RECOGNIZE_SPEECH","Android speech recognizer");
        require(voice,"RecognizerIntent.EXTRA_PREFER_OFFLINE,true","offline preference");
        require(voice,"\"tr-TR\"","Turkish recognition");
        if(read("app/src/main/AndroidManifest.xml").contains("android.permission.RECORD_AUDIO"))
            throw new AssertionError("MusaCAD should not directly request microphone recording permission for delegated speech UI");
        System.out.println("MusaAiVoiceContractTest OK");
    }
}
