import com.musa.cad.DxfMTextBackground;

public class DxfMTextBackgroundTest {
    private static void yes(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void no(boolean v,String m){if(v)throw new AssertionError(m);}
    private static void eq(int a,int b,String m){if(a!=b)throw new AssertionError(m+": "+Integer.toHexString(a)+" != "+Integer.toHexString(b));}
    private static void close(double a,double b,String m){if(Math.abs(a-b)>1e-9)throw new AssertionError(m+": "+a+" != "+b);}
    public static void main(String[] args){
        no(DxfMTextBackground.enabled(0),"off");
        yes(DxfMTextBackground.enabled(1),"explicit fill");
        yes(DxfMTextBackground.enabled(2),"drawing background fill");
        no(DxfMTextBackground.enabled(16),"frame bit alone is not fill");
        eq(DxfMTextBackground.color(2,1,0x123456,0xff12181e),0xff12181e,"drawing background wins");
        eq(DxfMTextBackground.color(1,1,0x123456,0xff12181e),0xff123456,"true color wins");
        eq(DxfMTextBackground.color(1,3,-1,0xff12181e),0xff00ff00,"ACI fill");
        eq(DxfMTextBackground.color(1,0,-1,0xff12181e),0xff12181e,"fallback background");
        close(DxfMTextBackground.boxScale(Double.NaN),1.5,"invalid scale");
        close(DxfMTextBackground.boxScale(.5),1.5,"too-small scale");
        close(DxfMTextBackground.boxScale(20),10,"scale cap");
        close(DxfMTextBackground.padding(4,1.5),1,"padding");
        System.out.println("12 MTEXT background mask cases passed");
    }
}
