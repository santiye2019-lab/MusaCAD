package com.musa.cad;

import android.content.Context;
import org.json.JSONObject;
import javax.net.ssl.HttpsURLConnection;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

/** Verifies Google Play purchases on the MusaCAD backend before Pro entitlement is granted. */
public final class PlayPurchaseVerifier {
    private static final int TIMEOUT_MS=10000,MAX_RESPONSE_BYTES=32768;

    public enum Status { ACTIVE, PENDING, NOT_CONFIGURED, NETWORK_ERROR, INVALID_RESPONSE, DENIED }
    public static final class Result {
        public final Status status;
        public final String message;
        Result(Status status,String message){this.status=status;this.message=message;}
    }

    public static Result verify(Context context,String purchaseToken){
        if(context==null||purchaseToken==null||purchaseToken.trim().isEmpty())
            return new Result(Status.INVALID_RESPONSE,"Satın alma jetonu eksik");

        String endpoint=BuildConfig.PLAY_VERIFY_URL==null?"":BuildConfig.PLAY_VERIFY_URL.trim();
        String productId=BuildConfig.PLAY_YEARLY_PRODUCT_ID==null?"":BuildConfig.PLAY_YEARLY_PRODUCT_ID.trim();
        if(endpoint.isEmpty()||productId.isEmpty())
            return new Result(Status.NOT_CONFIGURED,"Google Play doğrulama sunucusu yapılandırılmadı");
        if(!endpoint.startsWith("https://"))
            return new Result(Status.NOT_CONFIGURED,"Google Play doğrulama adresi HTTPS olmalıdır");

        HttpsURLConnection connection=null;
        try{
            URL url=new URL(endpoint);
            connection=(HttpsURLConnection)url.openConnection();
            connection.setConnectTimeout(TIMEOUT_MS);
            connection.setReadTimeout(TIMEOUT_MS);
            connection.setInstanceFollowRedirects(false);
            connection.setRequestMethod("POST");
            connection.setDoOutput(true);
            connection.setUseCaches(false);
            connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
            connection.setRequestProperty("Accept","application/json");

            JSONObject request=new JSONObject();
            request.put("deviceId",LicenseManager.installationId(context));
            request.put("packageName",context.getPackageName());
            request.put("productId",productId);
            request.put("purchaseToken",purchaseToken.trim());
            request.put("purpose","annual_renewal");
            request.put("versionName",BuildConfig.VERSION_NAME);
            request.put("versionCode",BuildConfig.VERSION_CODE);

            byte[] body=request.toString().getBytes(StandardCharsets.UTF_8);
            connection.setFixedLengthStreamingMode(body.length);
            try(OutputStream out=connection.getOutputStream()){out.write(body);}

            int code=connection.getResponseCode();
            if(code>=300&&code<400)return new Result(Status.DENIED,"Doğrulama sunucusu yönlendirme döndürdü");
            InputStream stream=code>=200&&code<300?connection.getInputStream():connection.getErrorStream();
            String response=readLimited(stream);
            if(response.isEmpty())return new Result(Status.INVALID_RESPONSE,"Boş Google Play doğrulama yanıtı");

            JSONObject json=new JSONObject(response);
            String status=json.optString("status","").trim().toLowerCase(Locale.ROOT);
            String message=json.optString("message","");
            if("active".equals(status)&&code>=200&&code<300)
                return new Result(Status.ACTIVE,message);
            if("pending".equals(status)||code==202)
                return new Result(Status.PENDING,message.isEmpty()?"Ödeme beklemede":message);
            if("denied".equals(status)||"cancelled".equals(status)||code==401||code==403||code==409)
                return new Result(Status.DENIED,message.isEmpty()?"Google Play satın alımı doğrulanamadı":message);
            if("server_error".equals(status)||code>=500)
                return new Result(Status.NETWORK_ERROR,message.isEmpty()?"Google Play doğrulama servisine ulaşılamadı":message);
            return new Result(Status.INVALID_RESPONSE,message.isEmpty()?"Tanımsız Google Play doğrulama yanıtı":message);
        }catch(IOException e){
            return new Result(Status.NETWORK_ERROR,"Google Play doğrulaması için internet bağlantısını kontrol edin");
        }catch(Exception e){
            return new Result(Status.INVALID_RESPONSE,"Google Play doğrulama yanıtı işlenemedi");
        }finally{
            if(connection!=null)connection.disconnect();
        }
    }

    private static String readLimited(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream input=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buffer=new byte[4096];
            int n,total=0;
            while((n=input.read(buffer))!=-1){
                total+=n;
                if(total>MAX_RESPONSE_BYTES)throw new IOException("response too large");
                out.write(buffer,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private PlayPurchaseVerifier(){}
}
