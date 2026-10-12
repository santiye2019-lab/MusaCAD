import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** A green test gate cannot claim the APK works if Gandalf endpoints are blank. */
public final class MusaCadWorkingApkRegressionTest {
    static void yes(boolean condition,String failure){
        if(!condition)throw new AssertionError(failure);
    }
    static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    public static void main(String[]args)throws Exception{
        String workflow=read(".github/workflows/android.yml");
        String gradle=read("app/build.gradle");
        String menu=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String health=read("app/src/main/java/com/musa/cad/MusaAiCloudHealth.java");

        yes(workflow.contains("Verify Gandalf connection configuration before assembling test APK"),
            "An APK must be blocked when Gandalf endpoints are missing");
        yes(workflow.contains("secrets.MUSACAD_AI_API_URL")&&
                workflow.contains("secrets.MUSACAD_AI_SESSION_URL"),
            "Existing public gateway addresses must reach the test APK build");
        yes(workflow.contains("secrets.MUSACAD_TRIAL_API_URL")&&
                workflow.contains("secrets.MUSACAD_TRIAL_PUBLIC_KEY_PEM")&&
                workflow.contains("TRIAL_PUBLIC_KEY_PEM"),
            "Restored APK must preserve signed trial entitlement verification");
        yes(workflow.contains("BuildConfig.java")&&workflow.contains("AI_SESSION_URL")&&
                workflow.contains("AI_API_URL")&&
                workflow.contains("APK withheld"),
            "Built flavor must be checked, not just unit test sources");
        yes(!workflow.contains("name: Build developer test APK (no production signing or deployment)"),
            "Old unconfigured build recipe must not remain");
        yes(gradle.contains("musaAiApiUrl = System.getenv('MUSACAD_AI_API_URL')"),
            "AI_API_URL remains injected by BuildConfig");
        yes(gradle.contains("musaAiSessionUrl = System.getenv('MUSACAD_AI_SESSION_URL')"),
            "AI_SESSION_URL remains injected by BuildConfig");
        yes(health.contains("return validBase(BuildConfig.AI_API_URL) &&"),
            "Indicator must require actual HTTPS endpoints");
        yes(menu.contains("Tam vektör hazırlanıyor")&&
                menu.contains("Tam vektör yüklemeyi yeniden dene")&&
                menu.contains("private void retryNativeVectorLoad()"),
            "Native preview must expose clear editor-loading state and retry");
        yes(menu.contains("project.prepareTask=task")&&
                menu.contains("project.parsed=finalParsed")&&
                menu.contains("cad.upgradeNativeDrawing(finalParsed)"),
            "Successful retry must upgrade the original project in place");
        yes(menu.contains("project.prepareError=message"),
            "Failed retry keeps an observable error");
        System.out.println("MusaCadWorkingApkRegressionTest OK");
    }
}
