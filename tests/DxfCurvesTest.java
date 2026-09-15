import com.musa.cad.DxfCurves;

public class DxfCurvesTest {
    private static void near(double actual,double expected,double tolerance,String label){
        if(Math.abs(actual-expected)>tolerance)throw new AssertionError(label+": "+actual+" != "+expected);
    }
    private static int nearest(double[] packed,double x,double y){
        int best=0;double distance=Double.POSITIVE_INFINITY;
        for(int i=0;i+1<packed.length;i+=2){double d=Math.hypot(packed[i]-x,packed[i+1]-y);if(d<distance){distance=d;best=i;}}
        return best;
    }
    public static void main(String[] args){
        double[] semi=DxfCurves.sampleBulgePolyline(new double[]{0,10},new double[]{0,0},new double[]{1,0},false);
        if(semi.length<8)throw new AssertionError("bulge should be sampled");
        near(semi[0],0,1e-9,"bulge start x");near(semi[1],0,1e-9,"bulge start y");
        near(semi[semi.length-2],10,1e-9,"bulge end x");near(semi[semi.length-1],0,1e-9,"bulge end y");
        double minY=Double.POSITIVE_INFINITY;
        for(int i=1;i<semi.length;i+=2)minY=Math.min(minY,semi[i]);
        near(minY,-5,.08,"positive semicircle depth");

        double[] reverse=DxfCurves.sampleBulgePolyline(new double[]{0,10},new double[]{0,0},new double[]{-1,0},false);
        double maxY=Double.NEGATIVE_INFINITY;
        for(int i=1;i<reverse.length;i+=2)maxY=Math.max(maxY,reverse[i]);
        near(maxY,5,.08,"negative semicircle height");

        double[] nurbs=DxfCurves.sampleNurbs(2,new double[]{0,0,0,1,1,1},null,
            new double[]{0,5,10},new double[]{0,10,0});
        if(nurbs.length<20)throw new AssertionError("NURBS should be sampled");
        near(nurbs[0],0,1e-9,"NURBS start x");near(nurbs[1],0,1e-9,"NURBS start y");
        near(nurbs[nurbs.length-2],10,1e-9,"NURBS end x");near(nurbs[nurbs.length-1],0,1e-9,"NURBS end y");
        double best=Double.POSITIVE_INFINITY,bx=0,by=0;
        for(int i=0;i<nurbs.length;i+=2){double d=Math.abs(nurbs[i]-5);if(d<best){best=d;bx=nurbs[i];by=nurbs[i+1];}}
        near(bx,5,.2,"NURBS midpoint x");near(by,5,.2,"NURBS midpoint y");

        double[] fit=DxfCurves.sampleFitSpline(new double[]{0,5,10},new double[]{0,10,0},false);
        if(fit.length<=6)throw new AssertionError("fit-point spline should be smoothly sampled");
        near(fit[0],0,1e-9,"fit start x");near(fit[1],0,1e-9,"fit start y");
        near(fit[fit.length-2],10,1e-9,"fit end x");near(fit[fit.length-1],0,1e-9,"fit end y");
        int mid=nearest(fit,5,10);near(fit[mid],5,1e-9,"fit point x");near(fit[mid+1],10,1e-9,"fit point y");
        if(Math.abs(fit[3]-fit[1])<1e-9)throw new AssertionError("fit spline must not collapse to straight chords");

        double[] tangent=DxfCurves.sampleFitSpline(new double[]{0,5,10},new double[]{0,10,0},false,1,0,1,0);
        if(tangent.length<8)throw new AssertionError("tangent spline should be sampled");
        if(Math.abs(tangent[3]-tangent[1])>.8)throw new AssertionError("horizontal start tangent should keep early y movement small");

        double[] closed=DxfCurves.sampleFitSpline(new double[]{0,10,10,0},new double[]{0,0,10,10},true);
        if(closed.length<20)throw new AssertionError("closed fit spline should be sampled");
        near(closed[closed.length-2],closed[0],1e-9,"closed fit x");near(closed[closed.length-1],closed[1],1e-9,"closed fit y");
        System.out.println("DXF curve sampling tests passed");
    }
}
