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

        CadEdit copy=poly.copy();poly.xy[0]=99;if(copy.xy[0]!=0)throw new AssertionError("copy geometry shared");

        // Editing must not silently reset a custom CAD layer to the default.
        // These are real drawing operations, not command-name recognition.
        CadEdit layerLine=CadEdit.line(0,0,10,0).withLayerOverride("MEKANIK-TEMIZ-SU");
        keepLayer(layerLine.copy(),"copy");
        keepLayer(layerLine.translated(2,3),"MOVE");
        keepLayer(layerLine.rotated(30,0,0),"ROTATE");
        keepLayer(layerLine.scaled(2,0,0),"SCALE");
        keepLayer(layerLine.mirrored(true,0,0),"MIRROR");
        keepLayer(layerLine.offset(2),"OFFSET LINE");
        keepLayer(layerLine.stretchedVertex(1,12,4),"STRETCH LINE");
        keepLayer(CadEdit.circle(2,3,4,3).withLayerOverride("MEKANIK-TEMIZ-SU").offset(2),"OFFSET CIRCLE");
        keepLayer(CadEdit.rectangle(0,0,5,5).withLayerOverride("MEKANIK-TEMIZ-SU").offset(1),"OFFSET RECTANGLE");
        keepLayer(CadEdit.rectangle(0,0,5,5).withLayerOverride("MEKANIK-TEMIZ-SU").stretchedVertex(2,8,9),"STRETCH RECTANGLE");
        keepLayer(CadEdit.polyline(new float[]{0,0,1,1,2,2},true).withLayerOverride("MEKANIK-TEMIZ-SU").stretchedVertex(1,3,3),"STRETCH POLYLINE");
        keepLayer(CadEdit.polyline(new float[]{0,0,1,1,2,2},true).withLayerOverride("MEKANIK-TEMIZ-SU").withClosed(false),"PEDIT CLOSE");
        CadEdit layerText=CadEdit.styledText(2,3,"SU",15,"ROMANS","monospace",true,5f,0.8f,5f,2).withLayerOverride("MEKANIK-TEMIZ-SU");
        keepLayer(layerText.withTextHeight(8),"text height");
        keepLayer(layerText.withTextStyle("STANDARD","sans",false,7),"text style");
        CadEdit movedStyled=layerText.translated(8,1).rotated(30,0,0).scaled(1.5f,0,0).mirrored(false,0,0);
        keepLayer(movedStyled,"chained edit");
        if(!"ROMANS".equals(movedStyled.textStyleName)||!movedStyled.textShx||movedStyled.textGenerationFlags!=2)
            throw new AssertionError("drawing annotation style changed during geometry transforms");
        CadEdit layeredBlock=CadEdit.insert("VANA",5,6,2,45).withLayerOverride("MEKANIK-TEMIZ-SU");
        keepLayer(layeredBlock.translated(2,3).rotated(90,0,0).scaled(2,0,0),"BLOCK INSERT transform");
        if(layerLine.layerOverride==null||!layerLine.layerOverride.equals("MEKANIK-TEMIZ-SU"))
            throw new AssertionError("source layer mutated by transformed copies");
        if(layerLine.withLayerOverride(null).layerOverride!=null)
            throw new AssertionError("explicitly clearing layer override must still work");

        System.out.println("CAD source edit transform and layer-preserving and text-style cases passed");
    }

    private static void keepLayer(CadEdit edit,String label){
        if(edit==null||!"MEKANIK-TEMIZ-SU".equals(edit.layerOverride))
            throw new AssertionError(label+" lost CAD source layer override");
    }

    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.01f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
