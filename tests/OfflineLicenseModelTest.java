import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class OfflineLicenseModelTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Offline license regression in "+area+": missing "+needle);
    }
    public static void main(String[]args)throws Exception{
        String activity=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String manager=read("app/src/main/java/com/musa/cad/LicenseManager.java");
        String shortCode=read("app/src/main/java/com/musa/cad/ShortLicenseCode.java");
        String cli=read("tools/license-generator/ShortLicenseGenerator.java");
        String gui=read("tools/license-generator/ShortLicenseGeneratorApp.java");
        String rsa=read("tools/license-generator/LicenseGenerator.java");

        require(activity,"LicenseManager.installationId(this)","displayed secure MC1 identity");
        require(activity,"LicenseManager.activateCode(this,code)","local activation path");
        require(activity,"Güvenli Lisans Kimliği için RSA imzalı MC1 lisansı kullanılır","offline/institutional user guidance");

        require(manager,"ShortLicenseCode.verify(code,serialId(c),now)","offline short-code verification");
        require(manager,"LicenseToken.verify","RSA offline license verification");
        require(shortCode,"public static String issue(String serial,int days,long nowMs)","short license issuance");

        require(cli,"ShortLicenseCode.issue","CLI institutional/offline generator");
        require(gui,"MusaCAD Serial (12 karakter)","desktop Serial generator");
        require(rsa,"Keep the private key outside the repository","secure RSA institutional generator");

        int start=manager.indexOf("public static ActivationResult activateCode");
        int end=manager.indexOf("private static boolean verifyStoredPaidToken",start);
        String activation=manager.substring(start,end);
        if(activation.contains("PlayBilling")||activation.contains("TrialService")||activation.contains("Http")||activation.contains("URL("))
            throw new AssertionError("Offline activation must not depend on Google Play or network");

        System.out.println("Offline/direct-APK license model OK: production MC1 activation is device-bound and network-independent.");
    }
}
