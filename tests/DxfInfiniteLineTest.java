import com.musa.cad.DxfInfiniteLine;

public class DxfInfiniteLineTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    private static void length(double[] a,int n,String label){if(a.length!=n)throw new AssertionError(label+" length "+a.length);}
    public static void main(String[] args){
        double[] x=DxfInfiniteLine.clip(5,5,1,0,false,0,0,10,10);length(x,4,"xline");close(x[0],0);close(x[1],5);close(x[2],10);close(x[3],5);
        double[] r=DxfInfiniteLine.clip(5,5,1,0,true,0,0,10,10);length(r,4,"ray");close(r[0],5);close(r[1],5);close(r[2],10);close(r[3],5);
        double[] enter=DxfInfiniteLine.clip(-5,5,1,0,true,0,0,10,10);length(enter,4,"enter ray");close(enter[0],0);close(enter[2],10);
        length(DxfInfiniteLine.clip(-5,5,-1,0,true,0,0,10,10),0,"away ray");
        double[] diag=DxfInfiniteLine.clip(5,5,1,1,false,0,0,10,10);length(diag,4,"diag");close(diag[0],0);close(diag[1],0);close(diag[2],10);close(diag[3],10);
        length(DxfInfiniteLine.clip(20,5,0,1,false,0,0,10,10),0,"parallel outside");
        length(DxfInfiniteLine.clip(5,5,0,0,false,0,0,10,10),0,"zero direction");
        System.out.println("DXF RAY/XLINE clipping cases passed");
    }
}
