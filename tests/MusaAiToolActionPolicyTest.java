import com.musa.cad.*;

public final class MusaAiToolActionPolicyTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void no(boolean v,String m){if(v)throw new AssertionError(m);}
    public static void main(String[]args){
        MusaAiToolActionPolicy.Decision view=MusaAiToolActionPolicy.evaluate("ZE");
        yes(view.allowed,"ZE allowed");no(view.requiresConfirmation,"ZE no confirm");

        MusaAiToolActionPolicy.Decision erase=MusaAiToolActionPolicy.evaluate("ERASE");
        yes(erase.allowed,"ERASE allowed");yes(erase.requiresConfirmation,"ERASE confirm");
        yes(erase.risk==MusaAiToolActionPolicy.Risk.EDIT,"ERASE risk");

        MusaAiToolActionPolicy.Decision save=MusaAiToolActionPolicy.evaluate("SAVE");
        yes(save.allowed,"SAVE allowed");yes(save.requiresConfirmation,"SAVE confirm");
        yes(save.risk==MusaAiToolActionPolicy.Risk.FILE,"SAVE risk");

        MusaAiToolActionPolicy.Decision injected=MusaAiToolActionPolicy.evaluate("SHELL rm -rf /");
        no(injected.allowed,"unknown denied");

        System.out.println("MusaAiToolActionPolicyTest OK");
    }
}
