import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class ResponsiveEntryLicenseContractTest {
    private static String read(String path)throws Exception{return Files.readString(Path.of(path),StandardCharsets.UTF_8);}
    private static void require(String source,String needle,String area){
        if(!source.contains(needle))throw new AssertionError("Responsive screen regression in "+area+": missing "+needle);
    }

    public static void main(String[]args)throws Exception{
        String splash=read("app/src/main/java/com/musa/cad/SplashActivity.java");
        String ui=read("app/src/main/java/com/musa/cad/LockedScreenUi.java");
        String layout=read("app/src/main/res/layout/activity_license.xml");
        String license=read("app/src/main/java/com/musa/cad/LicenseActivity.java");
        String manager=read("licensemanager/src/main/java/com/musa/cad/licensemanager/MainActivity.java");

        require(ui,"textPx(TextView view","locked artwork text scaling");
        require(ui,"LockedScreenLayout.scalePx","reference-art scale math");
        require(splash,"LockedScreenUi.textPx(threeD","splash badge responsive text");
        require(splash,"LockedScreenUi.textPx(start","splash CTA responsive text");

        require(layout,"android:scaleType=\"centerCrop\"","license full-bleed background");
        require(layout,"@drawable/musacad_screen_3_v2","license artwork");
        require(license,"LockedScreenUi.textPx(codeLabel","license code label scaling");
        require(license,"LockedScreenUi.textPx(licenseCode","license input scaling");
        require(license,"LockedScreenUi.textPx(deviceId","secure ID scaling");
        require(license,"LockedScreenUi.textPx(directModel","direct distribution card scaling");
        require(license,"SOFT_INPUT_ADJUST_PAN","keyboard visibility policy");

        require(manager,"scroll.setFillViewport(true)","manager scrolling");
        require(manager,"screenWidthDp<360","narrow-screen action stacking");
        require(manager,"setResponsiveTextSize","manager font scaling");
        require(manager,"fontScale","manager large-font cap");
        require(manager,"SOFT_INPUT_ADJUST_RESIZE","manager keyboard resizing");
        require(manager,"ViewGroup.LayoutParams.WRAP_CONTENT","manager content-height buttons");

        System.out.println("Responsive entry/license screen contract OK.");
    }
}
