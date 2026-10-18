import com.musa.cad.Cad3dFaceLighting;
import java.lang.reflect.Field;
/** Real JNI crosscheck, Android runtime forced before class initialization. */
public final class Cad3dFaceLightingNativeHostTest {
    public static void main(String[] args)throws Exception{
        System.setProperty("java.vm.name","Dalvik");
        Field field=Cad3dFaceLighting.class.getDeclaredField("ANDROID_RUNTIME");
        field.setAccessible(true);
        if(!field.getBoolean(null))throw new AssertionError("Native face shading bypassed");
        Cad3dFaceLightingTest.main(args);
        System.out.println("PASS: actual native JNI face-lighting path");
    }
}
