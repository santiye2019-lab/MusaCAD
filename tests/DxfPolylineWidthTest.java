import com.musa.cad.DxfPolylineWidth;

public class DxfPolylineWidthTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-8)throw new AssertionError(a+" != "+b);}
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    public static void main(String[] args){
        close(DxfPolylineWidth.constant(-20),20);
        close(DxfPolylineWidth.constant(Double.NaN),0);
        close(DxfPolylineWidth.uniform(5,5.0000001),5.00000005);
        close(DxfPolylineWidth.uniform(5,7),0);
        close(DxfPolylineWidth.uniform(5,0),0);
        close(DxfPolylineWidth.device(3,2,2),6);
        close(DxfPolylineWidth.device(20,1.5,.5),20);
        close(DxfPolylineWidth.device(0,2,2),0);
        close(DxfPolylineWidth.device(1,0,0),0);

        double[][] taper=DxfPolylineWidth.variable(new double[]{0,10},new double[]{0,0},new double[]{0,0},new double[]{2,0},new double[]{4,0},false);
        yes(taper.length==1,"straight taper should create one strip");
        close(taper[0][0],0);close(taper[0][1],1);close(taper[0][2],10);close(taper[0][3],2);
        close(taper[0][4],10);close(taper[0][5],-2);close(taper[0][6],0);close(taper[0][7],-1);

        double[][] corner=DxfPolylineWidth.variable(new double[]{0,10,10},new double[]{0,0,10},new double[]{0,0,0},
            new double[]{2,2,0},new double[]{2,2,0},false);
        yes(corner.length==4,"two strips plus two join triangles expected");
        yes(corner[2].length==6&&corner[3].length==6,"join triangles must be packed xyz pairs");

        double[][] arc=DxfPolylineWidth.variable(new double[]{0,10},new double[]{0,0},new double[]{1,0},new double[]{2,0},new double[]{4,0},false);
        yes(arc.length==1&&arc[0].length>8,"bulge width should sample the curved strip");
        for(double v:arc[0])yes(Double.isFinite(v),"arc strip must stay finite");

        double[][] zero=DxfPolylineWidth.variable(new double[]{0,10},new double[]{0,0},null,new double[]{0,0},new double[]{0,0},false);
        yes(zero.length==0,"zero widths should not create fill geometry");
        yes(DxfPolylineWidth.hasVariable(new double[]{0,2},new double[]{0,0}),"positive per-vertex width should be detected");
        yes(!DxfPolylineWidth.hasVariable(new double[]{0,0},new double[]{0,0}),"all zero widths are not geometric width");
        System.out.println("DXF geometric polyline width cases passed");
    }
}
