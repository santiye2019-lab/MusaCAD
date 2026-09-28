import com.musa.cad.*;
import java.util.*;

public final class MusaAiDetailedReportTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Zemin",4,0,
            Arrays.asList("S_KOLON","S_KIRIS","YANGIN_SPRINKLER"),
            Arrays.asList("S_KOLON","S_KIRIS","YANGIN_SPRINKLER"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","S_KOLON","KOLON",10,4),
                new MusaAiDrawingIndex.Item(2,"LINE","S_KIRIS","KİRİŞ REZERVASYON",8,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"LINE","YANGIN_SPRINKLER","SPRINKLER",12,Double.NaN)
            ),"m");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiEstimate.Document draft=MusaAiEstimate.fromDrawing(index(),"statik.dwg");
        MusaAiDetailedReport.Result statik=MusaAiDetailedReport.generate(index(),"statik.dwg","Statik detaylı rapor oluştur",null,draft);
        if(!statik.matched)throw new AssertionError("not matched");
        has(statik.text,"STATİK PROJE İNCELEME RAPORU");
        has(statik.text,"PROJE–KEŞİF UYGUNLUĞU");
        has(statik.text,"KOORDİNASYON / ÇAPRAZ KONTROL");

        MusaAiDetailedReport.Result all=MusaAiDetailedReport.generate(index(),"proje.dwg","Tam proje denetim raporu oluştur",null,draft);
        has(all.text,"TAM PROJE DENETİM VE UYGUNLUK RAPORU");
        has(all.text,"DİSİPLİNLER ARASI KOORDİNASYON");
        System.out.println("MusaAiDetailedReportTest OK");
    }
}
