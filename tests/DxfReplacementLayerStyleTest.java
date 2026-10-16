import com.musa.cad.*;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

/**
 * Host JVM contract for BYLAYER 2D BREAK child rendering and independent
 * layer visibility. The Android Canvas integration remains a separate gate.
 */
public final class DxfReplacementLayerStyleTest {
    private static void check(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
    private static SourceReplacement part(int color,int colorMode,String layer,String type,int weight){
        return new SourceReplacement(7,new SourceRange(7,10,18),layer,color,
                colorMode,type,1.5d,weight,CadEdit.line(0,0,4,0));
    }
    public static void main(String[] args){
        Map<String,Integer> colors=new LinkedHashMap<>();
        colors.put("M-BORU",0xFF33CC44);
        colors.put("E-KUVVET",0xFFCC2244);
        Map<String,String> types=new LinkedHashMap<>();
        types.put("M-BORU","DASHED");
        types.put("E-KUVVET","CENTER");
        Map<String,Integer> weights=new LinkedHashMap<>();
        weights.put("M-BORU",35);
        weights.put("E-KUVVET",70);
        DxfReplacementLayerStyle resolver=
            new DxfReplacementLayerStyle(colors,types,weights);

        SourceReplacement mechanical=part(0xFF998877,
             SourceReplacement.COLOR_BYLAYER,"m-boru",DxfLineStyle.BYLAYER,35);
        SourceReplacement electrical=part(0xFF998877,
             SourceReplacement.COLOR_BYLAYER,"E-KUVVET",DxfLineStyle.BYLAYER,70);
        check(resolver.argb(mechanical)==0xFF33CC44,"BYLAYER must use actual mechanical layer color");
        check(resolver.argb(electrical)==0xFFCC2244,"BYLAYER must use actual electrical layer color");
        check("DASHED".equals(resolver.lineType(mechanical)),"mechanical layer linetype");
        check("CENTER".equals(resolver.lineType(electrical)),"electrical layer linetype");
        check(resolver.lineWeight(mechanical)==35&&resolver.lineWeight(electrical)==70,
              "independent lineweights");

        SourceReplacement explicit=part(0xFFAABBCC,
             SourceReplacement.COLOR_EXPLICIT,"M-BORU","CONTINUOUS",25);
        check(resolver.argb(explicit)==0xFFAABBCC,
              "explicit TrueColor must NOT inherit the layer color");
        check("CONTINUOUS".equals(resolver.lineType(explicit)),
              "explicit linetype must override layer");

        Set<String> visible=new HashSet<>(Arrays.asList("M-BORU"));
        check(resolver.visible(mechanical,"Model","Model",visible),
              "source layout plus target visible layer");
        check(!resolver.visible(electrical,"Model","Model",visible),
              "moved BREAK child to hidden layer must not render");
        check(!resolver.visible(mechanical,"Layout1","Model",visible),
              "paper-space replacement must not leak into Model");
        visible.add("E-KUVVET");
        check(resolver.visible(electrical,"Model","Model",visible),
              "child becomes visible after layer switched on");
        check(!resolver.visible(mechanical,"Model","Layout1",visible),
              "wrong layout must never render replacement");
        check(!resolver.visible(part(0xFF000000,SourceReplacement.COLOR_BYLAYER,
                           "NONEXISTENT",DxfLineStyle.BYLAYER,25),
                  "Model","Model",visible),"missing layer must fail closed");

        colors.put("M-BORU",0xFFFF0000);
        check(resolver.argb(mechanical)==0xFF33CC44,
              "copied layer table must be immutable across drawing mutations");
        check(resolver.argb(part(0xFF001122,SourceReplacement.COLOR_BYBLOCK,
              "M-BORU","BYBLOCK",25))==
              DxfColor.aciArgb(DxfColor.DEFAULT_ACI),
              "uninserted BYBLOCK has default ACI7, not stale source TrueColor");

        DxfReplacementLayerStyle empty=DxfReplacementLayerStyle.empty();
        check(empty.argb(mechanical)==DxfColor.aciArgb(DxfColor.DEFAULT_ACI),
              "missing palette must never use stale explicitly colored source");
        check("CONTINUOUS".equals(empty.lineType(mechanical)),
              "missing layer linetype falls back without inventing a pattern");
        check(!empty.visible(mechanical,"Model","Model",Collections.emptySet()),
              "missing visibility table must never leak hidden replacements");
        System.out.println("BREAK BYLAYER rendering, target-layer visibility and layout isolation OK");
    }
}
