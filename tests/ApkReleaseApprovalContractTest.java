import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * No source push, PR, scheduled test, AI pilot, or default manual action may
 * produce an APK before a separate, affirmative final-release decision.
 */
public final class ApkReleaseApprovalContractTest {
    private static String read(String file)throws Exception{
        return Files.readString(Path.of(file),StandardCharsets.UTF_8);
    }
    private static void require(boolean yes,String reason){
        if(!yes)throw new AssertionError(reason);
    }
    private static void guarded(String path,String gate)throws Exception{
        String s=read(path);
        require(s.contains("release_approved:"),path+" lacks an explicit final-approval input");
        require(s.contains("default: false"),path+" lacks a safe default");
        require(s.contains(gate),path+" must require final release approval");
    }
    public static void main(String[]args)throws Exception{
        String manual="github.event_name == 'workflow_dispatch' && inputs.release_approved == true";
        String android=read(".github/workflows/android.yml");
        guarded(".github/workflows/android.yml",manual);
        require(android.contains("if: ${{ "+manual+" }}"),"Android APK must not build on push");
        require(!android.contains("contains(github.event.head_commit.message, '[build-apk]')"),
            "commit message must never trigger an APK");
        guarded(".github/workflows/production-apk.yml",manual);
        guarded(".github/workflows/production-license-manager.yml",manual);
        guarded(".github/workflows/production-licensemanager-apk.yml",manual);
        String gandalf=read(".github/workflows/deploy-gandalf-ai.yml");
        guarded(".github/workflows/deploy-gandalf-ai.yml",manual);
        require(gandalf.contains("- name: Build Gandalf-enabled production APK\n        if: ${{ "+manual+" }}"),
            "AI deployment must not package APK unless explicitly approved");
        String pilot=read(".github/workflows/qwen-isolated-pilot-24h.yml");
        guarded(".github/workflows/qwen-isolated-pilot-24h.yml",
            "inputs.build_pilot_apk == true && inputs.release_approved == true");
        require(pilot.contains("if: inputs.operation == 'deploy' && inputs.build_pilot_apk == true && inputs.release_approved == true"),
            "AI pilot APK must have its own final-acceptance gate");
        System.out.println("ApkReleaseApprovalContractTest OK");
    }
}
