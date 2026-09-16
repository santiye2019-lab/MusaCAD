import com.musa.cad.DxfColor;

public class DxfColorTest {
    private static void rgb(int aci,int expected){
        int actual=DxfColor.aciRgb(aci);
        if(actual!=expected)throw new AssertionError("ACI "+aci+" expected "+Integer.toHexString(expected)+" got "+Integer.toHexString(actual));
    }
    public static void main(String[] args){
        rgb(1,0xFF0000);rgb(2,0xFFFF00);rgb(7,0xFFFFFF);
        rgb(10,0xFF0000);rgb(11,0xFF7F7F);rgb(12,0xA50000);rgb(20,0xFF3F00);rgb(21,0xFF9F7F);
        rgb(90,0x00FF00);rgb(130,0x00FFFF);rgb(170,0x0000FF);rgb(210,0xFF00FF);rgb(240,0xFF003F);
        rgb(250,0x333333);rgb(251,0x505050);rgb(252,0x696969);rgb(253,0x828282);rgb(254,0xBEBEBE);rgb(255,0xFFFFFF);
        if(DxfColor.trueColorArgb(0x123456)!=0xFF123456)throw new AssertionError("true color");
        System.out.println("ACI/TrueColor palette cases passed");
    }
}
