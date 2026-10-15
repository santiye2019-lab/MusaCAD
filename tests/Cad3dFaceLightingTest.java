import com.musa.cad.Cad3dFaceLighting;

/** Android-free numeric regression for viewport face shading. */
public final class Cad3dFaceLightingTest {
    private static void near(double actual,double expected,double tolerance,String description){
        if(!(Double.isFinite(actual)&&Math.abs(actual-expected)<=tolerance))
            throw new AssertionError(description+" actual="+actual+" expected="+expected);
    }
    private static void need(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[] args){
        float[] xy={0,0,0,4,0,0,0,3,0};
        near(Cad3dFaceLighting.brightness(xy,0,1,2),0.8d,1e-12,
                "normal facing +Z");
        near(Cad3dFaceLighting.brightness(xy,2,1,0),0.8d,1e-12,
                "reversed face stays identically lit");

        float[] yz={0,0,0,0,3,0,0,0,4};
        near(Cad3dFaceLighting.brightness(yz,0,1,2),0.35d,1e-12,
                "normal facing +X");

        // Float32 intermediate multiplications previously overflowed to
        // +/-infinity or NaN despite a valid finite source triangle.
        float v=1e20f;
        float[] huge={0,0,0,v,0,0,0,v,0};
        near(Cad3dFaceLighting.brightness(huge,0,1,2),0.8d,1e-12,
                "huge finite drawing still shades correctly");

        // Tiny but valid architecture/engineering face should not disappear
        // solely because its cross-product magnitude is under 1e-9.
        float e=1e-12f;
        float[] tiny={0,0,0,e,0,0,0,e,0};
        near(Cad3dFaceLighting.brightness(tiny,0,1,2),0.8d,1e-12,
                "tiny nondegenerate drawing face stays visible");

        float[] nearOrigin={1000000f,1000000f,0,
                            1000004f,1000000f,0,
                            1000000f,1000003f,0};
        near(Cad3dFaceLighting.brightness(nearOrigin,0,1,2),0.8d,1e-12,
                "million-unit translation invariance");
        need(Cad3dFaceLighting.brightness(xy,0,0,2)<0d,
                "degenerate repeated vertex never shaded");
        need(Cad3dFaceLighting.brightness(
            new float[]{0,0,0,1,1,1,2,2,2},0,1,2)<0d,
                "collinear face never shaded");
        need(Cad3dFaceLighting.brightness(
            new float[]{0,0,0,Float.NaN,0,0,0,1,0},0,1,2)<0d,
                "NaN coordinate cannot reach Android Canvas");
        need(Cad3dFaceLighting.brightness(xy,0,1,3)<0d,
                "invalid triangle index rejected");
        System.out.println("CAD viewport face normals: orientation, huge/tiny, invalid and degenerate PASS");
    }
}
