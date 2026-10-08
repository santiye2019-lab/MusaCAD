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
        float zoomIn=CadNavigationPolicy.pinchScaleFactor(1.5f);
        if(!(zoomIn>1.5f&&zoomIn<2f))throw new AssertionError("pinch zoom-in response should be accelerated: "+zoomIn);
        float zoomOut=CadNavigationPolicy.pinchScaleFactor(.75f);
        if(!(zoomOut<.75f&&zoomOut>.5f))throw new AssertionError("pinch zoom-out response should be accelerated: "+zoomOut);
        near(CadNavigationPolicy.pinchScaleFactor(Float.NaN),1f,"invalid pinch factor");
        near(CadNavigationPolicy.cullingPadWorld(2.5f),60f,"24 pixel culling guard");
        near(CadNavigationPolicy.cullingPadWorld(Float.NaN),0f,"invalid culling scale");
        near(CadNavigationPolicy.movingMinWorldSpan(17999,2f),0f,
            "smaller scenes retain all navigation detail");
        near(CadNavigationPolicy.movingMinWorldSpan(18000,2f),.9f,
            "dense scene motion subpixel LOD");
        near(CadNavigationPolicy.movingMinWorldSpan(60000,2f),1.8f,
            "extremely dense scene stronger motion LOD");
        near(CadNavigationPolicy.movingMinWorldSpan(90000,Float.NaN),0f,
            "invalid dimensions cannot trigger geometry suppression");
        if(!CadNavigationPolicy.isSubpixelBothAxes(.4f,.6f,.9f))
            throw new AssertionError("tiny glyph candidate may be culled while moving");
        if(CadNavigationPolicy.isSubpixelBothAxes(30f,.6f,.9f))
            throw new AssertionError("long pipe or line never culled while moving");
        if(CadNavigationPolicy.isSubpixelBothAxes(.4f,.6f,0f))
            throw new AssertionError("settled frame is always full fidelity");
        System.out.println("CAD navigation scale/culling policy cases passed");
    }
}
