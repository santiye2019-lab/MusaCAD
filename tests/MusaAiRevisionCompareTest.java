import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiRevisionCompare;
import java.util.*;

public final class MusaAiRevisionCompareTest {
    private static MusaAiDrawingIndex base(){
        return new MusaAiDrawingIndex("Model",5,0,
            Arrays.asList("BORU","NOT"),Arrays.asList("BORU","NOT"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LINE","BORU","",10,Double.NaN,false,false,"LINE|0,0;1000000,0|c=0",5,0),
                new MusaAiDrawingIndex.Item(2,"CIRCLE","BORU","",6.283,3.141,false,false,"CIRCLE|2000000,0;3000000,0|c=1",2,0),
                new MusaAiDrawingIndex.Item(3,"TEXT","NOT","VANA",Double.NaN,Double.NaN,false,false,"TEXT|0,2000000|c=0|t=vana|r=0",0,2),
                new MusaAiDrawingIndex.Item(4,"LINE","BORU","",5,Double.NaN,false,false,"LINE|0,3000000;500000,3000000|c=0",.25,3)
            ),"m");
    }
    private static MusaAiDrawingIndex current(){
        return new MusaAiDrawingIndex("Model",6,0,
            Arrays.asList("BORU","NOT","YENI"),Arrays.asList("BORU","NOT","YENI"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(10,"LINE","BORU","",10,Double.NaN,false,false,"LINE|0,0;1000000,0|c=0",5,0),
                new MusaAiDrawingIndex.Item(11,"CIRCLE","YENI","",6.283,3.141,false,false,"CIRCLE|2000000,0;3000000,0|c=1",2,0),
                new MusaAiDrawingIndex.Item(12,"TEXT","NOT","VANA",Double.NaN,Double.NaN,false,false,"TEXT|100000,2100000|c=0|t=vana|r=0",.1,2.1),
                new MusaAiDrawingIndex.Item(13,"LINE","BORU","",8,Double.NaN,false,false,"LINE|0,3000000;800000,3000000|c=0",.4,3),
                new MusaAiDrawingIndex.Item(14,"CIRCLE","BORU","",3.141,0.785,false,false,"CIRCLE|5000000,0;5500000,0|c=1",5,0)
            ),"m");
    }
    private static void eq(int a,int b,String n){if(a!=b)throw new AssertionError(n+" "+a+" != "+b);}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args){
        MusaAiRevisionCompare.Result r=MusaAiRevisionCompare.compare(base(),current(),"revizyonları karşılaştır","R01.dwg","R02.dwg");
        if(!r.matched)throw new AssertionError("not matched");
        eq(r.unchanged,1,"unchanged");
        eq(r.added,1,"added");
        eq(r.removed,0,"removed");
        eq(r.changed,3,"changed");
        if(r.sourceIds.size()!=4)throw new AssertionError("highlight "+r.sourceIds.size());
        has(r.text,"Yeni katman: YENI");
        has(r.text,"Değişen / taşınan: 3");

        MusaAiRevisionCompare.Result added=MusaAiRevisionCompare.compare(base(),current(),"eklenenleri göster","R01","R02");
        eq(added.added,1,"added filter");
        if(added.sourceIds.size()!=1||added.sourceIds.get(0)!=14)throw new AssertionError("added id");

        MusaAiRevisionCompare.Result changed=MusaAiRevisionCompare.compare(base(),current(),"değişenleri göster","R01","R02");
        if(changed.sourceIds.size()!=3)throw new AssertionError("changed ids");

        if(MusaAiRevisionCompare.compare(base(),current(),"kaç daire var","R01","R02").matched)throw new AssertionError("unrelated matched");
        System.out.println("MusaAiRevisionCompareTest OK");
    }
}
