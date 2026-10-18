import com.musa.cad.CadGizmoMath;
import com.musa.cad.CadUcsFrame;
import java.lang.reflect.Field;
/* Real host JNI proof; force Android pathway on an ordinary Linux JDK. */
public final class CadGizmoNativeHostTest {
    private static void near(double v,double want){
        if(!Double.isFinite(v)||Math.abs(v-want)>1e-8)
            throw new AssertionError(v+" != "+want);
    }
    public static void main(String[] args)throws Exception {
        System.setProperty("java.vm.name","Dalvik");
        Field nativeFlag=CadGizmoMath.class.getDeclaredField("ANDROID_RUNTIME");
        nativeFlag.setAccessible(true);
        if(!nativeFlag.getBoolean(null))throw new AssertionError("Native path inactive");
        CadUcsFrame world=CadUcsFrame.world();
        double[] x=CadGizmoMath.axisDirection(world,0,0,0);
        double[] y=CadGizmoMath.axisDirection(world,1,0,0);
        double[] z=CadGizmoMath.axisDirection(world,2,0,0);
        near(x[0],1);near(x[1],0);
        near(y[0],0);near(y[1],0);
        near(z[0],0);near(z[1],-1);
        double[][] axes={x,y,z};
        if(CadGizmoMath.pick(58,1,axes,58,16)!=0)throw new AssertionError("native X pick");
        if(CadGizmoMath.pick(1,-58,axes,58,16)!=2)throw new AssertionError("native Z pick");
        if(CadGizmoMath.pick(0,58,axes,58,16)!=-1)throw new AssertionError("hidden Y");
        if(CadGizmoMath.pick(100,100,axes,58,16)!=-1)throw new AssertionError("outside");
        near(CadGizmoMath.drawingDistance(100,0,1,0,10),10);
        near(CadGizmoMath.drawingDistance(0,-25,0,-1,5),5);
        if(!Double.isNaN(CadGizmoMath.drawingDistance(100,0,0.05,0,10)))
            throw new AssertionError("invalid foreshortening");
        double[][] invalid={{Double.NaN,Double.NaN},null,z};
        if(CadGizmoMath.pick(0,-58,invalid,58,16)!=2)
            throw new AssertionError("native skips invalid axes");
        System.out.println("PASS: same Android JNI math on Linux host, projection, pick, drag");
    }
}
