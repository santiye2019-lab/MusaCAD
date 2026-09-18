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

        CadEdit styled=CadEdit.styledText(4,8,"KESIT",15,"ROMANS","monospace",true,22f,.8f,12f,2).translated(5,-3).rotated(90,9,5);
        if(!styled.hasTextStyle()||!"ROMANS".equals(styled.textStyleName)||!"monospace".equals(styled.textFamilyHint)||!styled.textShx)throw new AssertionError("text style identity lost");
        near(styled.textHeight,22f,"text height");near(styled.textWidthFactor,.8f,"text width");near(styled.textOblique,12f,"text oblique");if(styled.textGenerationFlags!=2)throw new AssertionError("text generation flags");near(styled.rotationDegrees,105f,"styled text rotation");
        CadEdit styledCopy=styled.copy();if(!styledCopy.hasTextStyle()||styledCopy.textGenerationFlags!=styled.textGenerationFlags)throw new AssertionError("styled copy metadata");
        CadEdit props=CadEdit.line(0,0,1,1).withCadProperties("MEKANIK",CadEdit.COLOR_ACI,3,"DASHED",70).translated(5,5).rotated(90,5,5).copy();
        if(!"MEKANIK".equals(props.layerName)||props.colorMode!=CadEdit.COLOR_ACI||props.colorValue!=3||!"DASHED".equals(props.lineTypeName)||props.lineWeight!=70)throw new AssertionError("CAD layer/color/lineweight properties lost");

        CadEdit copy=poly.copy();poly.xy[0]=99;if(copy.xy[0]!=0)throw new AssertionError("copy geometry shared");
        System.out.println("CAD source edit transform and text-style cases passed");
    }

    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.01f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
