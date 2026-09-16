import com.musa.cad.DxfMPolygon;

public class DxfMPolygonTest {
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static void close(double a,double b,String message){if(Math.abs(a-b)>1e-9)throw new AssertionError(message+": "+a+" != "+b);}
    public static void main(String[] args){
        yes(DxfMPolygon.solid(1),"solid flag");yes(!DxfMPolygon.solid(0),"pattern flag");
        int entity=0x80446688;
        int red=DxfMPolygon.solidFillArgb(1,entity);
        yes((red>>>24)==0x80,"alpha preserved");yes((red&0x00ffffff)==0xff0000,"ACI red");
        yes(DxfMPolygon.solidFillArgb(256,entity)==entity,"BYLAYER fallback");
        yes(DxfMPolygon.solidFillArgb(0,entity)==entity,"BYBLOCK fallback");
        double[] shifted=DxfMPolygon.offset(new double[]{0,0,10,0,10,5},2,-3);
        close(shifted[0],2,"x0");close(shifted[1],-3,"y0");close(shifted[4],12,"x2");close(shifted[5],2,"y2");
        double[] unchanged=DxfMPolygon.offset(new double[]{1,2},Double.NaN,3);close(unchanged[0],1,"invalid offset x");close(unchanged[1],2,"invalid offset y");
        System.out.println("MPOLYGON display semantics passed");
    }
}
