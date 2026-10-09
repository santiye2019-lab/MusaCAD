import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiConnectionFailureContractTest {
    private static String file(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void has(String source,String needle){
        if(!source.contains(needle))throw new AssertionError("Missing "+needle);
    }
    public static void main(String[] args)throws Exception{
        String service=file("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String activity=file("app/src/main/java/com/musa/cad/MainActivity.java");
        String worker=file("server/ai-worker/src/index.js");
        has(service,"TIMEOUT_MS=90000");
        has(service,"QUOTA_EXHAUSTED");
        has(service,"MODEL_TIMEOUT");
        has(service,"UPSTREAM_UNAVAILABLE");
        has(service,"code==429");
        has(service,"code==504||code==408");
        has(service,"code==401||code==403");
        has(service,"reason(json");
        // Real outage category must survive the exported PDF instead of being
        // flattened to one ambiguous "network failed" string.
        has(service,"catch(UnknownHostException e)");
        has(service,"catch(SSLException e)");
        has(service,"catch(ConnectException e)");
        has(service,"catch(SocketException e)");
        has(service,"catch(OversizedResponseException e)");
        has(service,"code==413||code==431");
        has(service,"JSON olmayan yanıt");
        has(service,"MAX_RESPONSE_BYTES");
        has(activity,"MusaAiSessionService.get(getApplicationContext())");
        has(activity,"ÇEVRİM İÇİ GÖRSEL ANALİZ BAŞLATILAMADI");
        has(activity,"AI sonucu alınamadı:");
        has(activity,"Çevrim içi görsel analiz durdu");
        has(activity,"Bulut AI tamamlanamadı:");
        has(worker,"AI_UPSTREAM_TIMEOUT_MS, 65000");
        has(worker,"Gandalf görsel AI modeli bekleme süresini aştı");
        has(worker,"Kota yenilendiğinde yeniden deneyin");
        System.out.println("MusaAiConnectionFailureContractTest OK");
    }
}
