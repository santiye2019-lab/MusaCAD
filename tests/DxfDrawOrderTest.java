import com.musa.cad.DxfDrawOrder;
import java.util.*;

public class DxfDrawOrderTest {
    public static void main(String[] args){
        List<String> tags=Arrays.asList(
            "0","SECTION","2","OBJECTS",
            "0","SORTENTSTABLE","5","AA","330","BB","100","AcDbSortentsTable","330","CC",
            "331","10","5","30","331","20","5","05",
            "0","ENDSEC","0","EOF"
        );
        Map<String,String> order=DxfDrawOrder.parse(tags);
        if(!"30".equals(order.get("10"))||!"05".equals(order.get("20")))throw new AssertionError(order.toString());
        if(DxfDrawOrder.compare("20","10",order)>=0)throw new AssertionError("sort handles must control order");
        if(DxfDrawOrder.compare("0A","0B",Collections.emptyMap())>=0)throw new AssertionError("entity handle fallback");
        System.out.println("3 DXF draw-order cases passed");
    }
}
