import com.musa.cad.CadEdit;
public final class CadPolylineOffsetTest {
    private static void check(boolean valid,String what){if(!valid)throw new AssertionError(what);}
    private static void near(float a,double b,String what){if(Math.abs(a-b)>0.0001)throw new AssertionError(what+"="+a+" expected "+b);}
    private static void points(CadEdit p,float[] expected,String what){
        check(p!=null&&p.type==CadEdit.Type.POLYLINE,what+" must return a polyline");
        check(p.xy.length==expected.length,what+" size");
        for(int i=0;i<expected.length;i++)near(p.xy[i],expected[i],what+"["+i+"]");
    }
    public static void main(String[] args){
        CadEdit l=CadEdit.polyline(new float[]{0,0,10,0,10,10}).withLayerOverride("TESISAT-SU");
        CadEdit left=l.offset(2);
        points(left,new float[]{0,2,8,2,8,10},"left open L");
        check(!left.closed&&"TESISAT-SU".equals(left.layerOverride),"open path type/layer");
        points(l.offset(-2),new float[]{0,-2,12,-2,12,10},"right open L");
        check(l.xy[1]==0&&l.xy[2]==10,"non-destructive original");
        CadEdit square=CadEdit.polyline(new float[]{0,0,10,0,10,10,0,10},true).withLayerOverride("TESISAT-SU");
        points(square.offset(2),new float[]{2,2,8,2,8,8,2,8},"closed inner");
        points(square.offset(-2),new float[]{-2,-2,12,-2,12,12,-2,12},"closed outer");
        check(square.offset(6)==null,"cannot reverse collapsed inset edges");
        check(square.offset(Float.NaN)==null&&square.offset(0)==null,"invalid offset");
        check("TESISAT-SU".equals(square.offset(-2).layerOverride),"closed layer preserved");
        CadEdit clockwise=CadEdit.polyline(new float[]{0,0,0,10,10,10,10,0},true);
        points(clockwise.offset(2),new float[]{-2,-2,-2,12,12,12,12,-2},"clockwise left is outside");
        CadEdit wcs=CadEdit.polyline(new float[]{1000000,1000000,1000010,1000000,1000010,1000010});
        points(wcs.offset(2),new float[]{1000000,1000002,1000008,1000002,1000008,1000010},"large WCS");
        check(CadEdit.polyline(new float[]{0,0,10,10,0,10,10,0},true).offset(1)==null,"bow tie source rejected");
        check(CadEdit.polyline(new float[]{0,0,10,0,10,0,10,10}).offset(1)==null,"zero-length segment rejected");
        check(CadEdit.polyline(new float[]{0,0,10,0,0,0}).offset(1)==null,"U-turn rejected");
        check(CadEdit.polyline(new float[]{0,0,10,0,10,10,0,5,0,10},true).offset(1)==null,
               "concave closed outline rejected pending a full intersection solver");
        check(CadEdit.polyline(new float[]{0,0,Float.NaN,1}).offset(1)==null,"nan rejected");
        System.out.println("CAD POLYLINE miter OFFSET: open, convex closed, both signs, WCS and geometry safety PASS");
    }
}
