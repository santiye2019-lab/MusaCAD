import com.musa.cad.NativePageCatalog;
import com.musa.cad.NativeViewportPageCache;
import com.musa.cad.NativeVisibleReloadBudget;
import java.util.List;

public final class NativeVisibleReloadBudgetTest {
    private static NativeViewportPageCache.Cursor c(long n){
        return new NativeViewportPageCache.Cursor(n,0);
    }
    private static void yes(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static void reject(Runnable action){
        try{action.run();throw new AssertionError("expected invalid budget");}
        catch(IllegalArgumentException ok){}
    }
    private static NativePageCatalog catalog(int count){
        NativePageCatalog result=new NativePageCatalog(count);
        for(int i=0;i<count;i++)
            result.append(c(i),c(i+1),i+1<count,i*10,0,i*10+5,5,1);
        return result;
    }
    private static void testBudget(){
        reject(()->new NativeVisibleReloadBudget(0));
        reject(()->new NativeVisibleReloadBudget(257));
        NativeVisibleReloadBudget budget=new NativeVisibleReloadBudget(32);
        yes(!budget.reserve(null),"null cursor denied");
        for(int i=0;i<32;i++)
            yes(budget.reserve(c(i)),"distinct cursor accepted "+i);
        yes(!budget.reserve(c(0)),"previously attempted cursor never retried");
        yes(!budget.reserve(c(33)),"request hard cap");
        yes(budget.attemptedCount()==32,"bounded page count");
    }
    private static void testCompletesBeyondFirstFour(){
        NativePageCatalog catalog=catalog(10);
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(1000,12);
        NativeVisibleReloadBudget budget=new NativeVisibleReloadBudget(32);
        int batches=0;
        while(true){
            List<NativePageCatalog.Entry> missing=
                    catalog.missingVisible(cache,-1,-1,100,8,4);
            if(missing.isEmpty())break;
            batches++;
            yes(batches<16,"bounded batch progression");
            for(NativePageCatalog.Entry entry:missing){
                yes(budget.reserve(entry.start),"fit viewport must not retry a page");
                yes(cache.put(entry.start,entry.next,entry.hasMore,
                        entry.minX,entry.minY,entry.maxX,entry.maxY,10,"page"),
                        "reload stored");
            }
        }
        yes(batches==3,"ten visible pages require three batches");
        yes(cache.isComplete(),"all visible pages loaded without evictions");
        yes(budget.attemptedCount()==10,"all pages loaded exactly once");
    }
    private static void testStopsEvictionThrash(){
        NativePageCatalog catalog=catalog(6);
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(20,2);
        NativeVisibleReloadBudget budget=new NativeVisibleReloadBudget(32);
        boolean repeated=false;
        int batches=0;
        while(batches++<12&&!repeated){
            List<NativePageCatalog.Entry> missing=
                    catalog.missingVisible(cache,-1,-1,60,8,4);
            if(missing.isEmpty())break;
            for(NativePageCatalog.Entry entry:missing){
                if(!budget.reserve(entry.start)){
                    repeated=true;
                    break;
                }
                yes(cache.put(entry.start,entry.next,entry.hasMore,
                        entry.minX,entry.minY,entry.maxX,entry.maxY,10,"page"),
                        "eviction test store");
            }
        }
        yes(repeated,"revisit detects impossible visible coverage");
        yes(batches<=3,"no unbounded reload thrash");
        yes(!cache.isComplete(),"oversized viewport never claims full DWG");
    }
    public static void main(String[] args){
        testBudget();
        testCompletesBeyondFirstFour();
        testStopsEvictionThrash();
        System.out.println("Native multi-batch visible page reload and bounded eviction handling PASS");
    }
}
