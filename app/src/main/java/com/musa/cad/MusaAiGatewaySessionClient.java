package com.musa.cad;

import android.content.Context;
import org.json.JSONObject;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;

/**
 * Development/test pairing flow for Gandalf cloud sessions.
 *
 * Production deployments may replace the gateway's pairing endpoint with a
 * supported account/entitlement provider without changing the analysis client.
 */
public final class MusaAiGatewaySessionClient {
    public interface Callback {
        void onConnected(String source,long expiresAtMs);
        void onError(String message);
    }

    public static void connect(Context context,String pairingCode,Callback callback){
        if(context==null||callback==null)return;
        final String analyze=BuildConfig.AI_GATEWAY_URL==null?"":BuildConfig.AI_GATEWAY_URL.trim();
        final String code=pairingCode==null?"":pairingCode.trim();
        if(analyze.isEmpty()){callback.onError("Gandalf ağ geçidi yapılandırılmamış.");return;}
        if(!analyze.startsWith("https://")){callback.onError("Gandalf ağ geçidi HTTPS olmalı.");return;}
        if(code.isEmpty()){callback.onError("Eşleştirme kodu boş olamaz.");return;}

        final String endpoint=sessionEndpoint(analyze);
        new Thread(()->{
            HttpURLConnection connection=null;
            try{
                JSONObject request=new JSONObject();
                request.put("pairing_code",code);

                connection=(HttpURLConnection)new URL(endpoint).openConnection();
                connection.setRequestMethod("POST");
                connection.setConnectTimeout(15000);
                connection.setReadTimeout(30000);
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
                connection.setRequestProperty("Accept","application/json");

                byte[]payload=request.toString().getBytes(StandardCharsets.UTF_8);
                connection.setFixedLengthStreamingMode(payload.length);
                try(OutputStream out=connection.getOutputStream()){out.write(payload);}

                int status=connection.getResponseCode();
                InputStream stream=status>=200&&status<300?connection.getInputStream():connection.getErrorStream();
                String raw=read(stream);
                if(status<200||status>=300)throw new IOException("HTTP "+status+(raw.isEmpty()?"":" • "+clip(raw,240)));

                JSONObject json=new JSONObject(raw);
                String token=json.optString("token","").trim();
                long expires=json.optLong("expires_at_ms",0L);
                boolean verified=json.optBoolean("entitlement_verified",false);
                String source=json.optString("source","gateway_session");
                if(token.isEmpty()||expires<=System.currentTimeMillis()||!verified){
                    throw new IOException("Geçersiz Gandalf oturumu.");
                }

                MusaAiCloudSession.set(token,expires);
                MusaAiCloudAccess.setAccountLinked(context,true);
                MusaAiCloudAccess.setVerifiedCloudEntitlement(context,true,expires,source);
                callback.onConnected(source,expires);
            }catch(Exception e){
                callback.onError("Gandalf bağlantısı kurulamadı: "+safeMessage(e));
            }finally{
                if(connection!=null)connection.disconnect();
            }
        },"MusaCAD-Gandalf-Session").start();
    }

    static String sessionEndpoint(String analyzeUrl){
        String s=analyzeUrl==null?"":analyzeUrl.trim();
        int query=s.indexOf('?');if(query>=0)s=s.substring(0,query);
        if(s.endsWith("/analyze"))return s.substring(0,s.length()-"/analyze".length())+"/session";
        int slash=s.lastIndexOf('/');
        return slash>="https://".length()?s.substring(0,slash)+"/session":s+"/session";
    }

    private static String read(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream src=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[]buf=new byte[4096];int n;int total=0;
            while((n=src.read(buf))>=0){
                total+=n;if(total>256_000)throw new IOException("Yanıt boyutu sınırı aşıldı.");
                out.write(buf,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }
    private static String safeMessage(Exception e){
        String m=e==null?"":e.getMessage();
        return m==null||m.trim().isEmpty()?"Bilinmeyen bağlantı hatası":clip(m.trim(),260);
    }
    private static String clip(String s,int max){return s.length()<=max?s:s.substring(0,max)+"…";}
    private MusaAiGatewaySessionClient(){}
}
