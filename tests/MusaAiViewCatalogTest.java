import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiViewCatalog;
import java.util.*;

public final class MusaAiViewCatalogTest {
    private static MusaAiDrawingIndex.Item text(int id,String label,double x,double y){
        return new MusaAiDrawingIndex.Item(id,"TEXT","YAZI",label,
            Double.NaN,Double.NaN,false,false,"",x,y);
    }
    private static void check(boolean ok,String reason){
        if(!ok)throw new AssertionError(reason);
    }
    public static void main(String[]args){
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",200,0,
            Arrays.asList("YAZI"),Arrays.asList("YAZI"),Arrays.asList(
                text(1,"2. BODRUM KAT SIHHI TESISAT PLANI",10,10),
                text(2,"Zemin Kat Pis Su Planı",100,10),
                text(3,"1. Normal Kat Sıhhi Tesisat Planı",190,10),
                text(4,"Çatı Katı Havalandırma Planı",280,10),
                text(5,"A-A KESİTİ",15,100),
                text(6,"VAZİYET PLANI",115,100),
                text(7,"KUZEY CEPHE GÖRÜNÜŞÜ",200,100),
                text(8,"DETAY PLANI ÖLÇEK 1/20",300,100),
                text(9,"KOT: -3.20",10,28),
                text(10,"±0.00",100,32),
                text(11,"+3,50",190,32),
                text(12,"DN100 Ø100 %2 boru eğimi",230,55),
                text(13,"A-A KESİTİ",15,100),
                text(14,"BODRUM KAT PLANI",Double.NaN,Double.NaN)
            ),"mm");
        MusaAiViewCatalog.Result r=MusaAiViewCatalog.analyze(index);
        check(r.count(MusaAiViewCatalog.Kind.BASEMENT)==2,"two basement view headings");
        check(r.count(MusaAiViewCatalog.Kind.GROUND)==1,"ground floor recognized");
        check(r.count(MusaAiViewCatalog.Kind.FLOOR)==1,"typical floor recognized");
        check(r.count(MusaAiViewCatalog.Kind.ROOF)==1,"roof plan recognized");
        check(r.count(MusaAiViewCatalog.Kind.SECTION)==1,"section recognized and duplicate ignored");
        check(r.count(MusaAiViewCatalog.Kind.SITE)==1,"site plan recognized");
        check(r.count(MusaAiViewCatalog.Kind.ELEVATION)==1,"elevation view recognized");
        check(r.count(MusaAiViewCatalog.Kind.DETAIL)==1,"detail plan recognized");
        check(r.levels.size()==3,"three signed or named elevation candidates");
        check(r.report.contains("vaziyet planında"),"coordination prompts include site plan");
        check(r.report.contains("kot/düşey"),"coordination prompts include section levels");
        check(r.report.contains("BAŞLIK ENVANTERİDİR"),"report cannot imply visual audit");
        check(r.unpositionedViews==1,"missing coordinate signaled");
        check(MusaAiViewCatalog.classify("tip kat pis su tesisat plani")==MusaAiViewCatalog.Kind.FLOOR,
            "unnumbered typical floor plumbing title");
        check(MusaAiViewCatalog.classify("normal kat sihhi tesisat plani")==MusaAiViewCatalog.Kind.FLOOR,
            "unnumbered normal floor plumbing title");
        check(MusaAiViewCatalog.classify("bodrum pis su tesisat plani")==MusaAiViewCatalog.Kind.BASEMENT,
            "basement drawing with omitted kat word");
        check(MusaAiViewCatalog.classify("zemin temiz su tesisat plani")==MusaAiViewCatalog.Kind.GROUND,
            "ground floor drawing with omitted kat word");
        check(MusaAiViewCatalog.classify("deniz seviyesine gore kot 0 00")==null,"level note is not a view");
        check(MusaAiViewCatalog.classify("bd00 dn100 boru")==null,"pipe cannot become view");
        MusaAiDrawingIndex none=new MusaAiDrawingIndex("Model",0,0,
            Collections.emptyList(),Collections.emptyList(),Collections.emptyList());
        check(MusaAiViewCatalog.analyze(none).views.isEmpty(),"empty drawing safe");
        System.out.println("MusaAiViewCatalogTest OK");
    }
}
