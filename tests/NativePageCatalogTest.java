import com.musa.cad.NativePageCatalog;
import com.musa.cad.NativeViewportPageCache;
import java.util.List;

public final class NativePageCatalogTest {
    static NativeViewportPageCache.Cursor c(long top,long leaf){
        return new NativeViewportPageCache.Cursor(top,leaf);
    }
    static void yes(boolean ok,String name){
        if(!ok)throw new AssertionError(name);
    }
    static void reject(Runnable action,String name){
        try{action.run();throw new AssertionError("expected rejection "+name);}
        catch(IllegalArgumentException expected){}
    }
    public static void main(String[] args){
        NativeViewportPageCache<String> cache=new NativeViewportPageCache<>(200,2);
        NativePageCatalog catalog=new NativePageCatalog(10);
        catalog.append(c(0,0),c(3,0),true,0,0,10,10,11);
        catalog.append(c(3,0),c(3,4),true,20,0,30,10,20);
        catalog.append(c(3,4),c(9,0),false,40,0,50,10,30);
        yes(catalog.isDiscovered()&&catalog.size()==3,"discovery complete");
        yes(catalog.samePage(c(3,0),c(3,4),true,20,0,30,10,20),
            "exact reload metadata");
        yes(!catalog.samePage(c(3,0),c(3,5),true,20,0,30,10,20),
            "no cursor drift");
        yes(!catalog.samePage(c(3,0),c(3,4),true,21,0,30,10,20),
            "no bounds drift");
        yes(!catalog.samePage(c(3,0),c(3,4),true,20,0,30,10,21),
            "no primitive loss");
        reject(()->catalog.append(c(9,0),c(10,0),false,60,0,70,10,1),
            "no append after terminal");
        cache.put(c(0,0),c(3,0),true,0,0,10,10,100,"A");
        cache.put(c(3,0),c(3,4),true,20,0,30,10,100,"B");
        cache.visible(0,0,10,10);
        cache.put(c(3,4),c(9,0),false,40,0,50,10,100,"C");
        yes(!cache.contains(c(3,0)),"middle page evicted");
        yes(catalog.get(c(3,0))!=null,"catalog metadata survives eviction");
        yes(!cache.isComplete(),"cache stays honestly incomplete");
        List<NativePageCatalog.Entry> missing=catalog.missingVisible(
                cache,19,-1,31,11,3);
        yes(missing.size()==1&&missing.get(0).start.equals(c(3,0)),
            "only missing viewport page selected");
        yes(cache.put(c(3,0),c(3,4),true,20,0,30,10,100,"B Reloaded"),
            "restore evicted page");
        yes(catalog.missingVisible(cache,19,-1,31,11,3).isEmpty(),
            "re-fetched page no longer requested");
        yes(!cache.contains(c(0,0)),"reload may evict a different page");
        // Different page bounds: always choose central visible pages first.
        cache.clear();
        missing=catalog.missingVisible(cache,-10,-10,60,20,2);
        yes(missing.size()==2,"bounded reload batch");
        yes(missing.get(0).start.equals(c(3,0)),"nearest page first");
        NativePageCatalog bounded=new NativePageCatalog(1);
        bounded.append(c(0,0),c(1,0),true,0,0,1,1,1);
        reject(()->bounded.append(c(1,0),c(2,0),false,1,0,2,1,1),
            "metadata memory cap");
        reject(()->catalog.missingVisible(cache,2,2,1,1,1),
            "inverted viewport rejected");
        catalog.clear();
        yes(!catalog.isDiscovered()&&catalog.size()==0,"catalog reset");
        reject(()->catalog.append(c(3,0),c(4,0),false,0,0,1,1,1),
            "no cursor gaps after clear");
        System.out.println("Native DWG page catalog and lossless cursor re-fetch scheduling PASS");
    }
}
