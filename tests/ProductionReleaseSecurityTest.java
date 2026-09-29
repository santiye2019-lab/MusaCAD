import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ProductionReleaseSecurityTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void need(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String gradle=read("app/build.gradle");
        String manager=read("app/src/main/java/com/musa/cad/LicenseManager.java");
        String screen=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String lmGradle=read("licensemanager/build.gradle");
        String lm=read("licensemanager/src/main/java/com/musa/cad/licensemanager/MainActivity.java");
        String key=read("licensemanager/src/main/java/com/musa/cad/licensemanager/LicenseKeyStore.java");
        String settings=read("settings.gradle");

        need(gradle,"musacadProductionRelease","production release flag");
        need(gradle,"verifyDirectProductionReleaseConfig","direct release gate");
        need(gradle,"verifyPlayProductionReleaseConfig","Play release gate");
        need(gradle,"MUSACAD_TRIAL_API_URL","trial endpoint requirement");
        need(gradle,"MUSACAD_TRIAL_PUBLIC_KEY_PEM","trial public key requirement");
        need(gradle,"Production MusaCAD requires MUSACAD_AI_API_URL and MUSACAD_AI_SESSION_URL","Gandalf production endpoint gate");
        need(gradle,"MUSACAD_PLAY_VERIFY_URL","Play verification endpoint requirement");
        need(gradle,"ALLOW_LEGACY_SHORT_LICENSE","legacy short-code production policy");

        need(manager,"BuildConfig.ALLOW_LEGACY_SHORT_LICENSE&&ShortLicenseCode.verify","release short-code gate");
        need(screen,"Güvenli Lisans Kimliği","secure device identity UI");
        need(screen,"GÜVENLİ KİMLİĞİ KOPYALA","secure identity copy action");

        need(settings,"include ':licensemanager'","License Manager module");
        need(lmGradle,"EXPECTED_LICENSE_KEY_FINGERPRINT","main-app public key binding");
        need(lm,"productionKeyMatches()","key fingerprint enforcement");
        need(lm,"LicenseKeyStore.generateBackup","backup-before-activation flow");
        need(lm,"LicenseKeyStore.restore","key recovery flow");
        need(key,"AndroidKeyStore","device wrapped private key");
        need(key,"AES/GCM/NoPadding","operational private-key encryption");
        need(key,"LicenseBackupCrypto.encrypt","portable encrypted backup");

        System.out.println("Production release security contract passed.");
    }
}
