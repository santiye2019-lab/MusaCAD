import com.musa.cad.DxfDimStyles;
import java.util.*;

public class DxfDimStylesTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args)throws Exception{
        List<String> tags=Arrays.asList(
            "0","SECTION","2","TABLES",
            "0","DIMSTYLE","2","STANDARD","40","2.0","41","2.5",
            "0","DIMSTYLE","2","SMALL","40","1.0","41","1.25",
            "0","ENDSEC","0","EOF");
        DxfDimStyles.Table t=DxfDimStyles.parse(tags);
        close(t.arrowSize("standard"),5.0);
        close(t.arrowSize("SMALL"),1.25);
        close(t.arrowSize("missing"),0);
        System.out.println("DXF DIMSTYLE cases passed");
    }
}
