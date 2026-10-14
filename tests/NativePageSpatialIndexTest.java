import com.musa.cad.NativePageCatalog;
import com.musa.cad.NativePageSpatialIndex;
import com.musa.cad.NativeViewportPageCache;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public final class NativePageSpatialIndexTest {
    private static NativeViewportPageCache.Cursor c(long value){
        return new NativeViewportPageCache.Cursor(value,0);
    }
    private static void need(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }

    private static void check(NativePageSpatialIndex index,List<NativePageCatalog.Entry> originals,
            float left,float top,float right,float bottom){
        NativePageSpatialIndex.Span chosen=index.candidates(left,top,right,bottom);
        HashSet<NativePageCatalog.Entry> actual=new HashSet<>();
        for(int k=chosen.from;k<chosen.to;k++){
            NativePageCatalog.Entry e=chosen.entries[k];
            if(e.intersects(left,top,right,bottom))actual.add(e);
        }
        HashSet<NativePageCatalog.Entry> expected=new HashSet<>();
        for(NativePageCatalog.Entry e:originals)
            if(e.intersects(left,top,right,bottom))expected.add(e);
        need(actual.equals(expected),"indexed viewport must match exhaustive source");
    }

    public static void main(String[] args){
        final int count=10000;
        NativePageCatalog catalog=new NativePageCatalog(count);
        List<NativePageCatalog.Entry> entries=new ArrayList<>();
        // 100 x 100 grid, no real engineering DWG is stored in CI.
        for(int i=0;i<count;i++){
            int x=i%100,y=i/100;
            float x0=x*10f,y0=y*10f;
            catalog.append(c(i),c(i+1L),i+1<count,x0,y0,x0+3,y0+3,1);
            entries.add(catalog.get(c(i)));
        }
        NativePageSpatialIndex index=new NativePageSpatialIndex(entries);
        need(index.size()==count,"all page metadata indexed");
        for(int x=0;x<100;x+=13)
            for(int y=0;y<100;y+=17)
                check(index,entries,x*10-1,y*10-1,x*10+14,y*10+14);
        check(index,entries,-100,-100,-1,-1);
        check(index,entries,1001,1001,2000,2000);
        check(index,entries,0,0,1000,1000);

        NativePageSpatialIndex.Span narrow=index.candidates(
                198f,28f,215f,35f);
        need(narrow.size()<3000,
                "narrow viewport should examine a small fraction of 10000 pages");

        NativeViewportPageCache<String> empty=new NativeViewportPageCache<>(1000,8);
        List<NativePageCatalog.Entry> nearest=catalog.missingVisible(
                empty,198,28,215,35,8);
        need(nearest.size()>0&&nearest.size()<=8,"bounded nearest viewport pages");
        for(NativePageCatalog.Entry e:nearest)
            need(e.intersects(198,28,215,35),"catalog never returns a wrong page");

        // A partially discovered index must still answer correctly.
        NativePageCatalog partial=new NativePageCatalog(250);
        for(int i=0;i<249;i++)
            partial.append(c(i),c(i+1),true,i,0,i+1,1,1);
        need(!partial.isDiscovered(),"partial source still discovering");
        need(partial.missingVisible(empty,2,-1,9,2,16).size()==9,
                "incremental catalog remains queryable");

        catalog.clear();
        need(catalog.size()==0&&
             catalog.missingVisible(empty,0,0,100,100,8).isEmpty(),
             "clearing drawing invalidates spatial index and cached metadata");
        System.out.println("10k DWG-page spatial index query and full-scan parity PASS");
    }
}
