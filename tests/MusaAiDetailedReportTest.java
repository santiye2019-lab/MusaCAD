import com.musa.cad.*;
import java.util.*;

public final class MusaAiDetailedReportTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Zemin",4,0,
            Arrays.asList("S_KOLON","S_KIRIS","YANGIN_SPRINKLER"),
            Arrays.asList("S_KOLON","S_KIRIS","YANGIN_SPRINKLER"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","S_KOLON","KOLON K1 30x60 C30 B420C Ø16 AKS A-1 KOT +3.20",10,4),
                new MusaAiDrawingIndex.Item(2,"LINE","S_KIRIS","KİRİŞ 25/50 REZERVASYON Ø12",8,Double.NaN),
                new MusaAiDrawingIndex.Item(3,"LINE","YANGIN_SPRINKLER","SPRINKLER",12,Double.NaN)
            ),"m");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiBoq.Model boq=MusaAiBoq.of("Yüklenen Keşif",Arrays.asList(
            new MusaAiBoq.Row("15.001","Betonarme kolon","m",9,"Statik keşif"),
            new MusaAiBoq.Row("25.001","Yangın sprinkler hattı","m",11,"Yangın keşfi")
        ));
        MusaAiStructuralCalc.Model calc=MusaAiStructuralCalc.parse("hesap.pdf",
            "C30 B420C K1 30x70\\nSDS 1.10 Zemin Sınıfı ZC\\nRADYE TEMEL");
        MusaAiDetailedReport.Result statik=MusaAiDetailedReport.generate(
            index(),"statik.dwg","Statik detaylı rapor oluştur",boq,calc);
        if(!statik.matched)throw new AssertionError("not matched");
        has(statik.text,"STATİK PROJE İNCELEME RAPORU");
        has(statik.text,"PROJE–KEŞİF UYGUNLUĞU");
        has(statik.text,"KOORDİNASYON / ÇAPRAZ KONTROL");
        has(statik.text,"STATİK TEKNİK VERİ ÖZETİ");
        has(statik.text,"kolon 1");
        has(statik.text,"kiriş 1");
        has(statik.text,"30X60");
        has(statik.text,"C30");
        has(statik.text,"B420C");
        has(statik.text,"Ø16");
        has(statik.text,"AKS A-1");
        has(statik.text,"+3.20");
        has(statik.text,"STATİK HESAP RAPORU ↔ DWG ÇAPRAZ KONTROLÜ");
        has(statik.text,"K1: rapor 30x70 • DWG 30x60");
        MusaAiDetailedReport.Result all=MusaAiDetailedReport.generate(index(),"proje.dwg","Tam proje denetim raporu oluştur",boq);
        has(all.text,"TAM PROJE DENETİM VE UYGUNLUK RAPORU");
        has(all.text,"DİSİPLİNLER ARASI KOORDİNASYON");
        System.out.println("MusaAiDetailedReportTest OK");
    }
}
