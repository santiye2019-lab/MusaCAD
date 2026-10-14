import com.musa.cad.NativePageTransformGuard;

public final class NativePageTransformGuardTest {
    private static void yes(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    private static void rejects(Runnable action,String why){
        try{action.run();throw new AssertionError("not rejected: "+why);}
        catch(IllegalStateException expected){}
    }
    private static float[] matrix(float scale,float translateX,float translateY){
        return new float[]{scale,0f,translateX,0f,-scale,translateY,0f,0f,1f};
    }
    public static void main(String[] ignored){
        float[] first=matrix(1.25f,-450f,900f);
        float[] next=matrix(1.25f,-450f,900f);
        NativePageTransformGuard.requireSame(first,next);
        yes(first!=next,"separately decoded page arrays");
        NativePageTransformGuard.requireSame(
                matrix(1f,-0.0f,3f),matrix(1f,0.0f,3f));
        rejects(()->NativePageTransformGuard.requireSame(
                first,matrix(1.25f,-449f,900f)),
                "one page's world origin may shift due to broken DWG extents");
        rejects(()->NativePageTransformGuard.requireSame(
                first,matrix(1.20f,-450f,900f)),
                "different normalization scales cannot be composited");
        rejects(()->NativePageTransformGuard.requireSame(
                first,new float[]{1.25f,0f,-450f,0f,-1.25f,900f,.1f,0f,1f}),
                "unknown perspective page transform rejected");
        rejects(()->NativePageTransformGuard.requireSame(
                first,matrix(0f,-450f,900f)),
                "collapsed/singular normalization rejected");
        rejects(()->NativePageTransformGuard.requireSame(
                first,matrix(Float.NaN,-450f,900f)),
                "non-finite scale rejected");
        rejects(()->NativePageTransformGuard.requireSame(
                first,matrix(1f,Float.POSITIVE_INFINITY,900f)),
                "non-finite translation rejected");
        rejects(()->NativePageTransformGuard.requireSame(first,null),
                "missing page normalization rejected");
        rejects(()->NativePageTransformGuard.requireValid(new float[]{1,2,3}),
                "truncated matrix rejected");
        // First scene invalid must be rejected before it becomes the origin
        // for every subsequently displayed page.
        rejects(()->NativePageTransformGuard.requireValid(
                new float[]{1f,0f,0f,0f,1f,0f,0f,0f,0f}),
                "bad homogeneous coordinate rejected");
        NativePageTransformGuard.requireSame(first,next);
        System.out.println("Global DWG page transform consistency and corrupt-extents guards PASS");
    }
}
