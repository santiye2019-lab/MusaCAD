import com.musa.cad.DxfLeader;

public class DxfLeaderTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        close(DxfLeader.saneSize(2,100),2);
        close(DxfLeader.saneSize(50,100),30);
        close(DxfLeader.saneSize(0,100),8);
        double[] a=DxfLeader.arrow(0,0,10,0,2);
        if(a.length!=6)throw new AssertionError("arrow");
        close(a[0],0);close(a[1],0);close(a[2],2);close(a[4],2);
        if(!(a[3]>0&&a[5]<0))throw new AssertionError("arrow width");
        if(DxfLeader.arrow(0,0,0,0,2).length!=0)throw new AssertionError("degenerate");
        System.out.println("DXF LEADER cases passed");
    }
}
