import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class Mc1ProductionContractTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void need(String source,String needle,String label){
        if(!source.contains(needle))throw new AssertionError(label+" missing: "+needle);
    }
    public static void main(String[] args)throws Exception{
        String appGradle=read("app/build.gradle");
        String activity=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String managerGradle=read("licensemanager/build.gradle");
        String managerUi=read("licensemanager/src/main/java/com/musa/cad/licensemanager/MainActivity.java");
        String issuer=read("licensemanager/src/main/java/com/musa/cad/licensemanager/LicenseIssuer.java");
        String verifier=read("app/src/main/java/com/musa/cad/LicenseToken.java");

        need(appGradle,"ALLOW_LEGACY_SHORT_LICENSE", "app production legacy gate");
        need(appGradle,"productionRelease ? \"false\" : \"true\"", "app production legacy disable");
        need(activity,"final String secureLicenseId=LicenseManager.installationId(this)", "visible MC1 device identity");
        need(activity,"Güvenli Lisans Kimliği + MC1 lisansı", "direct production guidance");
        need(activity,"copySecureLicenseId(secureLicenseId)", "secure identity copy action");
        need(activity,"Production sürümünde 12 haneli kısa kod kabul edilmez", "production short-code rejection guidance");

        need(managerGradle,"ALLOW_LEGACY_SHORT_LICENSE", "manager legacy gate");
        need(managerGradle,"managerProductionRelease ? \"false\" : \"true\"", "manager production legacy disable");
        need(managerUi,"BuildConfig.ALLOW_LEGACY_SHORT_LICENSE", "manager MC1-only production UI");
        need(managerUi,"productionKeyMatches()", "manager public-key fingerprint enforcement");
        need(managerUi,"Aktif RSA anahtarı MusaCAD release public key'i ile eşleşmiyor", "mismatch issuance block");
        need(managerUi,"YENİ ANAHTAR ile .mlk yedeği oluşturun", "first-run MC1 bootstrap guidance");

        need(issuer,"SHA256withRSA", "MC1 RSA signature");
        need(issuer,"MC1|", "MC1 payload");
        need(verifier,"constantEquals(installationId,expectedInstallationId)", "device binding");
        need(verifier,"nowMs>expiresAt", "expiry enforcement");

        System.out.println("MC1 production contract OK: secure identity, RSA device binding, fingerprint gate and legacy disable.");
    }
}
