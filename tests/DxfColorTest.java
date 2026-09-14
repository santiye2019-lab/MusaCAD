import com.musa.cad.DxfColor;
import java.util.*;

public class DxfColorTest {
    public static void main(String[] args){
        if(DxfColor.aciArgb(1)!=0xFFFF0000)throw new AssertionError("ACI red");
        if(DxfColor.aciArgb(2)!=0xFFFFFF00)throw new AssertionError("ACI yellow");
        if(DxfColor.aciArgb(3)!=0xFF00FF00)throw new AssertionError("ACI green");
        if(DxfColor.aciArgb(5)!=0xFF0000FF)throw new AssertionError("ACI blue");
        if(DxfColor.aciArgb(7)!=0xFFFFFFFF)throw new AssertionError("ACI 7 white");
        if(DxfColor.aciArgb(253)!=0xFF999999)throw new AssertionError("ACI 253 exact gray");
        Map<String,Integer> layers=new HashMap<>();layers.put("BORU",0xFF123456);
        if(DxfColor.argb(DxfColor.resolve(256,-1,"boru",null),layers)!=0xFF123456)throw new AssertionError("BYLAYER");
        DxfColor.Ref parent=DxfColor.resolve(1,-1,"0",null);
        if(DxfColor.argb(DxfColor.resolve(0,-1,"0",parent),layers)!=0xFFFF0000)throw new AssertionError("BYBLOCK");
        if(DxfColor.argb(DxfColor.resolve(256,0x336699,"0",null),layers)!=0xFF336699)throw new AssertionError("TrueColor");
        System.out.println("9 DXF color cases passed");
    }
}
