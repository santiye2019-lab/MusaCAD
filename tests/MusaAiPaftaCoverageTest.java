import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiViewCatalog;
import com.musa.cad.MusaAiPaftaCoverage;
import java.util.*;

public final class MusaAiPaftaCoverageTest {
    private static MusaAiDrawingIndex.Item label(int id,String text,double x,double y){
        return new MusaAiDrawingIndex.Item(id,"TEXT","BASLIK",text,
            Double.NaN,Double.NaN,false,false,"",x,y);
    }
    private static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}

    public static void main(String[] args){
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",1000,3,
            Arrays.asList("BASLIK"),Arrays.asList("BASLIK"),Arrays.asList(
                label(1,"Bodrum Kat Sıhhi Tesisat Planı",30,60),
                label(2,"Zemin Kat Pis Su Planı",120,60),
                label(3,"A-A KESİTİ",30,140),
                label(4,"Vaziyet Planı",120,140),
                label(5,"Çatı Katı Havalandırma Planı",Double.NaN,Double.NaN)
            ),"mm");
        MusaAiViewCatalog.Result names=MusaAiViewCatalog.analyze(index);
        String output=MusaAiPaftaCoverage.build(index,names,0,9,0,5,
            "Gemini HTTP 429 kota sınırı");
        check(output.contains("Bodrum Kat"),"basement is listed");
        check(output.contains("Zemin Kat"),"ground floor is listed");
        check(output.contains("A-A KESİTİ"),"section is listed");
        check(output.contains("Vaziyet Planı"),"site plan is listed");
        check(output.contains("Çatı Katı"),"roof is listed");
        check(output.contains("0/9"),"unreviewed tiles are not called inspected");
        check(output.contains("0/5"),"unreviewed closeups are not called inspected");
        check(output.contains("HTTP 429"),"quota blocking is visible");
        check(output.contains("kaynak 1"),"DWG source id appears");
        check(output.contains("konumu bilinmiyor"),"missing position is flagged");
        check(output.contains("tam sayısal denetim değildir"),"sample limitations explicit");
        check(output.contains("Pafta sınırı"),"title is not treated as complete sheet");
        check(!output.contains("9/9"),"does not invent visual success");
        String empty=MusaAiPaftaCoverage.build(index,
            MusaAiViewCatalog.analyze(new MusaAiDrawingIndex("Model",0,0,
                Collections.emptyList(),Collections.emptyList(),Collections.emptyList())),
            0,9,0,0,"");
        check(empty.contains("güvenilir şekilde tespit edilmedi"),"no fabricated sheets");
        System.out.println("MusaAiPaftaCoverageTest OK");
    }
}
