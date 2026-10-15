import com.musa.cad.CadUcsFrame;
import java.util.Arrays;

public final class CadUcsFrameTest {
    private static void near(double a,double b){if(!Double.isFinite(a)||Math.abs(a-b)>1e-8)throw new AssertionError(a+" != "+b);}
    private static void point(double[] a,double x,double y,double z){near(a[0],x);near(a[1],y);near(a[2],z);}
    private static void reject(String input){
        boolean failed=false;
        try{CadUcsFrame.parse(input);}
        catch(IllegalArgumentException expected){failed=true;}
        if(!failed)throw new AssertionError("Bad UCS accepted "+input);
    }
    public static void main(String[]args){
        CadUcsFrame w=CadUcsFrame.world();
        point(w.toWorld(2,3,4),2,3,4);
        point(w.toLocal(2,3,4),2,3,4);
        point(w.move(1,5),0,5,0);
        CadUcsFrame front=CadUcsFrame.front();
        point(front.move(1,4),0,0,4);
        point(front.move(2,4),0,-4,0);
        CadUcsFrame right=CadUcsFrame.right();
        point(right.move(0,5),0,5,0);
        point(right.move(1,5),0,0,5);
        point(right.move(2,5),5,0,0);
        CadUcsFrame custom=CadUcsFrame.parse("10, 20, 30, 0, 2, 0, 0, 0, 3");
        point(custom.toWorld(1,2,3),13,21,32);
        point(custom.toLocal(13,21,32),1,2,3);
        point(custom.move(0,4),0,4,0);
        point(custom.move(2,4),4,0,0);
        CadUcsFrame oblique=CadUcsFrame.of(-5,6,7,1,1,1,-1,1,0);
        double[] p=oblique.toWorld(2,3,4);
        point(oblique.toLocal(p[0],p[1],p[2]),2,3,4);
        near(oblique.xAxis()[0]*oblique.yAxis()[0]+oblique.xAxis()[1]*oblique.yAxis()[1]+oblique.xAxis()[2]*oblique.yAxis()[2],0);
        double[] isolated=oblique.xAxis();isolated[0]=500;
        if(oblique.xAxis()[0]==500)throw new AssertionError("UCS axes must be immutable");
        reject("0,0,0,0,0,0,0,1,0");
        reject("0,0,0,1,0,0,2,0,0");
        reject("0,0,0,1,0,0,NaN,1,0");
        reject("0,0,0,1,0,0,Infinity,1,0");
        reject("1000000001,0,0,1,0,0,0,1,0");
        reject("0,0,0,1,0,0,0,1");
        System.out.println("3D UCS orthonormalization, WCS conversions, local moves and invalid axes passed");
    }
}
