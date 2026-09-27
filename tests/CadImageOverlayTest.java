import com.musa.cad.CadImageOverlay;

public final class CadImageOverlayTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void near(float actual,float expected,String message){if(Math.abs(actual-expected)>1e-4f)throw new AssertionError(message+": "+actual+" != "+expected);}
    public static void main(String[]args){
        CadImageOverlay image=new CadImageOverlay("abc123.png","Plan.png",10f,20f,100f,50f,0f);
        require(image.contains(10f,20f,0f),"center hit");
        require(image.contains(60f,45f,.01f),"edge hit");
        require(!image.contains(61f,46f,0f),"outside miss");
        CadImageOverlay moved=image.movedTo(30f,40f);near(moved.centerX,30f,"move X");near(moved.centerY,40f,"move Y");
        CadImageOverlay scaled=moved.scaled(1.5f);near(scaled.width,150f,"scale width");near(scaled.height,75f,"scale height");
        CadImageOverlay rotated=scaled.rotated(90f);near(rotated.rotationDegrees,90f,"rotation");
        require(rotated.contains(30f,40f,0f),"rotated center hit");
        require(rotated.contains(30f,110f,6f),"rotated long axis hit");
        float[] corners=rotated.corners();require(corners.length==8,"four corners");
        CadImageOverlay copy=rotated.copy();require(copy!=rotated&&copy.key.equals(rotated.key),"copy");
        System.out.println("Raster image overlay geometry passed");
    }
}
