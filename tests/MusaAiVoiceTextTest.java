import com.musa.cad.MusaAiVoiceText;
import java.util.*;

public final class MusaAiVoiceTextTest {
    private static void eq(String a,String b){if(!Objects.equals(a,b))throw new AssertionError(a+" != "+b);}
    public static void main(String[]args){
        eq(MusaAiVoiceText.clean("  Ekrana   sığdır \n "),"Ekrana sığdır");
        eq(MusaAiVoiceText.best(Arrays.asList("   ","Projeyi kontrol et","ikinci")),"Projeyi kontrol et");
        eq(MusaAiVoiceText.best(Collections.emptyList()),"");
        StringBuilder longText=new StringBuilder();for(int i=0;i<1200;i++)longText.append('a');
        if(MusaAiVoiceText.clean(longText.toString()).length()!=1000)throw new AssertionError("voice bound");
        System.out.println("MusaAiVoiceTextTest OK");
    }
}
