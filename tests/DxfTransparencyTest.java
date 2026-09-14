import com.musa.cad.DxfTransparency;
import java.util.*;

public class DxfTransparencyTest {
    private static void eq(int a,int b){if(a!=b)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        Map<String,Integer> layers=new HashMap<>();layers.put("A",128);
        eq(DxfTransparency.layerOpacity(0x020000ffL),255);
        eq(DxfTransparency.layerOpacity(0x02000019L),25);
        eq(DxfTransparency.opacity(DxfTransparency.resolve(-1,"A",null),layers),128);
        DxfTransparency.Ref parent=DxfTransparency.resolve(0x02000066L,"A",null);
        eq(DxfTransparency.opacity(DxfTransparency.resolve(0x01000000L,"0",parent),layers),102);
        eq(DxfTransparency.apply(0xff112233,128),0x80112233);
        System.out.println("5 DXF transparency cases passed");
    }
}
