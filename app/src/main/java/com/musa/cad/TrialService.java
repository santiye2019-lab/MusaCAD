package com.musa.cad;

import android.content.Context;
import org.json.JSONObject;
import javax.net.ssl.HttpsURLConnection;
import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/** One-time online activation; the returned signed token is then verified/used offline. */
public final class TrialService {
    private static final int TIMEOUT_MS=10000,MAX_RESPONSE_BYTES=32768;

    public enum Status { ACTIVATED, ALREADY_USED, NOT_CONFIGURED, NETWORK_ERROR, INVALID_RESPONSE, DENIED }

    public static boolean isConfigured(){
        String endpoint=BuildConfig.TRIAL_API_URL==null?"":BuildConfig.TRIAL_API_URL.trim();
        String trialKey=BuildConfig.TRIAL_PUBLIC_KEY_PEM==null?"":BuildConfig.TRIAL_PUBLIC_KEY_PEM.trim();
        return !endpoint.isEmpty()&&!trialKey.isEmpty()&&endpoint.startsWith("https://");
    }
    public static final class Result {public final Status status;public final String message;Result(Status status,String message){this.status=status;this.message=message;}}

    public static Result start(Context context){
        if(context==null)return new Result(Status.INVALID_RESPONSE,"context");
        String endpoint=BuildConfig.TRIAL_API_URL==null?"":BuildConfig.TRIAL_API_URL.trim();
        String trialKey=BuildConfig.TRIAL_PUBLIC_KEY_PEM==null?"":BuildConfig.TRIAL_PUBLIC_KEY_PEM.trim();
        if(!isConfigured())return new Result(Status.NOT_CONFIGURED,"Trial sunucusu veya doğrulama anahtarı yapılandırılmadı");
        HttpsURLConnection connection=null;
        try{
            URL url=new URL(endpoint);connection=(HttpsURLConnection)url.openConnection();
            connection.setConnectTimeout(TIMEOUT_MS);connection.setReadTimeout(TIMEOUT_MS);connection.setInstanceFollowRedirects(false);
            connection.setRequestMethod("POST");connection.setDoOutput(true);connection.setUseCaches(false);
            connection.setRequestProperty("Content-Type","application/json; charset=utf-8");connection.setRequestProperty("Accept","application/json");
            JSONObject request=new JSONObject();request.put("deviceId",LicenseManager.installationId(context));request.put("packageName",context.getPackageName());request.put("versionName",BuildConfig.VERSION_NAME);request.put("versionCode",BuildConfig.VERSION_CODE);
            byte[] body=request.toString().getBytes(StandardCharsets.UTF_8);connection.setFixedLengthStreamingMode(body.length);
            try(OutputStream out=connection.getOutputStream()){out.write(body);}
            int code=connection.getResponseCode();
            if(code>=300&&code<400)return new Result(Status.DENIED,"Trial sunucusu yönlendirme döndürdü");
            InputStream stream=code>=200&&code<300?connection.getInputStream():connection.getErrorStream();String response=readLimited(stream);
            if(response.isEmpty())return new Result(Status.INVALID_RESPONSE,"Boş trial yanıtı");
            JSONObject json=new JSONObject(response);String status=json.optString("status","").trim().toLowerCase(java.util.Locale.ROOT);
            if("active".equals(status)){
                String token=json.optString("token","");
                if(LicenseManager.activateTrialToken(context,token))return new Result(Status.ACTIVATED,"Trial etkinleştirildi");
                return new Result(Status.INVALID_RESPONSE,"Trial imzası veya süresi geçersiz");
            }
            if("used".equals(status)||"expired".equals(status)||code==409){LicenseManager.markServerTrialUsed(context);return new Result(Status.ALREADY_USED,"Bu cihaz ücretsiz denemeyi daha önce kullandı");}
            if("denied".equals(status)||code==401||code==403)return new Result(Status.DENIED,json.optString("message","Trial isteği reddedildi"));
            return new Result(Status.INVALID_RESPONSE,json.optString("message","Tanımsız trial yanıtı"));
        }catch(IOException e){return new Result(Status.NETWORK_ERROR,"Trial başlatmak için internet bağlantısını kontrol edin");}
        catch(Exception e){return new Result(Status.INVALID_RESPONSE,"Trial sunucu yanıtı doğrulanamadı");}
        finally{if(connection!=null)connection.disconnect();}
    }

    private static String readLimited(InputStream in)throws IOException{
        if(in==null)return "";try(InputStream input=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[4096];int n,total=0;while((n=input.read(b))!=-1){total+=n;if(total>MAX_RESPONSE_BYTES)throw new IOException("response too large");out.write(b,0,n);}return out.toString(StandardCharsets.UTF_8.name());
        }
    }
    private TrialService(){}
}
