import com.musa.cad.*;
import java.util.*;

public final class MusaAiAutoReportTest {
    private static MusaAiDrawingIndex base(){
        return new MusaAiDrawingIndex("Model",3,0,
            Arrays.asList("BORU","NOT"),Arrays.asList("BORU","NOT"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","BORU","",10,Double.NaN,false,false,"LINE|0,0;1000000,0|c=0",5,0),
                new MusaAiDrawingIndex.Item(2,"TEXT","NOT","REVİZE ET",Double.NaN,Double.NaN,false,false,"TEXT|0,2|c=0|t=revize et|r=0",0,2)
            ),"m");
    }
    private static MusaAiDrawingIndex current(){
        return new MusaAiDrawingIndex("Model",4,1,
            Arrays.asList("BORU","NOT","YENI"),Arrays.asList("BORU","NOT","YENI"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(10,"LINE","BORU","",10,Double.NaN,false,false,"LINE|0,0;1000000,0|c=0",5,0),
                new MusaAiDrawingIndex.Item(11,"TEXT","NOT","REVİZE ET",Double.NaN,Double.NaN,false,false,"TEXT|0,2|c=0|t=revize et|r=0",0,2),
                new MusaAiDrawingIndex.Item(12,"CIRCLE","YENI","",6.283,3.141,false,false,"CIRCLE|5,0;6,0|c=1",5.5,0)
            ),"m",Arrays.asList(
                new MusaAiDrawingIndex.OleItem("EXCEL",true,true,"OOXML","— Sayfa 1 —\nA1\tMALZEME\nB1\tVANA",1,2)
            ));
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        if(!MusaAiAutoReport.asksReport("proje raporu oluştur"))throw new AssertionError("report query");
        if(MusaAiAutoReport.asksReport("kaç daire var"))throw new AssertionError("unrelated query");
        MusaAiAutoReport.Result r=MusaAiAutoReport.generate(current(),"R02.dwg",base(),"R01.dwg");
        if(!r.matched)throw new AssertionError("not matched");
        has(r.text,"MUSACAD AI • OTOMATİK PROJE RAPORU");
        has(r.text,"1. METRAJ ÖZETİ");
        has(r.text,"2. CAD KALİTE KONTROLÜ");
        has(r.text,"3. GÖMÜLÜ BELGE / OLE");
        has(r.text,"Excel: 1");
        has(r.text,"4. REVİZYON DURUMU");
        has(r.text,"Referans: R01.dwg → Güncel: R02.dwg");
        has(r.text,"5. RAPOR SONUCU");
        if(r.sourceIds.isEmpty())throw new AssertionError("expected report highlights");

        MusaAiAutoReport.Result noBase=MusaAiAutoReport.generate(current(),"R02.dwg",null,"");
        has(noBase.text,"Referans revizyon tanımlı değil");
        System.out.println("MusaAiAutoReportTest OK");
    }
}
