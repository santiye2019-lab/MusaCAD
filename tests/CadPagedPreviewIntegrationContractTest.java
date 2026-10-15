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
        String reloadGate=read("app/src/main/java/com/musa/cad/NativeViewportReloadGate.java");
        String compositor=read("app/src/main/java/com/musa/cad/NativePagedScenePreview.java");
        String cache=read("app/src/main/java/com/musa/cad/NativeViewportPageCache.java");
        String catalog=read("app/src/main/java/com/musa/cad/NativePageCatalog.java");
        String transformGuard=read("app/src/main/java/com/musa/cad/NativePageTransformGuard.java");
        String engine=read("app/src/main/java/com/musa/cad/NativeCadEngine.java");

        need(view,"attachExperimentalPagedPreview(", "explicit opt-in");
        need(view,"!preview.acceptsFirstScene(nativeDrawing)","cross-project attachment guard");
        need(compositor,"WeakReference<NativeScene>","evicted page must not be pinned");
        need(view,"experimentalPagedPreview.draw(c,imageMatrix,fastNavigation)","render path");
        need(view,"experimentalPagedPreview.isPartial()","missing-page marker");
        need(view,"DWG önizleme kısmi", "visible incomplete-state text");
        need(view,"if(experimentalPagedPreview!=null)","fallback path");
        // The ordinary native renderer remains available when opt-in paging is
        // absent. It now also warns when its first-paint scene is incomplete.
        need(view,"}else{\n                nativeDrawing.draw(c,imageMatrix,fastNavigation);",
                "legacy native renderer fallback");
        need(view,"if(nativeDrawing.truncated)", "native incomplete-scene warning");
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
        need(compositor,"catalog.requireAppend(","preflight before cache eviction");
        need(compositor,"cache.put(start,next,page.hasMore","validated cache publication");
        need(compositor,"catalog.samePage(","reject changed source metadata");
        need(compositor,"page.scene.contentSha256()",
                "full DWG page content hash from immutable decoded float stream");
        need(compositor,"pageHashes.put(start,digest)",
                "first decode stores SHA-256 before cache eviction");
        need(compositor,"NativeSceneFingerprint.matches(",
                "reloaded page must match initial SHA-256, not only cursor/bounds/count");
        need(compositor,"NativePageTransformGuard.requireValid(pageWorldTransform)",
                "first page cannot establish a singular or corrupt screen transform");
        need(compositor,"NativePageTransformGuard.requireSame(sharedWorldTransform,pageWorldTransform)",
                "each native DWG page and reload must use exact global coordinates");
        need(compositor,"sharedWorldTransform=null",
                "project/tab reset clears page coordinate identity");
        need(transformGuard,"determinant==0d",
                "zero-area native CAD view must fail closed");
        need(transformGuard,"expected[i]!=candidate[i]",
                "do not silently apply a different DWG coordinate normalization");
        need(compositor,"pageHashes.clear()",
                "source switch must discard original page identities");
        need(compositor,"catalog.clear()","clear catalog on source switch");
        need(loader,"latestViewport.getAndSet(null)","latest viewport wins");
        need(loader,"preview.retainReload(entry,refreshed)","reload validates records");
        need(loader,"MAX_RELOADS_PER_REQUEST","bounded reload work");
        need(loader,"viewportGate.shouldRequest(", "same view retried as discovery progresses");
        need(loader,"currentPages", "monotonic page-discovery generation");
        need(reloadGate,"previousPageCount==decodedPages",
                "unchanged viewport suppresses duplicate drawing requests");
        need(reloadGate,"decodedPages<previousPageCount",
                "cross-source rollback rejected instead of mixed");

        need(loader,"pageNoticePending.compareAndSet(false,true)","coalesced UI updates");
        need(loader,"currentPrimitives,preview.isComplete()","fresh callback state");


        need(compositor,"cache.visible(","viewport page pruning");
        need(compositor,"catalog.hasCompleteCachedCoverage(cache.pageCount())",
                "no O(N) page walk on Canvas draw to show partial flag");
        need(compositor,"public synchronized boolean isPartial(){return !isComplete();}",
                "cached complete-state predicate used by partial indicator");
        need(catalog,"discoveredAll && !ordered.isEmpty()",
                "never claim complete before terminal native page");
        need(catalog,"retainedPages==ordered.size()",
                "no missing or extra source page in complete view");
        need(cache,"index.get(cursor)","non-mutating completeness read");
        need(cache,"result.sort((a,b)->a.start.compareTo(b.start))","source draw order");
        need(engine,"previewNestedPage(","native paged cursor interface");

        System.out.println("Opt-in CadView paging, async loader, legacy fallback and lifecycle gates PASS");
    }
}
