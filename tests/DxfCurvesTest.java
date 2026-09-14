import com.musa.cad.DxfCurves;

public class DxfCurvesTest {
    private static void near(double actual,double expected,double tolerance,String label){
        if(Math.abs(actual-expected)>tolerance)throw new AssertionError(label+": "+actual+" != "+expected);
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
        System.out.println("DXF curve sampling tests passed");
    }
}
