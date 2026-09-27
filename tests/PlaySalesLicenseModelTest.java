import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class PlaySalesLicenseModelTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Play sales/license model regression in "+area+": missing "+needle);
    }
    public static void main(String[]args)throws Exception{
        String billing=read("app/src/play/java/com/musa/cad/PlayBillingManager.java");
        String license=read("app/src/main/java/com/musa/cad/LicenseManager.java");
        String screen=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String verifier=read("app/src/main/java/com/musa/cad/PlayPurchaseVerifier.java");
        String worker=read("server/trial-worker/src/index.js");
        String gradle=read("app/build.gradle");

        require(worker,"const TRIAL_MS = 24 * 60 * 60 * 1000","one-day full trial");
        require(license,"eligibleForPlayYearlyRenewal","renewal eligibility");
        require(license,"K_EVER_PAID_LICENSE","renewal requires prior paid license");
        require(screen,"Yıllık yenileme için mevcut lisans gerekli","renewal-only UI");
        require(screen,"Google Play yalnızca mevcut veya daha önce etkinleştirilmiş yıllık MusaCAD lisansını yenilemek içindir","renewal-only explanation");

        require(billing,"BillingClient.ProductType.SUBS","Google Play subscription product");
        require(billing,"PlayBillingPolicy.isYearlyBillingPeriod","P1Y offer enforcement");
        require(billing,"LicenseManager.eligibleForPlayYearlyRenewal(context)","purchase gate");
        require(billing,"setObfuscatedAccountId(LicenseManager.installationId(context))","device-bound Play purchase");
        require(verifier,"request.put(\"purpose\",\"annual_renewal\")","annual renewal verification purpose");
        require(verifier,"BuildConfig.PLAY_YEARLY_PRODUCT_ID","yearly product binding");
        require(worker,"purpose !== \"annual_renewal\"","backend renewal-only binding");
        require(worker,"purchases/subscriptionsv2/tokens","server-side Google Play verification");
        require(worker,":acknowledge","server-side acknowledgement");
        require(gradle,"musacad_yearly_renewal","yearly Play product id");
        require(gradle,"playImplementation 'com.android.billingclient:billing:9.1.0'","Billing dependency is Play-only");
        require(gradle,"buildConfigField \"boolean\", \"PLAY_DISTRIBUTION\", \"true\"","Play distribution flag");

        System.out.println("Play sales/license model OK: free install + one-day trial + manual first activation + Play yearly renewal is locked.");
    }
}
