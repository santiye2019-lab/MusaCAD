import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ProductionPreflightContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void need(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[] args)throws Exception{
        String wf=read(".github/workflows/production-preflight.yml");
        String ps=read("tools/production/setup-production-secrets.ps1");
        String docs=read("PRODUCTION_SETUP.md");

        need(wf,"workflow_dispatch","manual preflight");
        need(wf,"MUSACAD_RELEASE_KEYSTORE_B64","main keystore secret");
        need(wf,"MUSACAD_MANAGER_KEYSTORE_B64","manager keystore secret");
        need(wf,"MUSACAD_TRIAL_PUBLIC_KEY_PEM","trial public key secret");
        need(wf,"Probe production trial backend","trial endpoint probe");
        need(wf,"verifyDirectProductionReleaseConfig","Gradle direct gate");
        need(wf,"verifyPlayProductionReleaseConfig","Gradle Play gate");
        if(wf.contains("assembleDirectRelease")||wf.contains("assemblePlayRelease")||wf.contains("bundlePlayRelease"))
            throw new AssertionError("preflight must not build APK/AAB");

        need(ps,"keytool -genkeypair","local Android keystore generation");
        need(ps,"openssl","trial key generation");
        need(ps,"GitHub","secret instructions");
        need(docs,"DEVICE_FINAL_QA.md","real-device handoff");

        System.out.println("Production preflight contract passed.");
    }
}
