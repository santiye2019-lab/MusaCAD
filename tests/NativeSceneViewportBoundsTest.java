import com.musa.cad.NativeSceneViewportBounds;

/** Pure-JVM regression: authentic DWG POINT has zero span, not corrupt. */
public final class NativeSceneViewportBoundsTest {
    private static void yes(boolean ok,String why) {
        if (!ok)throw new AssertionError(why);
    }
    private static void approx(float x,float value) {
        yes(Float.isFinite(x)&&Math.abs(x-value)<=0.001f,
             "Expected "+value+" got "+x);
    }
    private static float[] valid(float x,float y,float a,float b) {
        float[] rect=NativeSceneViewportBounds.forScene(x,y,a,b);
        yes(rect!=null&&rect.length==4,"Valid original DWG bounds rejected");
        yes(rect[2]>rect[0]&&rect[3]>rect[1],
             "Viewport must be nondegenerate");
        for(float value:rect)yes(Float.isFinite(value),"Finite bounds");
        return rect;
    }
    public static void main(String[] args) {
        float[] point=valid(27.928f,8.65308f,27.928f,8.65308f);
        yes(point[0]<27.928f&&point[2]>27.928f&&
            point[1]<8.65308f&&point[3]>8.65308f,
            "True original POINT must stay centered in camera");
        float[] vertical=valid(5f,-10f,5f,80f);
        approx(vertical[1],-10f);approx(vertical[3],80f);
        yes(vertical[0]<5f&&vertical[2]>5f,"Vertical line visible");
        float[] horizontal=valid(-10f,20f,80f,20f);
        approx(horizontal[0],-10f);approx(horizontal[2],80f);
        yes(horizontal[1]<20f&&horizontal[3]>20f,"Horizontal line");
        float[] normal=valid(-100f,-200f,300f,400f);
        approx(normal[0],-100f);approx(normal[1],-200f);
        approx(normal[2],300f);approx(normal[3],400f);
        yes(NativeSceneViewportBounds.forScene(10f,1f,9f,8f)==null,
            "Reject reversed X");
        yes(NativeSceneViewportBounds.forScene(1f,10f,8f,9f)==null,
            "Reject reversed Y");
        yes(NativeSceneViewportBounds.forScene(
               Float.NaN,0f,1f,1f)==null,"Reject NaN");
        yes(NativeSceneViewportBounds.forScene(
               Float.POSITIVE_INFINITY,0f,1f,1f)==null,
               "Reject infinity");
        yes(NativeSceneViewportBounds.forScene(
               Float.MAX_VALUE,2f,Float.MAX_VALUE,2f)==null,
               "Do not overflow camera framing");
        float large=1e20f;
        float[] big=valid(large,large,large,large);
        yes(big[0]<large&&big[2]>large,
            "ULP-aware large DWG world coordinates");
        System.out.println("PASS pure Android viewport framing for real"
            +" original POINT, vertical LINE and horizontal LINE, reject NaN");
    }
}
