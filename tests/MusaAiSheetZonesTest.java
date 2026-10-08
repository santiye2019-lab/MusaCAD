import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiSheetZones;
import com.musa.cad.MusaAiEngineeringReview;
import java.util.*;

public final class MusaAiSheetZonesTest {
    private static MusaAiDrawingIndex.Item at(int id,String type,String layer,
                                               String label,double len,double x,double y){
        return new MusaAiDrawingIndex.Item(id,type,layer,label,len,Double.NaN,
            false,false,"",x,y);
    }
    private static void check(boolean ok,String message){
        if(!ok)throw new AssertionError(message);
    }
    public static void main(String[]args){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            at(1,"TEXT","MEK_YANGIN","Yangın pompası YP-1",Double.NaN,0,0),
            at(2,"LINE","MEK_YANGIN","",10,1,1),
            at(3,"TEXT","PIS_SU","DN100",Double.NaN,100,200),
            at(4,"LINE","PIS_SU","",20,99,199),
            at(5,"BLOCK","PIS_SU","Lavabo",Double.NaN,Double.NaN,Double.NaN)
        );
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",500,0,
            Arrays.asList("MEK_YANGIN","PIS_SU"),Arrays.asList("MEK_YANGIN","PIS_SU"),items,"m");
        MusaAiSheetZones.Result zones=MusaAiSheetZones.analyze(index);
        check(zones.positioned==4,"four coordinate-bearing items");
        check(zones.withoutCoordinates==1,"one unlocated block");
        check(zones.zones.size()==2,"two spatial sectors");
        check(zones.text.contains("X1/Y1"),"first sector labeled");
        check(zones.text.contains("X4/Y4"),"last sector labeled");
        check(zones.text.contains("örnek: Yangın pompası"),"source annotation visible");
        check(zones.text.contains("Yakınlık bağlantı"),"not claimed connected");
        check(zones.zones.get(0).sourceIds.size()==2,"unique source references");
        MusaAiEngineeringReview.Result reviewed=MusaAiEngineeringReview.analyze(index,"test.dwg");
        check(reviewed.text.contains("PAFTA KONUM BÖLGELERİ"),"integrated into report");
        check(reviewed.text.contains("tam tarama değil"),"sampling disclaimer preserved");
        MusaAiDrawingIndex unknown=new MusaAiDrawingIndex("Model",2,0,
            Arrays.asList("A"),Arrays.asList("A"),
            Arrays.asList(at(7,"TEXT","A","DN100",Double.NaN,Double.NaN,Double.NaN)));
        MusaAiSheetZones.Result absent=MusaAiSheetZones.analyze(unknown);
        check(absent.zones.isEmpty()&&absent.text.contains("bulunamadı"),
            "coordinates missing must not produce fabricated sectors");
        System.out.println("MusaAiSheetZonesTest OK");
    }
}
