import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** A visual Qwen transport failure can recover, but never manufacture inspected tiles. */
public final class MusaAiCompactVisionRetryContractTest {
    private static String read(String path)throws Exception {
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void require(boolean yes,String message) {
        if(!yes)throw new AssertionError(message);
    }
    private static void contains(String source,String text) {
        require(source.contains(text),"Missing evidence: "+text);
    }
    public static void main(String[]args)throws Exception {
        String visual=read("app/src/main/java/com/musa/cad/MusaAiVisualEvidence.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String client=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String worker=read("server/ai-worker/src/index.js");
        String pilot=read(".github/workflows/qwen-isolated-pilot-24h.yml");

        contains(visual,"renderBatchCompact(");
        contains(visual,"renderBatchInternal(drawing,batch,progress,true)");
        contains(visual,"COMPACT_TILE_SIDE=896");
        contains(visual,"COMPACT_OVERVIEW_SIDE=512");
        contains(visual,"COMPACT_BASE64_CHARS=1_000_000");
        contains(visual,"payload.put(\"reducedResolution\",compact)");
        contains(visual,"payload.put(\"complete\",complete)");
        contains(visual,"payload.put(\"rawDrawingIncluded\",false)");
        contains(visual,"drawingBounds");

        int policy=main.indexOf("private static boolean canRetryCompactVisual(");
        int visualMethod=main.indexOf("private void runMusaAiWholeSheetVision(");
        require(policy>=0&&visualMethod>policy,"Missing compact retry policy");
        String retryPolicy=main.substring(policy,visualMethod);
        contains(retryPolicy,"case NETWORK_ERROR:");
        contains(retryPolicy,"case MODEL_TIMEOUT:");
        contains(retryPolicy,"case UPSTREAM_UNAVAILABLE:");
        contains(retryPolicy,"case INVALID_RESPONSE:");
        contains(retryPolicy,"HTTP 413");
        contains(retryPolicy,"HTTP 431");
        require(!retryPolicy.contains("case DENIED:") &&
                !retryPolicy.contains("case QUOTA_EXHAUSTED:"),
                "Auth/quota errors must not retry");
        contains(main,"canRetryCompactVisual(cloud)");
        contains(main,"renderBatchCompact(drawing,batch,reply::progress)");
        contains(main,"if(retry.ok())compactAcceptedBatches++");
        contains(main,"acceptedTiles+=rendered.renderedTiles");
        contains(main,"acceptedTiles==total");
        contains(main,"MusaAiVisionTimeBudget.mayStartNextBatch(");
        contains(main,"MusaAiVisionTimeBudget.mayRetryBatch(");
        String synthesis=read("app/src/main/java/com/musa/cad/MusaAiEngineeringSynthesis.java");
        contains(synthesis,"KISMİ; eksik bölgeler hakkında teknik çıkarım yapılmadı.");

        contains(client,"visualEvidence==null?MusaAiCadJson.DEFAULT_MAX_ITEMS:650");
        // Transport resets must be correlated without uploading credentials to logs.
        contains(client,"MAX_REQUEST_BYTES=3*1024*1024");
        contains(client,"X-MusaCAD-Request-ID");
        contains(client,"phase=\"görsel/CAD yüklemesi\"");
        contains(client,"phase=\"sunucu yanıtını bekleme\"");
        contains(client,"İstek: ");
        contains(worker,"MUSACAD_AI_VISUAL_ACCEPTED");
        contains(worker,"MUSACAD_AI_VISUAL_FAILURE");
        contains(worker,"MUSACAD_AI_VISUAL_UPSTREAM");
        contains(worker,"request.headers.get(\"x-musacad-request-id\")");
        contains(worker,"Math.max(1536, Math.min(2048, maxOutputTokens))");
        contains(pilot,"AI_UPSTREAM_TIMEOUT_MS = \"72000\"");
        System.out.println("MusaAiCompactVisionRetryContractTest OK");
    }
}
