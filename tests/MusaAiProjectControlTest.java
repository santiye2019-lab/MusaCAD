import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiProjectControl;
import java.util.*;

public final class MusaAiProjectControlTest {
    private static MusaAiDrawingIndex index(){
        return new MusaAiDrawingIndex("Model",10,0,
            Arrays.asList("PIS_SU","NOTLAR","0","GIZLI"),
            Arrays.asList("PIS_SU","NOTLAR","0"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","PIS_SU","",5,Double.NaN,false,false,"LINE|0,0;500000,0|c=0"),
                new MusaAiDrawingIndex.Item(2,"LINE","PIS_SU","",5,Double.NaN,false,false,"LINE|0,0;500000,0|c=0"),
                new MusaAiDrawingIndex.Item(3,"LINE","PIS_SU","",0,Double.NaN,false,false,"LINE|0,0;0,0|c=0"),
                new MusaAiDrawingIndex.Item(4,"TEXT","NOTLAR","",Double.NaN,Double.NaN,false,false,"TEXT|0,0|c=0|t=|r=0"),
                new MusaAiDrawingIndex.Item(5,"LWPOLYLINE","PIS_SU","",12,Double.NaN,true,false,"LWPOLYLINE|0,0;1,0;1,1|c=0"),
                new MusaAiDrawingIndex.Item(6,"LINE","0","",3,Double.NaN,false,false,"LINE|0,0;0,300000|c=0"),
                new MusaAiDrawingIndex.Item(7,"TEXT","NOTLAR","TODO: revize et",Double.NaN,Double.NaN,false,false,"TEXT|2,2|c=0|t=todo revize et|r=0")
            ),"mm");
    }
    private static MusaAiProjectControl.Result ask(String q){
        MusaAiProjectControl.Result r=MusaAiProjectControl.analyze(index(),q);
        if(!r.matched)throw new AssertionError("not matched: "+q);
        return r;
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiProjectControl.Result full=ask("projeyi kontrol et");
        has(full.text,"Mükerrer geometri: 1 grup / 2 nesne");
        has(full.text,"Sıfır uzunluk/dejenere geometri: 1");
        has(full.text,"Boş TEXT/MTEXT/ATTRIB: 1");
        has(full.text,"Açık polyline: 1");
        has(full.text,"0 katmanındaki görünür nesne: 1");
        has(full.text,"Revizyon/kontrol notu: 1");
        has(full.text,"Gizli katman: 1");
        if(full.sourceIds.size()!=7)throw new AssertionError("highlight ids "+full.sourceIds.size());

        MusaAiProjectControl.Result dup=ask("mükerrer nesneleri bul");
        has(dup.text,"1 grup / 2 nesne");
        if(dup.sourceIds.size()!=2)throw new AssertionError("duplicate highlights");

        MusaAiProjectControl.Result open=ask("açık polylineleri kontrol et");
        has(open.text,"Açık polyline: 1");
        if(open.sourceIds.size()!=1||open.sourceIds.get(0)!=5)throw new AssertionError("open polyline id");

        if(MusaAiProjectControl.analyze(index(),"kaç daire var").matched)throw new AssertionError("unrelated query matched");
        System.out.println("MusaAiProjectControlTest OK");
    }
}
