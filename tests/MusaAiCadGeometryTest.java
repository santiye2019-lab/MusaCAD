import com.musa.cad.MusaAiCadGeometry;
import java.util.Arrays;

public final class MusaAiCadGeometryTest {
    private static void near(double a,double b){if(Math.abs(a-b)>1e-6)throw new AssertionError(a+" != "+b);}
    private static void tight(double[] a,double...b){
        if(a==null||a.length!=b.length)throw new AssertionError("small LINE shape "+Arrays.toString(a));
        for(int i=0;i<a.length;i++)if(!Double.isFinite(a[i])||Math.abs(a[i]-b[i])>1e-12)
            throw new AssertionError("2D LINE precision mismatch at "+i+": "+a[i]+" != "+b[i]);
    }
    private static void arr(double[] a,double...b){
        if(a==null||a.length!=b.length)throw new AssertionError("array "+Arrays.toString(a));
        for(int i=0;i<a.length;i++)near(a[i],b[i]);
    }

    public static void main(String[]args){
        double[] target={0,0,10,0}, boundary={5,-5,5,5};
        arr(MusaAiCadGeometry.trimLine(target,boundary,1,0),5,0,10,0);
        arr(MusaAiCadGeometry.trimLine(target,boundary,9,0),0,0,5,0);

        arr(MusaAiCadGeometry.extendLine(new double[]{0,0,4,0},boundary),0,0,5,0);
        arr(MusaAiCadGeometry.extendLine(new double[]{6,0,10,0},boundary),5,0,10,0);
        if(MusaAiCadGeometry.extendLine(target,boundary)!=null)throw new AssertionError("already crossing line must not extend");

        // P2 source-edit bug: cross-products of 0.1 mm perpendicular lines
        // are only 2e-8 and must NOT be mistaken for parallelism.
        double[] shortLine={0,0,0.0001,0};
        double[] shortBoundary={0.00005,-0.0001,0.00005,0.0001};
        tight(MusaAiCadGeometry.intersectLines(shortLine,shortBoundary),
              0.00005,0,0.5,0.5);
        near(MusaAiCadGeometry.projectOnLine(0.00009,0,shortLine),0.9);
        if(!Double.isNaN(MusaAiCadGeometry.projectOnLine(0,0,new double[]{0,0,0,0})))
            throw new AssertionError("invalid projection must fail closed");
        tight(MusaAiCadGeometry.trimLine(shortLine,shortBoundary,0.00001,0),
              0.00005,0,0.0001,0);
        tight(MusaAiCadGeometry.trimLine(shortLine,shortBoundary,0.00009,0),
              0,0,0.00005,0);
        tight(MusaAiCadGeometry.extendLine(
              new double[]{0,0,0.00004,0},shortBoundary),
              0,0,0.00005,0);
        tight(MusaAiCadGeometry.extendLine(
              new double[]{0.00006,0,0.0001,0},shortBoundary),
              0.00005,0,0.0001,0);
        if(MusaAiCadGeometry.trimLine(shortLine,
                new double[]{0.00005,0.0002,0.00005,0.0004},0.00001,0)!=null)
            throw new AssertionError("boundary supporting line is not enough without a segment hit");
        if(MusaAiCadGeometry.trimLine(shortLine,
                new double[]{0,0,0.0001,0},0.00002,0)!=null)
            throw new AssertionError("collinear overlap needs dedicated topology, not a fake trim");
        if(MusaAiCadGeometry.extendLine(new double[]{0,0,0,0},shortBoundary)!=null)
            throw new AssertionError("degenerate line must not extend");
        if(MusaAiCadGeometry.extendLine(
                new double[]{0,0,1e8,0},new double[]{0,1,1e8,1.000000001})!=null)
            throw new AssertionError("near-parallel lines must not create unstable far-away vertices");

        arr(MusaAiCadGeometry.continuePath(new double[]{0,0,10,0},false,new double[]{15,0,20,5}),
            0,0,10,0,15,0,20,5);
        arr(MusaAiCadGeometry.continuePath(new double[]{0,0,10,0},true,new double[]{-5,0,-10,5}),
            -10,5,-5,0,0,0,10,0);

        if(!MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0},false))throw new AssertionError("open polyline");
        if(!MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0,1,1},true))throw new AssertionError("closed polyline");
        if(MusaAiCadGeometry.validPolyline(new double[]{0,0,1,0},true))throw new AssertionError("closed needs 3 points");

        // Corner rays are selected by user picks, not by LINE storage order.
        double[] horizontal={0,0,10,0},vertical={0,0,0,10};
        double[] chamfer=MusaAiCadGeometry.cornerLines(
            horizontal,vertical,5,0,0,5,2,false);
        tight(chamfer,10,0,2,0,0,10,0,2,1,1);
        double k=2-2/Math.sqrt(2);
        tight(MusaAiCadGeometry.cornerLines(horizontal,vertical,5,0,0,5,2,true),
              10,0,2,0,0,10,0,2,k,k);
        tight(MusaAiCadGeometry.cornerLines(
              new double[]{10,0,0,0},new double[]{0,10,0,0},
              5,0,0,5,2,false),10,0,2,0,0,10,0,2,1,1);
        tight(MusaAiCadGeometry.cornerLines(
              new double[]{-10,0,10,0},new double[]{0,-10,0,10},
              5,0,0,-5,2,false),10,0,2,0,0,-10,0,-2,1,-1);

        // An excessive FILLET radius or CHAMFER distance must NOT extend
        // past a source endpoint or create a zero-length kept section.
        for(boolean fillet:new boolean[]{false,true}){
            for(double large:new double[]{10,12}){
                if(MusaAiCadGeometry.cornerLines(horizontal,vertical,5,0,0,5,large,fillet)!=null)
                    throw new AssertionError("oversize corner should fail closed");
            }
            if(MusaAiCadGeometry.cornerLines(
                    new double[]{2,0,10,0},vertical,5,0,0,5,2,fillet)!=null)
                throw new AssertionError("external theoretical corner is unsupported");
            if(MusaAiCadGeometry.cornerLines(horizontal,vertical,0,0,0,0,2,fillet)!=null)
                throw new AssertionError("ambiguous picks on intersection must not alter geometry");
            if(MusaAiCadGeometry.cornerLines(horizontal,
                    new double[]{0,1,10,1},5,0,4,1,2,fillet)!=null)
                throw new AssertionError("parallel corner must fail");
            if(MusaAiCadGeometry.cornerLines(horizontal,vertical,5,0,0,5,
                    Double.POSITIVE_INFINITY,fillet)!=null)
                throw new AssertionError("nonfinite corner radius/distance must fail");
        }

        tight(MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},5,3),
              0,0,5,0,5,0,10,0);
        tight(MusaAiCadGeometry.breakLineAt(new double[]{10,0,0,0},5,3),
              10,0,5,0,5,0,0,0);
        tight(MusaAiCadGeometry.breakLineAt(new double[]{0,0,0.0001,0},0.00005,0),
              0,0,0.00005,0,0.00005,0,0.0001,0);
        if(MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},0,0)!=null||
           MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},10,0)!=null||
           MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},11,0)!=null)
            throw new AssertionError("BREAK at or outside endpoint is invalid");
        if(MusaAiCadGeometry.breakLineAt(new double[]{0,0,0,0},0,0)!=null)
            throw new AssertionError("zero-length source line is not breakable");
        if(MusaAiCadGeometry.breakLineAt(
              new double[]{1e9,1e9,1e9+0.0001,1e9},1e9+0.00005,1e9)!=null)
            throw new AssertionError("BREAK must reject float-collapsed source endpoints");
        if(MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},Double.NaN,0)!=null||
           MusaAiCadGeometry.breakLineAt(new double[]{0,0,10,0},
                                         Double.POSITIVE_INFINITY,0)!=null)
            throw new AssertionError("invalid BREAK pick must fail closed");

        System.out.println("MusaAiCadGeometryTest OK");
    }
}
