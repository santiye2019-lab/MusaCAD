import com.musa.cad.Cad3dSurfaceMetrics;

public final class Cad3dSurfaceMetricsTest {
    private static void yes(boolean test,String description){
        if(!test)throw new AssertionError(description);
    }
    private static void close(double actual,double expected,double tolerance,String label){
        yes(Math.abs(actual-expected)<=tolerance,label+": "+actual+" expected "+expected);
    }
    private static void fails(float[] xyz,int[] triangles,String label){
        try{Cad3dSurfaceMetrics.measure(xyz,triangles);
            throw new AssertionError("invalid triangle accepted: "+label);
        }catch(IllegalArgumentException good){}
    }
    public static void main(String[] args){
        float[] triangle={0,0,0,3,0,0,0,4,0};
        Cad3dSurfaceMetrics.Result one=Cad3dSurfaceMetrics.measure(triangle,new int[]{0,1,2});
        close(one.area,6,1e-12,"right triangle surface");
        yes(one.triangleCount==1&&one.degenerateCount==0,"triangle inventory");
        close(one.centroidX,1d,1e-12,"open triangular surface centroid X");
        close(one.centroidY,4d/3d,1e-12,"open triangular surface centroid Y");
        close(one.centroidZ,0d,1e-12,"open triangular surface centroid Z");
        // All six faces of a unit cube, each split into two triangles.
        float[] cube={0,0,0, 1,0,0, 1,1,0, 0,1,0,
                      0,0,1, 1,0,1, 1,1,1, 0,1,1};
        int[] faces={
          0,1,2, 0,2,3, 4,6,5, 4,7,6,
          0,4,5, 0,5,1, 1,5,6, 1,6,2,
          2,6,7, 2,7,3, 3,7,4, 3,4,0
        };
        Cad3dSurfaceMetrics.Result surface=Cad3dSurfaceMetrics.measure(cube,faces);
        close(surface.area,6,1e-12,"unit-cube triangle area");
        yes(surface.triangleCount==12,"cube face tessellation");
        close(surface.centroidX,.5d,1e-12,"cube shell surface centroid X");
        close(surface.centroidY,.5d,1e-12,"cube shell surface centroid Y");
        close(surface.centroidZ,.5d,1e-12,"cube shell surface centroid Z");
        float[] translated={1000000f,1000000f,1000000f,1000003f,1000000f,1000000f,
                            1000000f,1000004f,1000000f};
        Cad3dSurfaceMetrics.Result translatedSurface=Cad3dSurfaceMetrics.measure(translated,new int[]{0,1,2});
        close(translatedSurface.centroidX,1000001d,1e-9,"large WCS surface center X");
        close(translatedSurface.centroidY,1000000d+4d/3d,1e-9,"large WCS surface center Y");
        close(translatedSurface.centroidZ,1000000d,1e-9,"large WCS surface center Z");
        // Repeated indices are a degenerate FACE, not positive surface.
        Cad3dSurfaceMetrics.Result degenerate=Cad3dSurfaceMetrics.measure(
            triangle,new int[]{0,0,2});
        close(degenerate.area,0,0,"degenerate surface");
        yes(degenerate.degenerateCount==1,"degenerate triangle flagged");
        yes(Double.isNaN(degenerate.centroidX)&&Double.isNaN(degenerate.centroidY)&&
            Double.isNaN(degenerate.centroidZ),"degenerate surface must not fabricate center");
        Cad3dSurfaceMetrics.Result empty=Cad3dSurfaceMetrics.measure(new float[]{},new int[]{});
        yes(empty.area==0d&&Double.isNaN(empty.centroidX),"empty surface centroid unavailable");
        // Float subtraction could overflow, but double subtraction must not.
        float max=Float.MAX_VALUE;
        Cad3dSurfaceMetrics.Result huge=Cad3dSurfaceMetrics.measure(
            new float[]{max,max,0,-max,max,0,max,-max,0},
            new int[]{0,1,2});
        yes(Double.isFinite(huge.area)&&huge.area>1e76,
            "large finite model area kept in double precision");
        fails(triangle,new int[]{0,1,3},"invalid mesh index");
        fails(new float[]{0,0,0, Float.NaN,0,0, 0,1,0},
              new int[]{0,1,2},"NaN coordinate");
        fails(new float[]{0,0,0, Float.POSITIVE_INFINITY,0,0, 0,1,0},
              new int[]{0,1,2},"infinite coordinate");
        fails(triangle,new int[]{0,1},"incomplete index tuple");
        fails(new float[]{0,0},new int[]{}, "incomplete XYZ triplet");
        System.out.println("CAD area-weighted surface centroid: open triangle, cube, translated model and invalid input PASS");
    }
}
