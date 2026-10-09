import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ResponsiveEntryLicenseUiContractTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Responsive entry/license UI regression in "+area+": missing "+needle);
    }
    private static void forbid(String source,String needle,String area){
        if(source.contains(needle))throw new AssertionError("Responsive entry/license UI regression in "+area+": forbidden "+needle);
    }

    public static void main(String[]args)throws Exception{
        String locked=read("app/src/main/java/com/musa/cad/LockedScreenUi.java");
        String splash=read("app/src/main/java/com/musa/cad/SplashActivity.java");
        String about=read("app/src/main/java/com/musa/cad/AboutActivity.java");
        String license=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String licenseLayout=read("app/src/main/res/layout/activity_license.xml");
        String appManifest=read("app/src/main/AndroidManifest.xml");
        String manager=read("licensemanager/src/main/java/com/musa/cad/licensemanager/MainActivity.java");
        String managerManifest=read("licensemanager/src/main/AndroidManifest.xml");

        require(locked,"fillStageFromDrawable","real artwork aspect helper");
        require(locked,"getIntrinsicWidth()","intrinsic artwork width");
        require(locked,"getIntrinsicHeight()","intrinsic artwork height");
        require(splash,"fillStageFromDrawable(this,root,stage,R.drawable.musacad_screen_1","first welcome aspect preservation");
        require(splash,"setAutoSizeTextTypeUniformWithConfiguration","welcome CTA autosize");
        require(about,"fillStageFromDrawable(this,root,stage,R.drawable.musacad_screen_2","about screen aspect preservation");

        require(licenseLayout,"android:id=\"@+id/licenseScroll\"","scrollable license screen");
        require(licenseLayout,"android:id=\"@+id/licenseContent\"","responsive license content");
        require(licenseLayout,"android:scaleType=\"centerCrop\"","full-bleed decorative background");
        forbid(licenseLayout,"android:id=\"@+id/artworkStage\"","fixed hotspot license stage");

        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        require(main,"menu.add(0,MENU_MY_LICENSE,9,\"Lisans Bilgilerim\")","license menu visible in CAD");
        require(main,"case MENU_MY_LICENSE:showMyLicenseInfo();return true;","license menu opens status");
        require(main,"LicenseManager.installationId(this)","actual device-bound secure ID");
        require(main,"setNeutralButton(\"KİMLİĞİ KOPYALA\"","secure ID copying");
        require(main,"MusaAiSessionService.get(getApplicationContext())","server-backed developer check");
        require(main,"if(session.developer())","developer role is authenticated, not inferred");
        require(main,"new Thread(()->{","developer status checked off UI thread");
        require(main,"if(!isFinishing()&&!isDestroyed()&&dialog.isShowing())","dismissed dialog not updated");
        require(main,"Kimliği herkese açık ortamlarda paylaşmayın.","identity sharing warning");

        require(license,"buildResponsiveLicenseUi()","responsive license builder");
        require(license,"Güvenli MC1 Lisansı","production MC1 wording");
        require(license,"GÜVENLİ KİMLİĞİ KOPYALA","visible secure identity action");
        require(license,"styleDialogButtons","readable dialog actions");
        require(license,"TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration","responsive action text");
        require(appManifest,"android:windowSoftInputMode=\"adjustResize\"","license keyboard resize");

        require(manager,"scroll.setFillViewport(true)","manager scrolling");
        require(manager,"Güvenli Lisans Kimliğini yapıştır","short manager identity hint");
        require(manager,"e.setMinHeight(dp(50))","boxed manager fields");
        require(manager,"TextViewCompat.setAutoSizeTextTypeUniformWithConfiguration","manager button autosize");
        require(managerManifest,"android:windowSoftInputMode=\"adjustResize\"","manager keyboard resize");

        System.out.println("Responsive entry/license UI contract OK: artwork aspect, scrollable forms, readable actions and keyboard-safe layouts are guarded.");
    }
}
