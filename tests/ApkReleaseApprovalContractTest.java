import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Production APKs require affirmative FINAL acceptance.
 * A 24-hour, developer-only pilot APK is NOT a production release:
 * it requires separate, explicit signed TEST APK consent (default false).
 * Neither source pushes nor PR tests can issue a signed pilot automatically.
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
        require(pilot.contains("build_pilot_apk:"),
            "AI pilot needs an explicit signed test APK approval input");
        require(pilot.contains("I approve creating a signed 24h TEST APK only"),
            "User must understand this is a test APK, not final acceptance");
        require(pilot.contains("default: false"),
            "AI pilot signed APK must be opt-in, never a default");
        require(pilot.contains("if: inputs.operation == 'deploy' && inputs.build_pilot_apk == true"),
            "AI pilot must require manual deploy and explicit test APK consent");
        require(!pilot.contains("inputs.release_approved == true"),
            "Pilot testing must not assert that final engineering acceptance passed");
        require(pilot.contains("name: MusaCAD-Qwen-24h-PILOT-signed") &&
                pilot.contains("retention-days: 2"),
            "Pilot must produce short-lived artifact rather than public release");
        require(pilot.contains("MUSACAD_PILOT_DEVELOPER_ONLY = \"true\"") &&
                pilot.contains("MUSACAD_PILOT_EXPIRES_AT_MS"),
            "Pilot worker must remain developer-only and expire");
        require(pilot.contains("Refusing APK with an unexpected AI host"),
            "Pilot APK must pin the correct isolated HTTPS endpoint");
        require(!pilot.contains("google-play-release")&&!pilot.contains("playstore-upload"),
            "A pilot workflow must never publish production artifacts");
        System.out.println("ApkReleaseApprovalContractTest OK");
    }
}
