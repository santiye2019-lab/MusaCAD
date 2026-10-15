import com.musa.cad.NativeViewportPageCache;
import java.util.ArrayList;

public class NativeViewportPageCacheTest {
    static NativeViewportPageCache.Cursor c(long top,long leaf){
        return new NativeViewportPageCache.Cursor(top,leaf);
    }
    static void test(boolean expected,String detail) {
        if(!expected)throw new AssertionError(detail);
    }
    static void expectFailure(Runnable action,String detail) {
        try{action.run();throw new AssertionError("expected failure: "+detail);}
        catch(IllegalArgumentException good){}
    }
    static void testViewportAndOrder(){
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(500,4);
        test(!cache.isComplete(),"empty cache cannot claim complete");
        test(cache.put(c(5,0),c(8,0),true,20,0,30,5,100,"B"),"page B");
        test(cache.put(c(0,0),c(5,0),true,0,0,10,5,100,"A"),"page A");
        test(!cache.isComplete(),"terminal missing");
        test(cache.put(c(8,0),c(12,0),false,40,0,50,5,100,"C"),"terminal C");
        test(cache.isComplete(),"complete chain");
        ArrayList<NativeViewportPageCache.Page<String>> all=
                cache.visible(-5,-5,100,100);
        test(all.size()==3,"three visible");
        test("A".equals(all.get(0).payload),"source order first");
        test("B".equals(all.get(1).payload),"source order second");
        test("C".equals(all.get(2).payload),"source order third");
        test(cache.visible(21,-1,22,4).size()==1,"visible pruning");
        test("B".equals(cache.visible(21,-1,22,4).get(0).payload),
             "near page should remain");
        test(cache.visible(31,-1,39,4).isEmpty(),"offscreen skipping");
        test(cache.isComplete(),"visible operations must preserve chain");
    }
    static void testEviction(){
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(200,2);
        cache.put(c(0,0),c(1,0),true,0,0,1,1,100,"A");
        cache.put(c(1,0),c(2,0),true,2,0,3,1,100,"B");
        cache.visible(0,0,1,1); // A most recently used
        cache.put(c(2,0),c(3,0),false,4,0,5,1,100,"C");
        test(cache.pageCount()==2,"LRU budget");
        test(cache.retainedBytes()==200,"exact retained bytes");
        test(cache.hasEvictedPages(),"eviction signal");
        test(!cache.isComplete(),"evicted middle page breaks coverage");
        test(cache.visible(2,0,3,1).isEmpty(),"B was actually evicted");
        test(!cache.put(c(7,0),c(8,0),false,10,0,11,1,300,"tooBig"),"oversize rejected");
        test(cache.pageCount()==2,"reject must preserve existing pages");
        cache.clear();
        test(cache.retainedBytes()==0&&!cache.isComplete()&&!cache.hasEvictedPages(),
             "clear resets cache and coverage");
    }
    static void testNestedCursorAndInputs(){
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(1000,4);
        cache.put(c(0,0),c(10,2),true,0,0,4,4,80,"p0");
        cache.put(c(10,2),c(10,5),true,0,0,5,5,80,"p1");
        cache.put(c(10,5),c(12,0),false,0,0,6,6,80,"p2");
        test(cache.isComplete(),"nested leaf cursor chain");
        expectFailure(()->cache.put(c(10,5),c(10,5),false,0,0,1,1,20,"same"),
            "no-progress cursor");
        expectFailure(()->cache.put(c(12,0),c(10,2),false,0,0,1,1,20,"backwards"),
            "cursor regression");
        expectFailure(()->cache.put(c(13,0),c(14,0),true,Float.NaN,0,1,1,20,"nan"),
            "reject invalid bbox");
        expectFailure(()->cache.visible(2,2,1,1),"reject inverted viewport");
        test(cache.isComplete(),"invalid insert cannot corrupt cache");
    }
    public static void main(String[] args){
        testViewportAndOrder();
        testEviction();
        testNestedCursorAndInputs();
        System.out.println("Native viewport page cache LRU, culling, ordering, nested coverage PASS");
    }
}
