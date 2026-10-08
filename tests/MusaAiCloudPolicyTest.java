import com.musa.cad.MusaAiCloudPolicy;

public final class MusaAiCloudPolicyTest {
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void no(boolean value,String message){if(value)throw new AssertionError(message);}
    public static void main(String[]args){
        no(MusaAiCloudPolicy.shouldUseCloud("Gandalf, metraj çıkar"),"ordinary Gandalf command stays local");
        yes(MusaAiCloudPolicy.shouldUseCloud("Gandalf, bu projeyi derin analiz et"),"explicit Gandalf deep analysis");
        yes(MusaAiCloudPolicy.shouldUseProjectPackage("Gandalf, bu projeyi tüm disiplinlerde derin analiz et"),"all disciplines package");
        yes(MusaAiCloudPolicy.shouldUseProjectPackage("Gandalf proje paketi kontrolü"),"package phrase");
        no(MusaAiCloudPolicy.shouldUseProjectPackage("GSTATIKAI_OPENINGS"),"single discipline expert stays active drawing");
        yes(MusaAiCloudPolicy.shouldUseCloud("Bu projeyi güncel kaynaklarla derin analiz et"),"deep cloud intent");
        yes(MusaAiCloudPolicy.shouldUseCloud("GMEKAI_FIRE"),"GMEKAI cloud expert route");
        yes(MusaAiCloudPolicy.allowEditProposals("GMEKAI_VRF"),"GMEKAI exposes proposal tools");
        yes(MusaAiCloudPolicy.shouldUseCloud("GSTATIKAI_OPENINGS"),"GSTATIKAI cloud expert route");
        yes(MusaAiCloudPolicy.shouldUseCloud("GELKAI_POWER"),"GELKAI cloud expert route");
        yes(MusaAiCloudPolicy.allowEditProposals("GYANGAI_SMOKE"),"GYANGAI exposes proposal tools");
        no(MusaAiCloudPolicy.shouldUseCloud("Projeyi kontrol et"),"local control should stay local");
        yes(MusaAiCloudPolicy.allowWeb("Gandalf, güncel yönetmeliğe göre webden kontrol et"),"web intent");
        no(MusaAiCloudPolicy.allowWeb("Gandalf, çizimi analiz et"),"web should be explicit");
        yes(MusaAiCloudPolicy.allowEditProposals("Gandalf, bu hattı düzelt"),"edit proposal intent");
        no(MusaAiCloudPolicy.allowEditProposals("Gandalf, raporla"),"report should not expose edit tools");
        no(MusaAiCloudPolicy.allowEditProposals("Silivrikapı projesini analiz et"),
            "Place-name substring must not unlock destructive tools");
        yes(MusaAiCloudPolicy.allowEditProposals("Bu çizgiyi sil"),
            "Explicit deletion may propose actions but still needs approval");
        String prompt=MusaAiCloudPolicy.promptForCloud("Gandalf, pis su projesini incele");
        if(!"pis su projesini incele".equals(prompt))throw new AssertionError("prefix strip: "+prompt);
        System.out.println("MusaAiCloudPolicyTest OK");
    }
}
