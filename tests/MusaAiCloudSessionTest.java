import com.musa.cad.*;

public final class MusaAiCloudSessionTest {
    private static void yes(boolean b,String m){if(!b)throw new AssertionError(m);}
    private static void no(boolean b,String m){if(b)throw new AssertionError(m);}
    public static void main(String[]args){
        MusaAiCloudSession.clear();
        no(MusaAiCloudSession.isValid(),"cleared");
        MusaAiCloudSession.set("abc",System.currentTimeMillis()+60000);
        yes(MusaAiCloudSession.isValid(),"valid");
        yes("abc".equals(MusaAiCloudSession.token()),"token");
        MusaAiCloudSession.set("expired",System.currentTimeMillis()-1);
        no(MusaAiCloudSession.isValid(),"expired");
        yes(MusaAiCloudSession.token().isEmpty(),"expired token hidden");
        MusaAiCloudSession.clear();
        System.out.println("MusaAiCloudSessionTest OK");
    }
}
