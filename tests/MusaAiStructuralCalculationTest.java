import com.musa.cad.*;
import java.util.*;

public final class MusaAiStructuralCalculationTest {
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiDrawingIndex drawing=new MusaAiDrawingIndex("Zemin",4,0,
            Arrays.asList("S_KOLON","S_KIRIS","S_TEMEL"),Arrays.asList("S_KOLON","S_KIRIS","S_TEMEL"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"TEXT","S_KOLON","K1 KOLON 30x60 C30 B420C Ø16 KOT +3.20"),
                new MusaAiDrawingIndex.Item(2,"TEXT","S_KIRIS","K12 KİRİŞ 25/50 C30 B420C Ø12"),
                new MusaAiDrawingIndex.Item(3,"TEXT","S_TEMEL","RADYE TEMEL C30 B420C")
            ),"cm");
        String report="STATİK HESAP RAPORU\nBeton sınıfı C35, donatı B420C. K1 kolon 30x60, K12 kiriş 25/50. Ø16 Ø12. Radye temel. Deprem spektrum SDS 1.0. Yük kombinasyonları G+Q. Modal periyot.";
        MusaAiStructuralCalculation.Result r=MusaAiStructuralCalculation.compare(drawing,"hesap.docx",report);
        has(r.text,"STATİK PROJE ↔ HESAP RAPORU KARŞILAŞTIRMASI");
        has(r.text,"Beton sınıfı — UYUŞMAZLIK ADAYI");
        has(r.text,"Donatı çeliği sınıfı — UYUMLU");
        has(r.text,"30X60");
        has(r.text,"Deprem / spektrum parametreleri: metin ipucu bulundu");
        if(r.mismatched<1)throw new AssertionError("expected mismatch");
        System.out.println("MusaAiStructuralCalculationTest OK");
    }
}
