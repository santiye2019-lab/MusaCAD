import com.musa.cad.DxfUnits;
import java.util.*;

public class DxfUnitsTest {
    private static void eq(int a,int b){if(a!=b)throw new AssertionError(a+" != "+b);}
    private static void eq(String a,String b){if(!a.equals(b))throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        List<String> full=Arrays.asList("0","SECTION","2","HEADER","9","$ACADVER","1","AC1032","9","$INSUNITS","70","4","9","$LTSCALE","40","1","0","ENDSEC","0","EOF");
        eq(4,DxfUnits.parse(full));eq("mm",DxfUnits.symbol(4));eq("milimetre",DxfUnits.name(4));
        List<String> record=Arrays.asList("2","HEADER","9","$LTSCALE","40","2","9","$INSUNITS","70","6");
        eq(6,DxfUnits.parseHeaderRecord(record));eq("m",DxfUnits.symbol(6));
        eq(0,DxfUnits.parse(Arrays.asList("0","SECTION","2","HEADER","9","$INSUNITS","70","0","0","ENDSEC")));
        if(DxfUnits.known(0)||DxfUnits.known(25)||!DxfUnits.known(24))throw new AssertionError("known unit range");
        System.out.println("8 DXF unit metadata cases passed");
    }
}
