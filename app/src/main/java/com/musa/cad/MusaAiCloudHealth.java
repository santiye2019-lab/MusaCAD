package com.musa.cad;

import android.os.Handler;
import android.os.Looper;
import org.json.JSONObject;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Background, bounded health probe for the configured MusaCAD AI gateway.
 * A successful /health request proves gateway reachability only. It does not
 * prove that a licensed AI session or an upstream model is available.
 * Never transmits CAD data, login tokens, book contents or chat history.
 */
public final class MusaAiCloudHealth {
    public interface Callback { void onResult(boolean gatewayReachable); }
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

    public static void check(Callback callback){
        if(callback==null)return;
        if(!configured()){MAIN.post(()->callback.onResult(false));return;}
        WORK.execute(()->{
            boolean ok=false;
            HttpURLConnection connection=null;
            try{
                URL endpoint=new URL(BuildConfig.AI_API_URL);
                URL health=new URL(endpoint.getProtocol(),endpoint.getHost(),endpoint.getPort(),"/health");
                connection=(HttpURLConnection)health.openConnection();
                connection.setInstanceFollowRedirects(false);
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(5000);
                connection.setReadTimeout(5000);
                connection.setRequestProperty("Accept","application/json");
                if(connection.getResponseCode()==200){
                    try(InputStream input=connection.getInputStream()){
                        byte[] bytes=new byte[1024];
                        int n=input.read(bytes);
                        if(n>0){
                            JSONObject json=new JSONObject(new String(bytes,0,n,StandardCharsets.UTF_8));
                            ok="ok".equals(json.optString("status")) &&
                                "musacad-ai-worker".equals(json.optString("service"));
                        }
                    }
                }
            }catch(Exception ignored){
                // The LED reports unreachable, never pretends a request worked.
            }finally{if(connection!=null)connection.disconnect();}
            final boolean reachable=ok;
            MAIN.post(()->callback.onResult(reachable));
        });
    }
    private MusaAiCloudHealth(){}
}
