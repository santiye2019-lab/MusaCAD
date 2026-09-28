import com.musa.cad.MusaAiCloudPolicy;

public final class MusaAiCloudPolicyTest {
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void no(boolean value,String message){if(value)throw new AssertionError(message);}
    public static void main(String[]args){
        yes(MusaAiCloudPolicy.shouldUseCloud("Gandalf, bu projeyi derin analiz et"),"Gandalf prefix");
        yes(MusaAiCloudPolicy.shouldUseCloud("Bu projeyi güncel kaynaklarla derin analiz et"),"deep cloud intent");
        no(MusaAiCloudPolicy.shouldUseCloud("Projeyi kontrol et"),"local control should stay local");
        yes(MusaAiCloudPolicy.allowWeb("Gandalf, güncel yönetmeliğe göre webden kontrol et"),"web intent");
        no(MusaAiCloudPolicy.allowWeb("Gandalf, çizimi analiz et"),"web should be explicit");
        yes(MusaAiCloudPolicy.allowEditProposals("Gandalf, bu hattı düzelt"),"edit proposal intent");
        no(MusaAiCloudPolicy.allowEditProposals("Gandalf, raporla"),"report should not expose edit tools");
        String prompt=MusaAiCloudPolicy.promptForCloud("Gandalf, pis su projesini incele");
        if(!"pis su projesini incele".equals(prompt))throw new AssertionError("prefix strip: "+prompt);
        System.out.println("MusaAiCloudPolicyTest OK");
    }
}
