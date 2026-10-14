import com.musa.cad.NativePageCatalog;
import com.musa.cad.NativeViewportPageCache;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Synthetic no-DWG regression for incremental viewport lookup during
 * incomplete native page discovery. Run with plain javac in Android CI.
 */
public final class NativePartialViewportIndexTest {
    private static NativeViewportPageCache.Cursor cursor(long n) {
        return new NativeViewportPageCache.Cursor(n,0);
    }
    private static void require(boolean condition,String message) {
        if(!condition)throw new AssertionError(message);
    }
    private static List<NativePageCatalog.Entry> reference(
            List<NativePageCatalog.Entry> all,
            NativeViewportPageCache<?> cache,
            float minX,float minY,float maxX,float maxY,int limit) {
        double cx=((double)minX+maxX)*.5;
        double cy=((double)minY+maxY)*.5;
        Comparator<NativePageCatalog.Entry> nearest=(a,b)->{
            double ax=((double)a.minX+a.maxX)*.5-cx;
            double ay=((double)a.minY+a.maxY)*.5-cy;
            double bx=((double)b.minX+b.maxX)*.5-cx;
            double by=((double)b.minY+b.maxY)*.5-cy;
            int cmp=Double.compare(ax*ax+ay*ay,bx*bx+by*by);
            return cmp==0?a.start.compareTo(b.start):cmp;
        };
        ArrayList<NativePageCatalog.Entry> visible=new ArrayList<>();
        for(NativePageCatalog.Entry page:all) {
            if(page.intersects(minX,minY,maxX,maxY)&&!cache.contains(page.start))
                visible.add(page);
        }
        visible.sort(nearest);
        return visible.subList(0,Math.min(visible.size(),limit));
    }
    private static void check(NativePageCatalog catalog,
            List<NativePageCatalog.Entry> original,
            NativeViewportPageCache<?> cache,
            float minX,float minY,float maxX,float maxY,int limit) {
        List<NativePageCatalog.Entry> got=catalog.missingVisible(
                cache,minX,minY,maxX,maxY,limit);
        List<NativePageCatalog.Entry> expected=reference(
                original,cache,minX,minY,maxX,maxY,limit);
        require(got.size()==expected.size(),"missing visible candidate count");
        for(int i=0;i<got.size();i++)
            require(got.get(i).start.equals(expected.get(i).start),
                    "partial index must preserve nearest cursor at "+i);
    }
    public static void main(String[] unused) {
        final int count=32767; // deliberately not a multiple of group size
        NativePageCatalog catalog=new NativePageCatalog(100000);
        NativeViewportPageCache<String> cache=
                new NativeViewportPageCache<>(65536,256);
        List<NativePageCatalog.Entry> all=new ArrayList<>(count);
        for(int i=0;i<count;i++){
            // Clusters group by X; negative world positions and thin pages
            // exercise the conservative envelopes and page-level filtering.
            float x=(i/128)*10f-1250f;
            float y=(i%128)*3f-180f;
            catalog.append(cursor(i),cursor(i+1),true,
                           x,y,x+2f,y+1f,1);
            all.add(catalog.get(cursor(i)));
        }
        require(!catalog.isDiscovered(),"still discovering after 32767 pages");
        for(int i=0;i<90;i++){
            float x=((i*37)%2700)-1300f;
            float y=((i*41)%420)-210f;
            check(catalog,all,cache,x,y,x+30f,y+30f,8);
        }
        check(catalog,all,cache,-2000f,-500f,2000f,500f,16);
        check(catalog,all,cache,3000f,3000f,3200f,3200f,8);
        check(catalog,all,cache,-1250f,-180f,-1250f,-180f,8);
        cache.put(cursor(64),cursor(65),true,-1250f,12f,
                  -1248f,13f,32L,"known");
        check(catalog,all,cache,-1260f,-200f,-1230f,230f,16);
        require(cache.contains(cursor(64)),"query must not change cache LRU");

        // A rejected append must not mutate the conservative group envelope.
        try {
            catalog.append(cursor(count+5),cursor(count+6),true,
                           -999999f,-999999f,999999f,999999f,1);
            throw new AssertionError("cursor gap accepted");
        }catch(IllegalArgumentException expected){}
        check(catalog,all,cache,-1250f,-180f,-1248f,-179f,8);

        catalog.clear();
        require(catalog.size()==0&&!catalog.isDiscovered(),
                "catalog and partial groups reset together");
        require(catalog.missingVisible(cache,-1f,-1f,1f,1f,8).isEmpty(),
                "cleared catalog contains no old page");
        catalog.append(cursor(0),cursor(1),false,
                       -0.0f,-0.0f,0.0f,0.0f,1);
        ArrayList<NativePageCatalog.Entry> single=new ArrayList<>();
        single.add(catalog.get(cursor(0)));
        check(catalog,single,cache,0f,0f,0f,0f,8);

        System.out.println("Partial 32k-page DWG grouped viewport index: exhaustive nearest parity PASS");
    }
}
