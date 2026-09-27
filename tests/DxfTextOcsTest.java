import com.musa.cad.DxfTextOcs;
import java.util.*;

public class DxfTextOcsTest {
    private static List<String> tags(Object... values){ArrayList<String> out=new ArrayList<>();for(Object v:values)out.add(String.valueOf(v));return out;}
    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.0001f)throw new AssertionError(name+" "+actual+" != "+expected);}
    private static void expect(boolean expected,boolean actual,String name){if(expected!=actual)throw new AssertionError(name+" expected "+expected+" got "+actual);}
    public static void main(String[] args){
        DxfTextOcs.Placement normal=DxfTextOcs.resolve(tags(),0,0,10,20,30,40,25,0);
        near(normal.x,10,"+Z x");near(normal.x2,30,"+Z x2");near(normal.angleDegrees,25,"+Z angle");if(normal.generationFlags!=0)throw new AssertionError("+Z flags");expect(true,normal.planar,"+Z planar");

        List<String> neg=tags(210,0,220,0,230,-1);
        DxfTextOcs.Placement mirrored=DxfTextOcs.resolve(neg,0,neg.size(),10,20,30,40,25,0);
        near(mirrored.x,-10,"-Z x");near(mirrored.y,20,"-Z y");near(mirrored.x2,-30,"-Z x2");near(mirrored.y2,40,"-Z y2");
        near(mirrored.angleDegrees,-25,"-Z angle");if(mirrored.generationFlags!=2)throw new AssertionError("-Z backward toggle");expect(true,mirrored.negativeZ,"-Z marker");

        DxfTextOcs.Placement doubleMirror=DxfTextOcs.resolve(neg,0,neg.size(),1,2,3,4,-170,2);
        if(doubleMirror.generationFlags!=0)throw new AssertionError("existing backward flag must toggle off");
        near(doubleMirror.angleDegrees,170,"normalized -Z angle");

        List<String> tilted=tags(210,1,220,0,230,1);
        DxfTextOcs.Placement keep=DxfTextOcs.resolve(tilted,0,tilted.size(),7,8,9,10,15,4);
        near(keep.x,7,"tilted preserve x");near(keep.angleDegrees,15,"tilted preserve angle");if(keep.generationFlags!=4)throw new AssertionError("tilted flags");expect(false,keep.planar,"tilted marker");
        System.out.println("DXF TEXT/ATTRIB planar OCS placement cases passed");
    }
}
