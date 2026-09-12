import com.musa.cad.SnapPoints;

public class SnapPointsTest {
    private static void check(int expected,float[] p,float x,float y,float scale,float radius){
        int actual=SnapPoints.nearest(p,x,y,scale,radius);
        if(actual!=expected)throw new AssertionError(actual+" != "+expected);
    }
    public static void main(String[] args){
        float[] p={0,0,10,10,20,20};
        check(2,p,11,11,1,5);
        check(-1,p,100,100,1,5);
        check(2,p,13,10,1,5);
        check(-1,p,13,10,2,5);
        check(2,p,13,10,.5f,5);
        check(0,new float[]{0,0,2,0},1,0,1,1);
        check(-1,new float[0],1,1,1,10);
        check(-1,p,0,0,0,10);
        check(-1,p,Float.NaN,0,1,10);
        check(2,new float[]{Float.NaN,0,1,1},1,1,1,10);
        System.out.println("10 snap cases passed");
    }
}
