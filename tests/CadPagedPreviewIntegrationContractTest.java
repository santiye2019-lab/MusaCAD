import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class CadPagedPreviewIntegrationContractTest {
    private static String read(String path) throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void need(String source,String fragment,String label){
        if(!source.contains(fragment))
            throw new AssertionError(label+" is missing "+fragment);
    }
    private static void absent(String source,String fragment,String label){
        if(source.contains(fragment))
            throw new AssertionError(label+" accidentally activated "+fragment);
    }
    public static void main(String[] args) throws Exception{
        String view=read("app/src/main/java/com/musa/cad/CadView.java");
        String activity=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String loader=read("app/src/main/java/com/musa/cad/NativePagedPreviewLoader.java");
        String compositor=read("app/src/main/java/com/musa/cad/NativePagedScenePreview.java");
        String cache=read("app/src/main/java/com/musa/cad/NativeViewportPageCache.java");
        String engine=read("app/src/main/java/com/musa/cad/NativeCadEngine.java");

        need(view,"attachExperimentalPagedPreview(", "explicit opt-in");
        need(view,"!preview.acceptsFirstScene(nativeDrawing)","cross-project attachment guard");
        need(compositor,"WeakReference<NativeScene>","evicted page must not be pinned");
        need(view,"experimentalPagedPreview.draw(c,imageMatrix,fastNavigation)","render path");
        need(view,"experimentalPagedPreview.isPartial()","missing-page marker");
        need(view,"DWG önizleme kısmi", "visible incomplete-state text");
        need(view,"if(experimentalPagedPreview!=null)","fallback path");
        need(view,"else nativeDrawing.draw(c,imageMatrix,fastNavigation)","unchanged legacy fallback");
        // Critical: switching tabs/drawings and vector handoff cannot reuse old DWG pages.
        need(view,"clearExperimentalPagedPreview();\n        stopFastNavigation();\n        vectorDrawing=vector;", "project switch reset");
        need(view,"clearExperimentalPagedPreview();\n        drawing=null;vectorDrawing=result;", "vector upgrade reset");
        need(view,"clearExperimentalPagedPreview();\n        drawing=null;nativeDrawing=null;vectorDrawing=result;", "vector reset");
        need(view,"clearExperimentalPagedPreview();\n        drawing=null;vectorDrawing=null;nativeDrawing=result;", "native drawing reset");
        absent(activity,"attachExperimentalPagedPreview(", "normal APK DWG open path");
        absent(activity,"new NativePagedPreviewLoader(", "normal APK DWG open path");

        need(loader,"Executors.newSingleThreadExecutor()","off-UI decoder");
        need(loader,"engine.previewNestedPage(top,leaf,maxFloatWords)","two-part nested traversal");
        need(loader,"AtomicBoolean cancelled","cancel-safe worker");
        need(loader,"main.post(","main thread callbacks");
        need(loader,"preview.retain(top,leaf,page)","bounded storage");
        need(loader,"listener.onEnd(preview.isComplete(),doneTotal)","honest completion");
        need(loader,"listener.onError(","explicit decoding errors");
        need(loader,"worker.shutdownNow()","worker cancellation");
        need(view,"experimentalPagedLoader.close()","project/tab cancellation");
        need(view,"experimentalPagedLoader.requestVisibleWorld(","visible reload trigger");
        need(compositor,"visibleWorldBounds(","shared viewport mapping");
        need(compositor,"retainReload(","strict source page reload");
        need(compositor,"catalog.samePage(","reject changed source");
        need(compositor,"catalog.clear()","clear catalog on source switch");
        need(loader,"latestViewport.getAndSet(null)","latest viewport wins");
        need(loader,"preview.retainReload(entry,refreshed)","reload validates records");
        need(loader,"MAX_RELOADS_PER_REQUEST","bounded reload work");

        need(loader,"pageNoticePending.compareAndSet(false,true)","coalesced UI updates");
        need(loader,"currentPrimitives,preview.isComplete()","fresh callback state");


        need(compositor,"cache.visible(","viewport page pruning");
        need(compositor,"cache.isComplete()","partial flag");
        need(cache,"index.get(cursor)","non-mutating completeness read");
        need(cache,"result.sort((a,b)->a.start.compareTo(b.start))","source draw order");
        need(engine,"previewNestedPage(","native paged cursor interface");

        System.out.println("Opt-in CadView paging, async loader, legacy fallback and lifecycle gates PASS");
    }
}
