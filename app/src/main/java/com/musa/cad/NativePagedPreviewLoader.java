package com.musa.cad;

import android.os.Handler;
import android.os.Looper;
import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Experimental incremental DWG preview loader (NOT enabled by MainActivity).
 *
 * The DWG decoder and NativeScene.fromRaw run on a dedicated worker. Listener
 * callbacks run on Android's main thread; the Activity must close/cancel this
 * loader during tab switches, user cancellation and Activity teardown.
 * No source DWG or geometry is uploaded to any service.
 *
 * This first loader sequentially discovers pages; it does NOT yet implement
 * viewport-priority fetching or reclaimable native traversal stacks.
 */
public final class NativePagedPreviewLoader implements AutoCloseable {
    public interface Listener {
        /** Called for each retained page. May explicitly opt in on a CadView. */
        void onPage(NativePagedScenePreview preview,NativeScene firstPage,
                    int pagesRead,long primitiveCount,boolean complete);
        /** Called when the producer reached EOF, not when all pages fit cache. */
        void onEnd(boolean complete,long primitiveCount);
        void onError(String safeMessage);
    }

    private final NativeCadEngine engine;
    private final NativePagedScenePreview preview;
    private final int maxFloatWords;
    private final Listener listener;
    private final Handler main=new Handler(Looper.getMainLooper());
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private final AtomicBoolean cancelled=new AtomicBoolean();
    private final AtomicBoolean started=new AtomicBoolean();
    /* At most one queued page-update callback during a fast host decode. */
    private final AtomicBoolean pageNoticePending=new AtomicBoolean();
    private volatile NativeScene firstScene;
    private volatile int currentPages;
    private volatile long currentPrimitives;


    public NativePagedPreviewLoader(NativeCadEngine engine,NativePagedScenePreview preview,
                                    int maxFloatWords,Listener listener) {
        if(engine==null||preview==null||listener==null||
           maxFloatWords<256||maxFloatWords>9*1024*1024)
            throw new IllegalArgumentException("invalid streaming preview parameters");
        this.engine=engine;this.preview=preview;
        this.maxFloatWords=maxFloatWords;this.listener=listener;
    }

    public void start() {
        if(cancelled.get()||!started.compareAndSet(false,true))
            throw new IllegalStateException("preview loader already started or cancelled");
        worker.execute(this::load);
    }

    private void load() {
        long top=0,leaf=0,total=0;
        int count=0;
        NativeScene first=null;
        try{
            while(!cancelled.get()) {
                if(count>=100000)throw new IOException("DWG sayfa sayısı güvenlik sınırını aşıyor");
                NativeCadEngine.NestedScenePage page=
                        engine.previewNestedPage(top,leaf,maxFloatWords);
                if(cancelled.get())break;
                if(page.scene.truncated)
                    throw new IOException("Eksik DWG sayfası reddedildi");
                if(!preview.retain(top,leaf,page))
                    throw new IOException("DWG sayfası ayrılan önbellekten büyük");
                if(first==null)first=page.scene;
                total+=page.scene.primitiveCount;
                count++;
                firstScene=first;
                currentPrimitives=total;
                currentPages=count;
                // Coalesce page notifications, never queue thousands of draw
                // callbacks when the worker decodes pages faster than the UI.
                if(pageNoticePending.compareAndSet(false,true))
                    main.post(()->{
                        pageNoticePending.set(false);
                        if(!cancelled.get())
                            listener.onPage(preview,firstScene,currentPages,
                                            currentPrimitives,preview.isComplete());
                    });
                top=page.nextTopLevelIndex;
                leaf=page.nextNestedLeafIndex;
                if(!page.hasMore) {
                    final long doneTotal=total;
                    main.post(()->{
                        if(!cancelled.get())
                            listener.onEnd(preview.isComplete(),doneTotal);
                    });
                    return;
                }
            }
        }catch(Exception error){
            // No drawing path or layer names in UI error messages.
            main.post(()->{
                if(!cancelled.get())
                    listener.onError("DWG sayfalı önizleme tamamlanamadı: "+
                            error.getClass().getSimpleName());
            });
        }
    }

    public boolean isCancelled(){return cancelled.get();}
    public void cancel(){cancelled.set(true);worker.shutdownNow();}
    @Override public void close(){cancel();}
}
