import com.musa.cad.CadUcsFrame;
import java.lang.reflect.Field;
public final class CadUcsNativeHostTest {
    private static void close(double got,double want){
        if(!Double.isFinite(got)||Math.abs(got-want)>1e-8)
            throw new AssertionError(got+" != "+want);
    }
    private static void point(double[] p,double x,double y,double z){
        close(p[0],x);close(p[1],y);close(p[2],z);
    }
    private static void reject(String text){
        try{CadUcsFrame.parse(text);throw new AssertionError("Accepted "+text);}
        catch(IllegalArgumentException correct){}
    }
    public static void main(String[] args)throws Exception {
        System.setProperty("java.vm.name","Dalvik");
        Field active=CadUcsFrame.class.getDeclaredField("ANDROID_RUNTIME");
        active.setAccessible(true);
        if(!active.getBoolean(null))throw new AssertionError("UCS native bypassed");
        CadUcsFrame world=CadUcsFrame.world();
        point(world.toWorld(2,3,4),2,3,4);
        point(world.toLocal(2,3,4),2,3,4);
        point(world.move(1,5),0,5,0);
        CadUcsFrame front=CadUcsFrame.front();
        point(front.move(1,4),0,0,4);
        point(front.move(2,4),0,-4,0);
        CadUcsFrame right=CadUcsFrame.right();
        point(right.move(0,5),0,5,0);
        point(right.move(1,5),0,0,5);
        point(right.move(2,5),5,0,0);
        CadUcsFrame custom=CadUcsFrame.parse("10,20,30,0,2,0,0,0,3");
        point(custom.toWorld(1,2,3),13,21,32);
        point(custom.toLocal(13,21,32),1,2,3);
        point(custom.move(2,4),4,0,0);
        CadUcsFrame skew=CadUcsFrame.of(-5,6,7,1,1,1,-1,1,0);
        double[] w=skew.toWorld(2,3,4);
        point(skew.toLocal(w[0],w[1],w[2]),2,3,4);
        reject("0,0,0,0,0,0,0,1,0");
        reject("0,0,0,1,0,0,2,0,0");
        reject("0,0,0,1,0,0,NaN,1,0");
        reject("1000000001,0,0,1,0,0,0,1,0");
        double[] isolated=skew.xAxis();isolated[0]=500;
        if(skew.xAxis()[0]==500)throw new AssertionError("Mutable UCS frame");
        System.out.println("PASS: real JNI UCS build, WCS/local roundtrips and validation");
    }
}
