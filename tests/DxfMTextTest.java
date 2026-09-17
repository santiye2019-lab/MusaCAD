import com.musa.cad.DxfMText;

public class DxfMTextTest {
    private static int cases;
    private static void ok(boolean value,String name){cases++;if(!value)throw new AssertionError(name);}
    private static void eq(String expected,String actual,String name){cases++;if(!expected.equals(actual))throw new AssertionError(name+": "+actual+" != "+expected);}
    public static void main(String[] args){
        DxfMText.Result a=DxfMText.parse("A\\P{\\H2x;B}C");
        eq("A\nBC",a.plainText(),"plain");
        ok(a.runs.size()>=3,"runs");
        boolean tall=false;for(DxfMText.Run r:a.runs)if(r.text.contains("B")&&Math.abs(r.heightScale-2d)<1e-9)tall=true;
        ok(tall,"height");
        DxfMText.Result b=DxfMText.parse("\\FArial|b0|i0;X\\C1;R\\LAlt\\l");
        boolean font=false,color=false,under=false;
        for(DxfMText.Run r:b.runs){
            if(r.text.contains("X")&&"Arial".equals(r.font))font=true;
            if(r.text.contains("R")&&r.aci==1)color=true;
            if(r.text.contains("Alt")&&r.underline)under=true;
        }
        ok(font,"font");ok(color,"aci");ok(under,"underline");
        DxfMText.Result fonts=DxfMText.parse("\\FArial;A{\\Fromans.shx;B}");
        boolean arialNormal=false,shx=false;for(DxfMText.Run r:fonts.runs){if(r.text.contains("A")&&!r.usesShxFont())arialNormal=true;if(r.text.contains("B")&&r.usesShxFont())shx=true;}
        ok(arialNormal,"Arial family is not SHX");ok(shx,".shx detected");
        eq("1/2 ±2° Ø50",DxfMText.parse("\\S1#2; %%p2%%d %%c50").plainText(),"specials");
        DxfMText.Result c=DxfMText.parse("{\\W0.75;Dar}Normal");
        boolean narrow=false;for(DxfMText.Run r:c.runs)if(r.text.contains("Dar")&&Math.abs(r.widthScale-.75d)<1e-9)narrow=true;
        ok(narrow,"width");
        System.out.println(cases+" rich MTEXT cases passed");
    }
}
