import com.musa.cad.MusaAiEngineeringReview;
import com.musa.cad.MusaAiDrawingIndex;
import java.util.*;

public final class MusaAiEngineeringReviewTest {
    private static void has(String hay,String needle){
        if(!hay.contains(needle))throw new AssertionError("missing '"+needle+"' in:\n"+hay);
    }
    private static MusaAiDrawingIndex.Item item(int id,String type,String layer,String text,double len){
        return new MusaAiDrawingIndex.Item(id,type,layer,text,len,Double.NaN);
    }
    public static void main(String[]args){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            item(1,"TEXT","MEK_YANGIN","YANGIN POMPASI YP-1 Q=20 m3/h H=45 mSS 11kW",Double.NaN),
            item(2,"TEXT","MEK_YANGIN","JOKEY POMPA JP-1",Double.NaN),
            item(3,"TEXT","HAVALANDIRMA","40x30 dikdörtgen hava kanalı",Double.NaN),
            item(4,"TEXT","HAVALANDIRMA","Ø200 SPIRO kanal",Double.NaN),
            item(5,"LINE","PIS_SU_DN100","",15),
            item(6,"LWPOLYLINE","PIS_SU_DN100","",5),
            item(7,"TEXT","PIS_SU_DN100","Ø100 Eğim %2",Double.NaN),
            item(8,"TEXT","SIHHI_CIHAZ","LAVABO",Double.NaN),
            item(9,"TEXT","MEK_YANGIN","HİDROFOR HF-1",Double.NaN),
            item(10,"LINE","HAVALANDIRMA","",12),
            item(11,"LINE","GENEL","",200)
        );
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",3000,0,
            Arrays.asList("PIS_SU_DN100","HAVALANDIRMA","MEK_YANGIN","SIHHI_CIHAZ","GENEL"),
            Arrays.asList("PIS_SU_DN100","HAVALANDIRMA","MEK_YANGIN","SIHHI_CIHAZ","GENEL"),items,"m");
        MusaAiEngineeringReview.Result r=MusaAiEngineeringReview.analyze(index,"mekanik.dwg");
        has(r.text,"MÜHENDİSLİK PAFTA ÖN İNCELEMESİ");
        has(r.text,"Yangın Pompası");
        has(r.text,"Q=20 m3/h");
        has(r.text,"H=45 mSS");
        has(r.text,"40×30 dikdörtgen kanal");
        has(r.text,"Ø200");
        has(r.text,"DN100");
        has(r.text,"Pis su: 20.00 m");
        has(r.text,"Havalandırma: 12.00 m");
        has(r.text,"Jokey Pompa");
        has(r.text,"Q/H değerleri bu etiket üzerinde okunamadı");
        has(r.text,"tam tarama değil");
        if(!r.sourceIds.contains(1)||!r.sourceIds.contains(5))
            throw new AssertionError("source evidence lost");
        if(r.equipmentLabels<3||r.dimensions<3||r.measuredRuns!=3)
            throw new AssertionError("technical extraction counts incorrect");
        MusaAiDrawingIndex unknown=new MusaAiDrawingIndex("Model",2,0,
            Arrays.asList("PIS_SU"),Arrays.asList("PIS_SU"),
            Arrays.asList(item(42,"LINE","PIS_SU","",700),
                          item(43,"TEXT","PIS_SU","DN75",Double.NaN)));
        MusaAiEngineeringReview.Result noUnits=MusaAiEngineeringReview.analyze(unknown,"units-unknown.dwg");
        has(noUnits.text,"birimi bilinmediği için metreye çevrilmedi");
        if(noUnits.text.contains("700.00 m"))throw new AssertionError("invented meter scale");
        if(!MusaAiEngineeringReview.asksReview("Projeyi mühendislik açısından analiz et"))
            throw new AssertionError("general engineering intent missed");
        if(!MusaAiEngineeringReview.asksReview("Havalandırma kanal kesitlerini analiz et"))
            throw new AssertionError("duct intent missed");
        if(MusaAiEngineeringReview.asksReview("Statik projeyi kontrol et"))
            throw new AssertionError("engineering review must not hijack structural expert");
        if(MusaAiEngineeringReview.asksReview("GMEKAI_FIRE projeyi derin analiz et"))
            throw new AssertionError("Gandalf deep route hijacked");
        System.out.println("MusaAiEngineeringReviewTest OK");
    }
}
