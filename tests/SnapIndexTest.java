import com.musa.cad.SnapPoints;
import java.util.Random;
public class SnapIndexTest {
    private static void compare(float[] p)throws Exception{
        SnapPoints.Index index=new SnapPoints.Index(p);Random r=new Random(71);
        for(int i=0;i<300;i++){
            float x=r.nextFloat()*2600-100,y=r.nextFloat()*2600-100,scale=(float)Math.pow(10,r.nextDouble()*5-3),radius=r.nextFloat()*50+1;
            int a=SnapPoints.nearest(p,x,y,scale,radius),b=index.nearest(x,y,scale,radius);
            if(a!=b)throw new AssertionError(a+" != "+b);
        }
    }
    public static void main(String[] args)throws Exception{
        Random r=new Random(12);float[] points=new float[1000000];
        for(int i=0;i<points.length;i++)points[i]=r.nextFloat()*2400;
        compare(points);compare(new float[]{Float.NaN,2,4,4,4,4});compare(new float[0]);
        SnapPoints.Index tie=new SnapPoints.Index(new float[]{2,0,0,0});
        if(tie.nearest(1,0,1,1)!=0)throw new AssertionError("Tie order");
        if(tie.nearest(Float.NaN,0,1,1)!=-1||tie.nearest(0,0,0,1)!=-1)throw new AssertionError("Invalid input");
        float[] source={1,1};SnapPoints.Index copy=new SnapPoints.Index(source);source[0]=100;
        if(copy.coordinate(0)!=1)throw new AssertionError("Not immutable");
        Thread.currentThread().interrupt();
        try{new SnapPoints.Index(points);throw new AssertionError("Cancellation ignored");}
        catch(java.io.InterruptedIOException expected){}finally{Thread.interrupted();}
        System.out.println("900 index parity queries, tie order, invalid input, ownership and cancellation passed");
    }
}
