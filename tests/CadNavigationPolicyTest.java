import com.musa.cad.CadNavigationPolicy;

public final class CadNavigationPolicyTest {
    private static void near(float actual,float expected,String name){
        if(Math.abs(actual-expected)>1e-5f*Math.max(1f,Math.abs(expected)))throw new AssertionError(name+" "+actual+" != "+expected);
    }
    public static void main(String[]args){
        near(CadNavigationPolicy.clampScale(2f,1f,1f),2f,"normal scale");
        near(CadNavigationPolicy.clampScale(.001f,1f,2f),.1f,"minimum relative zoom");
        near(CadNavigationPolicy.clampScale(Float.NaN,3f,2f),3f,"bad gesture keeps current scale");
        near(CadNavigationPolicy.clampScale(1000000f,2f,.1f),819.2f,"maximum relative zoom");
        near(CadNavigationPolicy.cullingPadWorld(2.5f),60f,"24 pixel culling guard");
        near(CadNavigationPolicy.cullingPadWorld(Float.NaN),0f,"invalid culling scale");
        System.out.println("CAD navigation scale/culling policy cases passed");
    }
}
