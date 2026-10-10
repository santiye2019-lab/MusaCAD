import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Contract: a valid license session is not evidence of Qwen Worker connectivity. */
public final class MusaAiPilotConnectivityContractTest {
    static String read(String path)throws Exception {
        return Files.readString(Path.of(path), StandardCharsets.UTF_8);
    }
    static void has(String body,String value) {
        if(!body.contains(value))throw new AssertionError("Missing "+value);
    }
    static void check(boolean result,String message) {
        if(!result)throw new AssertionError(message);
    }
    public static void main(String[]args)throws Exception {
        String health=read("app/src/main/java/com/musa/cad/MusaAiCloudHealth.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String cloud=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String pilot=read(".github/workflows/qwen-isolated-pilot-24h.yml");
        has(health,"public static Diagnostic diagnoseNow()");
        has(health,"if(!configured())");
        has(health,"new URL(endpoint.getProtocol(),endpoint.getHost(),endpoint.getPort(),\"/health\")");
        has(health,"connection=(HttpsURLConnection)health.openConnection()");
        has(health,"connection.setInstanceFollowRedirects(false)");
        has(health,"connection.setRequestMethod(\"GET\")");
        has(health,"connection.setConnectTimeout(5000)");
        has(health,"connection.setReadTimeout(5000)");
        has(health,"catch(UnknownHostException e)");
        has(health,"catch(SSLException e)");
        has(health,"catch(SocketTimeoutException e)");
        has(health,"\"DNS\"");
        has(health,"\"TLS\"");
        has(health,"\"HTTP\"");
        has(health,"\"ok\".equals(data.optString(\"status\"))");
        has(health,"\"musacad-ai-worker\".equals(data.optString(\"service\"))");
        check(!health.contains("Bearer ")&&!health.contains("session.token"),
            "No authentication token in diagnostic request");
        check(!health.contains("MusaAiCadJson")&&!health.contains("rawDrawing"),
            "Never upload drawing evidence in diagnostic request");

        int session=main.indexOf("MusaAiSessionService.Result preflight=");
        int probe=main.indexOf("MusaAiCloudHealth.Diagnostic network=MusaAiCloudHealth.diagnoseNow();");
        int index=main.indexOf("MusaAiDrawingIndex snapshot=activeSnapshot.aiDrawingIndex(CLOUD_AI_INDEX_MAX_ITEMS);");
        check(session>=0&&probe>session&&index>probe,
            "Probe must follow authorization, precede costly indexing");
        has(main,"AI görsel bölgeleri: 0/9 — hiçbir görüntü AI tarafından incelenmedi.");
        has(main,"MusaAiCloudHealth.Diagnostic afterFailure=");
        has(main,"cloud.status==MusaAiCloudService.Status.NETWORK_ERROR");
        has(main,"(afterFailure==null||afterFailure.reachable)");
        has(main,"if(!firstTransferFailure.isEmpty()");
        has(main,"Worker sağlık kontrolü [");
        has(main,"acceptedTiles==total");
        has(cloud,"catch(UnknownHostException e)");
        has(cloud,"catch(SSLException e)");
        has(cloud,"catch(SocketException e)");
        has(pilot,"MUSACAD_PILOT_DEVELOPER_ONLY = \"true\"");
        has(pilot,"denied\" =");
        System.out.println("MusaAiPilotConnectivityContractTest OK");
    }
}
