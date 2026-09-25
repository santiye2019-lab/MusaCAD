import com.musa.cad.DxfTextStyle;
import java.util.*;

public class DxfTextStyleTest {
    private static String tags(Object... values){StringBuilder s=new StringBuilder();for(Object v:values)s.append(v).append('\n');return s.toString();}
    public static void main(String[] args){
        String dxf=tags(0,"SECTION",2,"TABLES",0,"TABLE",2,"STYLE",
            0,"STYLE",2,"ROMANS",70,0,40,0,41,.8,50,12,71,2,3,"romans.shx",4,"",
            0,"STYLE",2,"ARIAL",70,0,40,2.5,41,1.1,50,0,71,0,3,"arial.ttf",4,"bigfont.shx",
            0,"STYLE",2,"XDATAFONT",70,0,40,0,41,1,50,0,71,0,3,"",4,"",1001,"ACAD",1000,"Arial",
            0,"ENDTAB",0,"ENDSEC",0,"EOF");
        Map<String,DxfTextStyle.Style> styles=DxfTextStyle.parse(Arrays.asList(dxf.split("\n")));
        DxfTextStyle.Style romans=DxfTextStyle.resolve(styles,"romans");
        if(!romans.usesShx()||!"monospace".equals(romans.familyHint()))throw new AssertionError("SHX fallback");
        near(romans.width(2f),1.6f,"width");near(romans.oblique(0f),12f,"oblique");if(romans.generation(4)!=6)throw new AssertionError("generation flags");
        DxfTextStyle.Style arial=DxfTextStyle.resolve(styles,"ARIAL");
        if(arial.usesShx()||!"arial".equals(arial.familyHint()))throw new AssertionError("TTF family");near(arial.textHeight(8f),2.5f,"fixed height");near(arial.width(1f),1.1f,"style width");
        DxfTextStyle.Style xdata=DxfTextStyle.resolve(styles,"XDATAFONT");if(xdata.usesShx()||!"Arial".equals(xdata.familyHint()))throw new AssertionError("extended font family");
        if(!"STANDARD".equals(DxfTextStyle.resolve(styles,"missing").name))throw new AssertionError("standard fallback");
        System.out.println("DXF text style cases passed");
    }
    private static void near(float actual,float expected,String name){if(Math.abs(actual-expected)>.001f)throw new AssertionError(name+": "+actual+" != "+expected);}
}
