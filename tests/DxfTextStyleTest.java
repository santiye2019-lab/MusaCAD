import com.musa.cad.DxfTextStyle;
import java.util.*;

public class DxfTextStyleTest {
    private static String tags(Object... values){
        StringBuilder s=new StringBuilder();
        for(Object v:values)s.append(v).append('\n');
        return s.toString();
    }

    public static void main(String[] args){
        String dxf=tags(
            0,"SECTION",2,"TABLES",0,"TABLE",2,"STYLE",
            0,"STYLE",2,"ROMANS",70,0,40,0,41,.8,50,12,71,2,3,"romans.shx",4,"",
            0,"STYLE",2,"EXTSHX",70,0,40,0,41,1,50,0,71,0,3,"simplex",4,"",
            0,"STYLE",2,"ARIAL",70,0,40,2.5,41,1.1,50,0,71,0,3,"arial.ttf",4,"bigfont.shx",
            0,"STYLE",2,"XDATAFONT",70,0,40,0,41,1,50,0,71,0,3,"",4,"",1001,"ACAD",1000,"Arial",
            0,"STYLE",2,"XDATASHX",70,0,40,0,41,1,50,0,71,0,3,"",4,"",1001,"ACAD",1000,"simplex.shx",
            0,"ENDTAB",0,"ENDSEC",0,"EOF"
        );

        Map<String,DxfTextStyle.Style> styles=DxfTextStyle.parse(Arrays.asList(dxf.split("\n")));

        DxfTextStyle.Style romans=DxfTextStyle.resolve(styles,"romans");
        if(!romans.usesShx()||!"monospace".equals(romans.familyHint())||!"monospace".equals(romans.androidFamilyHint()))throw new AssertionError("SHX fallback");
        near(romans.width(2f),1.6f,"width");
        near(romans.oblique(0f),12f,"oblique");
        if(romans.generation(4)!=6)throw new AssertionError("generation flags");

        DxfTextStyle.Style extensionlessShx=DxfTextStyle.resolve(styles,"EXTSHX");
        if(!extensionlessShx.usesShx()||!"monospace".equals(extensionlessShx.familyHint()))throw new AssertionError("extensionless SHX");

        DxfTextStyle.Style arial=DxfTextStyle.resolve(styles,"ARIAL");
        if(arial.usesShx()||!"arial".equals(arial.familyHint())||!"sans-serif".equals(arial.androidFamilyHint()))throw new AssertionError("TTF family");
        near(arial.textHeight(8f),2.5f,"fixed height");
        near(arial.width(1f),1.1f,"style width");

        DxfTextStyle.Style xdata=DxfTextStyle.resolve(styles,"XDATAFONT");
        if(xdata.usesShx()||!"Arial".equals(xdata.familyHint()))throw new AssertionError("extended font family");

        DxfTextStyle.Style xdataShx=DxfTextStyle.resolve(styles,"XDATASHX");
        if(!xdataShx.usesShx()||!"monospace".equals(xdataShx.familyHint()))throw new AssertionError("extended SHX file");

        if(!"STANDARD".equals(DxfTextStyle.resolve(styles,"missing").name))throw new AssertionError("standard fallback");
        if(!"serif".equals(DxfTextStyle.androidFamilyHint("Cambria",false)))throw new AssertionError("Cambria serif fallback");
        if(!"sans-serif".equals(DxfTextStyle.androidFamilyHint("Calibri",false)))throw new AssertionError("Calibri sans fallback");
        if(!"monospace".equals(DxfTextStyle.androidFamilyHint("Consolas",false)))throw new AssertionError("Consolas mono fallback");
        if(!"sans-serif-condensed".equals(DxfTextStyle.androidFamilyHint("Arial Narrow",false)))throw new AssertionError("narrow fallback");
        if(!"My CAD Font".equals(DxfTextStyle.androidFamilyHint("My CAD Font",false)))throw new AssertionError("custom installed family preserved");
        System.out.println("DXF text style/font fallback cases passed");
    }

    private static void near(float actual,float expected,String name){
        if(Math.abs(actual-expected)>.001f)throw new AssertionError(name+": "+actual+" != "+expected);
    }
}
