package com.musa.cad;

import android.content.Context;
import org.json.JSONObject;
import javax.net.ssl.HttpsURLConnection;
import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/** Exchanges a MusaCAD license proof for a short-lived cloud AI session token. */
public final class MusaAiSessionService {
    private static final int TIMEOUT_MS=12000,MAX_RESPONSE_BYTES=32768;
    private static volatile String cachedToken="";
    private static volatile long cachedExpiresAtMs=0L;
    private static volatile String cachedAccessMode="licensed";

    public enum Status { ACTIVE, NOT_CONFIGURED, ACCESS_REQUIRED, NETWORK_ERROR, DENIED, INVALID_RESPONSE }
    public static final class Result {
        public final Status status;
        public final String token,message,accessMode;
        public final long expiresAtMs;
        Result(Status status,String token,long expiresAtMs,String message){
            this(status,token,expiresAtMs,message,"licensed");
        }
        Result(Status status,String token,long expiresAtMs,String message,String accessMode){
            this.status=status;
            this.token=token==null?"":token;
            this.expiresAtMs=expiresAtMs;
            this.message=message==null?"":message;
            this.accessMode="developer".equalsIgnoreCase(accessMode)?"developer":"licensed";
        }
        public boolean active(){return status==Status.ACTIVE&&!token.isEmpty()&&expiresAtMs>System.currentTimeMillis();}
        public boolean developer(){return active()&&"developer".equals(accessMode);}
    }

    public static Result get(Context context){
        if(context==null)return new Result(Status.INVALID_RESPONSE,"",0,"context");
        long now=System.currentTimeMillis();
        String token=cachedToken;long exp=cachedExpiresAtMs;
        if(!token.isEmpty()&&exp-now>60_000L)
            return new Result(Status.ACTIVE,token,exp,"",cachedAccessMode);

        // Do not short-circuit only on local license state: the server can grant a
        // trusted administrator device a signed Gandalf Developer session. Normal
        // unlicensed devices are still rejected by the server.

        String endpoint=BuildConfig.AI_SESSION_URL==null?"":BuildConfig.AI_SESSION_URL.trim();
        if(endpoint.isEmpty())return new Result(Status.NOT_CONFIGURED,"",0,"Bulut AI oturum sunucusu yapılandırılmadı");
        if(!endpoint.startsWith("https://"))return new Result(Status.NOT_CONFIGURED,"",0,"Bulut AI oturum adresi HTTPS olmalıdır");

        HttpsURLConnection connection=null;
        try{
            connection=(HttpsURLConnection)new URL(endpoint).openConnection();
            connection.setConnectTimeout(TIMEOUT_MS);connection.setReadTimeout(TIMEOUT_MS);
            connection.setInstanceFollowRedirects(false);connection.setUseCaches(false);
            connection.setRequestMethod("POST");connection.setDoOutput(true);
            connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
            connection.setRequestProperty("Accept","application/json");

            JSONObject body=new JSONObject();
            body.put("deviceId",LicenseManager.installationId(context));
            body.put("packageName",context.getPackageName());
            body.put("entitlementProof",LicenseManager.cloudEntitlementProof(context));
            body.put("versionName",BuildConfig.VERSION_NAME);
            body.put("versionCode",BuildConfig.VERSION_CODE);
            byte[] bytes=body.toString().getBytes(StandardCharsets.UTF_8);
            connection.setFixedLengthStreamingMode(bytes.length);
            try(OutputStream out=connection.getOutputStream()){out.write(bytes);}

            int code=connection.getResponseCode();
            if(code>=300&&code<400)return new Result(Status.DENIED,"",0,"Oturum sunucusu yönlendirme döndürdü");
            String response=readLimited(code>=200&&code<300?connection.getInputStream():connection.getErrorStream());
            if(response.isEmpty())return new Result(Status.INVALID_RESPONSE,"",0,"Boş oturum yanıtı");
            JSONObject json=new JSONObject(response);
            if(code==401||code==403)return new Result(Status.DENIED,"",0,json.optString("message","Bulut AI oturumu reddedildi"));
            String status=json.optString("status","");
            if(!"active".equalsIgnoreCase(status))return new Result(Status.INVALID_RESPONSE,"",0,json.optString("message","Bulut AI oturumu açılamadı"));
            token=json.optString("token","").trim();exp=json.optLong("expiresAtMs",0L);
            if(token.isEmpty()||exp<=now)return new Result(Status.INVALID_RESPONSE,"",0,"Geçersiz bulut AI oturum yanıtı");
            String accessMode=json.optString("accessMode","licensed").trim();
            if(!"developer".equalsIgnoreCase(accessMode))accessMode="licensed";
            cachedToken=token;cachedExpiresAtMs=exp;cachedAccessMode=accessMode;
            return new Result(Status.ACTIVE,token,exp,"",accessMode);
        }catch(IOException e){
            return new Result(Status.NETWORK_ERROR,"",0,"Bulut AI oturumu için internet bağlantısını kontrol edin");
        }catch(Exception e){
            return new Result(Status.INVALID_RESPONSE,"",0,"Bulut AI oturum yanıtı doğrulanamadı");
        }finally{if(connection!=null)connection.disconnect();}
    }

    public static boolean developerCached(){
        return cachedExpiresAtMs>System.currentTimeMillis()&&"developer".equals(cachedAccessMode);
    }

    public static String cachedAccessMode(){
        return developerCached()?"developer":"licensed";
    }

    static void clearCache(){cachedToken="";cachedExpiresAtMs=0L;cachedAccessMode="licensed";}

    private static String readLimited(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream input=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n,total=0;
            while((n=input.read(b))!=-1){
                total+=n;if(total>MAX_RESPONSE_BYTES)throw new IOException("response too large");
                out.write(b,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private MusaAiSessionService(){}
}
