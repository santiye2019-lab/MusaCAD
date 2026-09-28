import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiTableOleAnalysis;
import java.util.*;

public final class MusaAiTableOleAnalysisTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",8,1,
            Arrays.asList("LEJANT","TABLO","BORU"),Arrays.asList("LEJANT","TABLO","BORU"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"TEXT","LEJANT","MEKANİK LEJANT"),
                new MusaAiDrawingIndex.Item(2,"TEXT","LEJANT","VANA - KÜRESEL"),
                new MusaAiDrawingIndex.Item(3,"TEXT","LEJANT","POMPA - SİRKÜLASYON"),
                new MusaAiDrawingIndex.Item(4,"TEXT","TABLO","POZ NO"),
                new MusaAiDrawingIndex.Item(5,"TEXT","TABLO","MALZEME"),
                new MusaAiDrawingIndex.Item(6,"TEXT","TABLO","ADET"),
                new MusaAiDrawingIndex.Item(7,"LINE","BORU","")
            ),"mm",Arrays.asList(
                new MusaAiDrawingIndex.OleItem("EXCEL",true,true,"OOXML",
                    "— Sayfa 1 —\nA1\tMALZEME\nB1\tVANA\nC1\t12",1,3)
            ));
    }
    private static MusaAiTableOleAnalysis.Answer ask(String q){
        MusaAiTableOleAnalysis.Answer a=MusaAiTableOleAnalysis.answer(index(),q);
        if(!a.matched)throw new AssertionError("not matched "+q);return a;
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiTableOleAnalysis.Answer t=ask("tabloyu özetle");
        has(t.text,"Tablo adayı: var");has(t.text,"Gömülü yapısal Excel: 1");has(t.text,"POZ NO");
        if(t.sourceIds.size()<3)throw new AssertionError("table ids");

        MusaAiTableOleAnalysis.Answer l=ask("lejantı özetle");
        has(l.text,"Açık lejant başlığı: 1");has(l.text,"VANA - KÜRESEL");
        if(l.sourceIds.isEmpty())throw new AssertionError("legend ids");

        MusaAiTableOleAnalysis.Answer o=ask("OLE nesnelerini özetle");
        has(o.text,"OLE nesnesi: 1");has(o.text,"Excel: 1");has(o.text,"hücre: 3");

        MusaAiTableOleAnalysis.Answer s=ask("Excel'de vana var mı");
        has(s.text,"Eşleşen nesne: 1");

        if(MusaAiTableOleAnalysis.answer(index(),"kaç daire var").matched)throw new AssertionError("unrelated matched");
        System.out.println("MusaAiTableOleAnalysisTest OK");
    }
}
