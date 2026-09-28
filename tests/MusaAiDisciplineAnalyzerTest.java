import com.musa.cad.*;
import java.util.*;

public final class MusaAiDisciplineAnalyzerTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",9,0,
            Arrays.asList("S_KOLON","S_KIRIS","E_PANO","E_MECH_FEED","PEYZAJ_DRENAJ","ALTYAPI_ROGAR","ASANSOR_KUYU","YANGIN_SPRINKLER"),
            Arrays.asList("S_KOLON","S_KIRIS","E_PANO","E_MECH_FEED","PEYZAJ_DRENAJ","ALTYAPI_ROGAR","ASANSOR_KUYU","YANGIN_SPRINKLER"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","S_KOLON","KOLON",10,4),
                new MusaAiDrawingIndex.Item(2,"LINE","S_KIRIS","KİRİŞ REZERVASYON",8,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"TEXT","E_PANO","PANO",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(4,"TEXT","E_MECH_FEED","POMPA BESLEME",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(5,"LINE","PEYZAJ_DRENAJ","SULAMA DRENAJ",5,Double.NaN),
                new MusaAiDrawingIndex.Item(6,"TEXT","ALTYAPI_ROGAR","RÖGAR KOT BAĞLANTI",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(7,"TEXT","ASANSOR_KUYU","ASANSÖR KUYU DİBİ",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(8,"LINE","YANGIN_SPRINKLER","SPRINKLER",12,Double.NaN)
            ),"m");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiDisciplineAnalyzer.Result structural=MusaAiDisciplineAnalyzer.analyze(index(),"Statik projeyi kontrol et");
        if(!structural.matched)throw new AssertionError("structural not matched");
        has(structural.text,"STATİK PROJE KONTROLÜ");
        has(structural.text,"Rezervasyon / geçiş koordinasyonu");
        MusaAiDisciplineAnalyzer.Result electrical=MusaAiDisciplineAnalyzer.analyze(index(),"Elektrik projesini analiz et");
        has(electrical.text,"Disiplinler arası besleme koordinasyonu");
        MusaAiDisciplineAnalyzer.Result all=MusaAiDisciplineAnalyzer.analyze(index(),"Tam proje denetimi yap");
        has(all.text,"ÇOK DİSİPLİNLİ PROJE DENETİMİ");
        has(all.text,"ASANSÖR");has(all.text,"YANGIN VE CAN GÜVENLİĞİ");
        System.out.println("MusaAiDisciplineAnalyzerTest OK");
    }
}
