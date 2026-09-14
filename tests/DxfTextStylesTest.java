import com.musa.cad.DxfTextStyles;
import java.util.*;

public class DxfTextStylesTest {
    private static List<String> tags(String... v){return Arrays.asList(v);}
    private static void near(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        DxfTextStyles.Table t=DxfTextStyles.parse(tags(
            "0","SECTION","2","TABLES",
            "0","STYLE","2","ROMANS","70","0","40","0","41","0.8","50","12","71","2","3","romans.shx","4","",
            "0","STYLE","2","ARIAL","40","2.5","41","1","50","0","71","0","3","Arial.ttf",
            "0","ENDSEC","0","EOF"));
        DxfTextStyles.Entry r=t.get("romans");near(r.widthFactor,.8);near(r.oblique,12);if(r.generationFlags!=2)throw new AssertionError();
        if(!"serif".equals(r.familyHint()))throw new AssertionError(r.familyHint());
        DxfTextStyles.Entry a=t.get("ARIAL");near(a.fixedHeight,2.5);if(!"sans-serif".equals(a.familyHint()))throw new AssertionError(a.familyHint());
        if(t.get("missing")==null)throw new AssertionError("STANDARD fallback missing");
        if(!"monospace".equals(DxfTextStyles.fontFamilyHint("simplex.shx")))throw new AssertionError();
        System.out.println("DXF text style cases passed");
    }
}
