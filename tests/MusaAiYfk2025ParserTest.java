import com.musa.cad.MusaAiYfk2025Parser;
import java.util.*;

public final class MusaAiYfk2025ParserTest {
    private static void check(boolean truth,String why){
        if(!truth)throw new AssertionError(why);
    }
    public static void main(String[] args) {
        String sample="01.01.2025\n"+
            "Sıhhi Tesisat  Montajlı Birim Fiyat  Montaj Bedeli\n"+
            "N25.175.1401   Test amaçlı 100 litre cihaz                      17.593,75        2.793,75\n"+
            "25.175.1402   Test amaçlı 160 litre cihaz                      21.875,00        3.337,50\n";
        List<MusaAiYfk2025Parser.Item> rows=MusaAiYfk2025Parser.parsePage(sample,269);
        check(rows.size()==2,"One row per poz code");
        check(rows.get(0).code.equals("25.175.1401"),"OCR column prefix stripped");
        check(rows.get(0).price2025.equals("17.593,75"),"Installed price preserved textually");
        check(rows.get(0).mounting2025.equals("2.793,75"),"Mounting component separate");
        check(rows.get(0).description.contains("100 litre"),"Actual row label");
        check(rows.get(0).pdfPage==269,"Source page retained");
        check(MusaAiYfk2025Parser.pageCodes(sample).size()==2,"Structured item candidates");

        String civil="15.500.1101  Sentetik örnek yapı yüzeyi işlemi      m²     1.229,86\n"+
            "Devam eden şartname satırı; doğrulanmamış fiyat yok\n"+
            "15.500.1102  Sentetik örnek ikinci işlem         m²     820,70\n";
        List<MusaAiYfk2025Parser.Item> civilRows=MusaAiYfk2025Parser.parsePage(civil,100);
        check(civilRows.size()==2,"Civil row separation");
        check(civilRows.get(0).unit.equals("m²"),"Unit captured");
        check(civilRows.get(0).price2025.equals("1.229,86"),"Single amount captured");
        check(civilRows.get(0).mounting2025.isEmpty(),"No invented second price");

        String incomplete="25.400.1111  Pressure grade not shown\n"+
            "Tesisata ilişkin ek şartlar\n";
        List<MusaAiYfk2025Parser.Item> uncertain=
            MusaAiYfk2025Parser.parsePage(incomplete,10);
        check(uncertain.size()==1,"Unpriced row available");
        check(uncertain.get(0).price2025.isEmpty(),"Missing price remains empty");
        check(uncertain.get(0).unit.isEmpty(),"Missing unit remains unknown");
        check(MusaAiYfk2025Parser.wantsImport("2025 kitabını yükle"),"User selected import");
        check(MusaAiYfk2025Parser.wantsStatus("2025 katalog durumu"),"Offline state");
        check(MusaAiYfk2025Parser.wantsCompare("2025 2026 yeni pozları tara"),"Delta scan");
        check(MusaAiYfk2025Parser.wantsLookup("2025 poz 25.100.1005"),"Source lookup");
        check(!MusaAiYfk2025Parser.wantsLookup("2026 poz 25.100.1005"),"2026 intent retained");
        check(MusaAiYfk2025Parser.findCode("2025 poz 25.100.1005")
            .equals("25.100.1005"),"Precise code extraction");
        check(MusaAiYfk2025Parser.searchTerm("2025 kitabında lavabo ara")
            .equals("lavabo"),"Turkish folded text search");
        System.out.println("MusaAiYfk2025ParserTest OK");
    }
}
