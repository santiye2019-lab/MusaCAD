import com.musa.cad.CadFontPolicy;

public class CadFontPolicyTest {
    private static int cases;
    private static void ok(boolean value,String name){cases++;if(!value)throw new AssertionError(name);}
    private static void eq(String expected,String actual,String name){cases++;if(!expected.equals(actual))throw new AssertionError(name+": "+actual+" != "+expected);}

    public static void main(String[] args){
        ok(CadFontPolicy.isShx("simplex.shx"),"simplex SHX");
        ok(CadFontPolicy.isShx("ROMANS"),"extensionless known SHX");
        ok(CadFontPolicy.knownShx("gothicg.shx"),"gothic SHX");
        ok(CadFontPolicy.isTtfOrOtf("Arial.TTF"),"TTF");
        ok(CadFontPolicy.isTtfOrOtf("Custom.otf"),"OTF");
        ok(!CadFontPolicy.isTtfOrOtf("simplex.shx"),"SHX not TTF");

        eq("serif",CadFontPolicy.shxFallbackFamily("romans.shx"),"roman fallback");
        eq("monospace",CadFontPolicy.shxFallbackFamily("simplex.shx"),"simplex fallback");
        eq("sans-serif-condensed",CadFontPolicy.androidFallbackFamily("Arial Narrow.ttf"),"narrow mapping");
        eq("serif",CadFontPolicy.androidFallbackFamily("Times New Roman.ttf"),"serif mapping");
        eq("monospace",CadFontPolicy.androidFallbackFamily("Consolas.ttf"),"mono mapping");

        eq("arial.ttf",CadFontPolicy.dxfFontFile("sans-serif",false),"sans DXF file");
        eq("times.ttf",CadFontPolicy.dxfFontFile("serif",false),"serif DXF file");
        eq("cour.ttf",CadFontPolicy.dxfFontFile("monospace",false),"mono DXF file");
        eq("simplex.shx",CadFontPolicy.dxfFontFile("simplex",true),"SHX DXF file");
        eq("CustomFont.otf",CadFontPolicy.dxfFontFile("/fonts/CustomFont.otf",false),"preserve OTF basename");

        String style=CadFontPolicy.dxfStyleName("My Custom Font.ttf");
        ok(style.startsWith("MUSA_"),"MusaCAD style prefix");
        ok(style.length()<=29,"style name bounded");
        eq("arial.ttf",CadFontPolicy.baseName("C:\\Fonts\\arial.ttf"),"Windows basename");
        System.out.println(cases+" CAD font policy cases passed");
    }
}
