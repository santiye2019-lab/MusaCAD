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

        double[] fixed=DxfLeader.arrowSized(0,0,1,0,4);
        close(fixed[2],4);close(fixed[4],4);

        double[] straight=DxfLeader.path(new double[]{0,5,10},new double[]{0,10,0},false);
        if(straight.length!=6)throw new AssertionError("straight path must preserve vertices");
        close(straight[2],5);close(straight[3],10);

        double[] spline=DxfLeader.path(new double[]{0,5,10},new double[]{0,10,0},true);
        if(spline.length<=6)throw new AssertionError("spline leader should be smoothly sampled");
        close(spline[0],0);close(spline[1],0);close(spline[spline.length-2],10);close(spline[spline.length-1],0);
        if(Math.abs(spline[3]-spline[1])<1e-9)throw new AssertionError("spline leader must curve between vertices");

        double[] tangent=DxfLeader.path(new double[]{0,5,10},new double[]{0,10,0},true,1,0);
        if(tangent.length<=6)throw new AssertionError("tangent spline leader should be sampled");
        int last=tangent.length-2,prev=last-2;
        if(Math.abs(tangent[last+1]-tangent[prev+1])>.8)throw new AssertionError("stored horizontal end tangent should control spline landing direction");
        if(!(tangent[last]-tangent[prev]>0))throw new AssertionError("stored positive horizontal tangent should approach leader end from the left");

        double[] hook=DxfLeader.hook(10,5,1,0,true,2.5);
        if(hook.length!=4)throw new AssertionError("hook");close(hook[0],10);close(hook[1],5);close(hook[2],12.5);close(hook[3],5);
        double[] opposite=DxfLeader.hook(10,5,1,0,false,2.5);
        close(opposite[2],7.5);close(opposite[3],5);
        double[] angled=DxfLeader.hook(2,3,3,4,true,5);
        close(angled[2],5);close(angled[3],7);
        if(DxfLeader.hook(0,0,0,0,true,2).length!=0)throw new AssertionError("degenerate hook");

        if(DxfLeader.path(new double[]{0},new double[]{0},true).length!=0)throw new AssertionError("bad leader path");
        System.out.println("DXF LEADER cases passed");
    }
}
