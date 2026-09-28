import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class DistributionModelsTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Distribution-model regression in "+area+": missing "+needle);
    }
    public static void main(String[]args)throws Exception{
        String gradle=read("app/build.gradle");
        String screen=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String play=read("app/src/play/java/com/musa/cad/PlayBillingManager.java");
        String direct=read("app/src/direct/java/com/musa/cad/PlayBillingManager.java");

        require(gradle,"flavorDimensions \"distribution\"","distribution flavor dimension");
        require(gradle,"play {","Google Play flavor");
        require(gradle,"direct {","direct APK flavor");
        require(gradle,"DISTRIBUTION_CHANNEL","distribution channel BuildConfig");
        require(gradle,"playImplementation 'com.android.billingclient:billing:9.1.0'","Play-only Billing dependency");

        require(screen,"BuildConfig.PLAY_DISTRIBUTION","runtime distribution UI gate");
        require(screen,"DOĞRUDAN APK / KURUMSAL","direct distribution UI");
        require(screen,"Güvenli Lisans Kimliği + MC1 lisansı","direct activation guidance");

        require(play,"com.android.billingclient.api.BillingClient","real Play Billing implementation");
        if(direct.contains("com.android.billingclient"))
            throw new AssertionError("Direct APK flavor must not compile or package Google Play Billing client classes");
        require(direct,"Güvenli Lisans Kimliği için üretilmiş MC1 lisansını kullanın","direct no-Play guidance");

        if(Files.exists(Path.of("app/src/main/java/com/musa/cad/PlayBillingManager.java")))
            throw new AssertionError("PlayBillingManager must be distribution-specific, not shared from main");

        System.out.println("Distribution models OK: Play and direct/institutional builds are separated and policy-gated.");
    }
}
