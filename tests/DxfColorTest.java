import com.musa.cad.DxfColor;

public class DxfColorTest {
    private static void rgb(int aci,int expected){
        int actual=DxfColor.aciRgb(aci);
        if(actual!=expected)throw new AssertionError("ACI "+aci+" expected "+Integer.toHexString(expected)+" got "+Integer.toHexString(actual));
    }
    public static void main(String[] args){
        rgb(1,0xFF0000);rgb(2,0xFFFF00);rgb(3,0x00FF00);rgb(4,0x00FFFF);rgb(5,0x0000FF);rgb(6,0xFF00FF);rgb(7,0xFFFFFF);
        rgb(8,0x414141);rgb(9,0x808080);
        rgb(10,0xFF0000);rgb(11,0xFFAAAA);rgb(12,0xBD0000);rgb(13,0xBD7E7E);
        rgb(20,0xFF3F00);rgb(21,0xFFBFAA);rgb(22,0xBD2E00);rgb(30,0xFF7F00);rgb(31,0xFFD4AA);
        rgb(60,0xBFFF00);rgb(90,0x00FF00);rgb(100,0x00FF3F);rgb(130,0x00FFFF);rgb(140,0x00BFFF);
        rgb(150,0x007FFF);rgb(170,0x0000FF);rgb(180,0x3F00FF);rgb(210,0xFF00FF);rgb(240,0xFF003F);
        rgb(250,0x333333);rgb(251,0x505050);rgb(252,0x696969);rgb(253,0x828282);rgb(254,0xBEBEBE);rgb(255,0xFFFFFF);
        if(DxfColor.trueColorArgb(0x123456)!=0xFF123456)throw new AssertionError("true color");
        if(DxfColor.aciRgb(0)!=0xFFFFFF||DxfColor.aciRgb(256)!=0xFFFFFF)throw new AssertionError("invalid ACI fallback");
        System.out.println("Pinned LibreDWG ACI/TrueColor palette cases passed");
    }
}
