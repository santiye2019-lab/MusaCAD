import com.musa.cad.CadEdit;

public class CadEditTransformTest {
    public static void main(String[] args){
        CadEdit line=CadEdit.line(0,0,10,0);
        CadEdit moved=line.translated(5,7);
        near(moved.xy[0],5,"move x1");near(moved.xy[1],7,"move y1");near(moved.xy[2],15,"move x2");near(moved.xy[3],7,"move y2");

        CadEdit rotated=line.rotated(90,0,0);
        near(rotated.xy[0],0,"rotate x1");near(rotated.xy[1],0,"rotate y1");near(rotated.xy[2],0,"rotate x2");near(rotated.xy[3],10,"rotate y2");

        CadEdit poly=CadEdit.polyline(new float[]{0,0,10,0,10,10},true);
        if(!poly.closed)throw new AssertionError("closed polyline flag lost");
        if(poly.hitDistance(5,5)>5.01f)throw new AssertionError("polyline hit distance invalid");

        CadEdit circle=CadEdit.circle(10,10,15,10).translated(-5,2);
        near(circle.centerX(),5,"circle center x");near(circle.centerY(),12,"circle center y");
        if(circle.hitDistance(5,17)>0.01f)throw new AssertionError("circle hit test invalid");

        CadEdit text=CadEdit.text(4,8,"ODA",30).rotated(90,4,8);
        near(text.rotationDegrees,120,"text rotation");
        if(!"ODA".equals(text.text))throw new AssertionError("text lost");

        CadEdit copy=poly.copy();poly.xy[0]=99;if(copy.xy[0]!=0)throw new AssertionError("copy geometry shared");
        System.out.println("CAD source edit transform cases passed");
    }

    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.01f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
