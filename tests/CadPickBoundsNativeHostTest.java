import com.musa.cad.CadPickBounds;
import java.lang.reflect.Field;
/** Existing 24k deterministic selections, exercised through actual C JNI. */
public final class CadPickBoundsNativeHostTest {
    public static void main(String[] args)throws Exception{
        System.setProperty("java.vm.name","Dalvik");
        Field field=CadPickBounds.class.getDeclaredField("ANDROID_RUNTIME");
        field.setAccessible(true);
        if(!field.getBoolean(null))throw new AssertionError("JNI picking bypassed");
        CadPickBoundsTest.main(args);
        System.out.println("PASS: actual JNI conservative picking on deterministic fixtures");
    }
}
