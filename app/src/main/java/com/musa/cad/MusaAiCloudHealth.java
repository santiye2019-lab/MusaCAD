package com.musa.cad;

import android.os.Handler;
import android.os.Looper;
import org.json.JSONObject;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLException;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.ConnectException;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * No-auth, no-CAD probe of the exact HTTPS host in the installed APK.
 * Health success proves only the Worker is reachable, not that Qwen or the
 * licensed AI session works. Never bypass Android DNS or TLS verification.
 */
public final class MusaAiCloudHealth {
    public interface Callback { void onResult(boolean gatewayReachable); }

    public static final class Diagnostic {
        public final boolean reachable;
        public final String category,message;
        private Diagnostic(boolean reachable,String category,String message){
            this.reachable=reachable;
            this.category=category;
            this.message=message;
        }
    }

    private static final ExecutorService WORK=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-Gandalf-health");
        t.setDaemon(true);
        return t;
    });
    private static final Handler MAIN=new Handler(Looper.getMainLooper());

    public static boolean configured(){
        return validBase(BuildConfig.AI_API_URL) &&
            validBase(BuildConfig.AI_SESSION_URL);
    }

    private static boolean validBase(String url){
        if(url==null||url.trim().isEmpty())return false;
        try{
            URL parsed=new URL(url);
            return "https".equalsIgnoreCase(parsed.getProtocol()) &&
                parsed.getHost()!=null&&!parsed.getHost().trim().isEmpty()&&
                new java.net.URI(url).getUserInfo()==null;
        }catch(Exception ignored){return false;}
    }

    /**
     * Call ONLY on a background worker. Validates /health against the gateway
     * configured in BuildConfig; performs no login, no model inference, no CAD
     * upload and never changes network security settings.
     */
    public static Diagnostic diagnoseNow(){
        if(!configured())return new Diagnostic(false,"CONFIG",
            "Gandalf AI sunucu veya oturum HTTPS adresi yapılandırılmamış.");
        HttpsURLConnection connection=null;
        try{
            URL endpoint=new URL(BuildConfig.AI_API_URL);
            URL health=new URL(endpoint.getProtocol(),endpoint.getHost(),endpoint.getPort(),"/health");
            connection=(HttpsURLConnection)health.openConnection();
            connection.setInstanceFollowRedirects(false);
            connection.setUseCaches(false);
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(5000);
            connection.setReadTimeout(5000);
            connection.setRequestProperty("Accept","application/json");
            int code=connection.getResponseCode();
            if(code!=200)return new Diagnostic(false,"HTTP",
                "Qwen Worker sağlık kontrolü HTTP "+code+" döndürdü. Sunucu yönlendirmesi/erişimi kontrol edilmeli.");
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();
            try(InputStream in=connection.getInputStream()){
                byte[] chunk=new byte[256];int n;
                while((n=in.read(chunk))!=-1){
                    if(bytes.size()+n>1024)
                        return new Diagnostic(false,"BODY","Worker sağlık yanıtı güvenli boyut sınırını aştı.");
                    bytes.write(chunk,0,n);
                }
            }
            JSONObject data=new JSONObject(bytes.toString(StandardCharsets.UTF_8.name()));
            if(!"ok".equals(data.optString("status"))||
               !"musacad-ai-worker".equals(data.optString("service")))
                return new Diagnostic(false,"BODY",
                    "HTTPS yanıtı geldi ama beklenen MusaCAD Worker sağlık imzası bulunamadı.");
            return new Diagnostic(true,"OK",
                "Qwen Worker /health HTTP 200. Bu yalnız sunucu erişimini doğrular, görsel model yanıtını değil.");
        }catch(UnknownHostException e){
            return new Diagnostic(false,"DNS",
                "Telefon Qwen Worker alan adını çözemiyor (DNS). Aynı telefonda /health bağlantısını ve Wi-Fi/mobil veri farkını kontrol edin.");
        }catch(SSLException e){
            return new Diagnostic(false,"TLS",
                "Qwen Worker HTTPS/TLS doğrulaması başarısız. Sertifika veya ağ güvenliği kontrol edilmeli.");
        }catch(SocketTimeoutException e){
            return new Diagnostic(false,"TIMEOUT",
                "Qwen Worker /health kontrolü 5 saniye içinde yanıt vermedi.");
        }catch(ConnectException e){
            return new Diagnostic(false,"CONNECT",
                "Qwen Worker sunucusuna TCP/HTTPS bağlantısı kurulamadı.");
        }catch(SocketException e){
            return new Diagnostic(false,"SOCKET",
                "Qwen Worker sağlık bağlantısı kesildi veya sıfırlandı.");
        }catch(IOException e){
            return new Diagnostic(false,"NETWORK",
                "Qwen Worker sağlık isteği ağ/aktarım hatasıyla tamamlanamadı.");
        }catch(Exception e){
            return new Diagnostic(false,"RESPONSE",
                "Qwen Worker sağlık yanıtı doğrulanamadı.");
        }finally{
            if(connection!=null)connection.disconnect();
        }
    }

    /** LED check reuses the same authenticated-independent endpoint probe. */
    public static void check(Callback callback){
        if(callback==null)return;
        WORK.execute(()->{
            final boolean reachable=diagnoseNow().reachable;
            MAIN.post(()->callback.onResult(reachable));
        });
    }

    private MusaAiCloudHealth(){}
}
