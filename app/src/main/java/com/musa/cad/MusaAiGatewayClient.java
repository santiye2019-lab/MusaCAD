package com.musa.cad;

import org.json.JSONArray;
import org.json.JSONObject;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/**
 * HTTPS client for the MusaCAD-owned Gandalf gateway.
 *
 * The Android app never talks to an OpenAI API key directly. The gateway URL
 * is build-time configuration and requests require a short-lived bearer token
 * kept only in process memory.
 */
public final class MusaAiGatewayClient {
    public interface Callback {
        void onSuccess(Response response);
        void onError(String message);
    }

    public static final class SuggestedAction {
        public final String command,description;
        SuggestedAction(String command,String description){
            this.command=command==null?"":command.trim();
            this.description=description==null?"":description.trim();
        }
    }

    public static final class Response {
        public final String reply;
        public final List<SuggestedAction> actions;
        Response(String reply,List<SuggestedAction>actions){
            this.reply=reply==null?"":reply.trim();
            this.actions=Collections.unmodifiableList(new ArrayList<>(actions));
        }
    }

    public static void analyze(String prompt,MusaAiProjectPacket packet,Callback callback){
        if(callback==null)return;
        final String endpoint=BuildConfig.AI_GATEWAY_URL==null?"":BuildConfig.AI_GATEWAY_URL.trim();
        final String token=MusaAiCloudSession.token();
        if(endpoint.isEmpty()){callback.onError("Gandalf ağ geçidi yapılandırılmamış.");return;}
        if(!endpoint.startsWith("https://")){callback.onError("Gandalf ağ geçidi HTTPS olmalı.");return;}
        if(token.isEmpty()){callback.onError("Gandalf çevrimiçi oturumu geçerli değil.");return;}

        new Thread(()->{
            HttpURLConnection connection=null;
            try{
                JSONObject body=new JSONObject();
                body.put("schema","musacad.gandalf.request.v1");
                body.put("prompt",prompt==null?"":prompt.trim());
                body.put("project",new JSONObject(packet==null?"{}":packet.json));
                body.put("client_capabilities",new JSONArray(Arrays.asList(
                    "project_analysis","technical_report","cad_action_suggestions"
                )));

                URL url=new URL(endpoint);
                connection=(HttpURLConnection)url.openConnection();
                connection.setRequestMethod("POST");
                connection.setConnectTimeout(20000);
                connection.setReadTimeout(60000);
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
                connection.setRequestProperty("Accept","application/json");
                connection.setRequestProperty("Authorization","Bearer "+token);

                byte[]payload=body.toString().getBytes(StandardCharsets.UTF_8);
                connection.setFixedLengthStreamingMode(payload.length);
                try(OutputStream out=connection.getOutputStream()){out.write(payload);}

                int code=connection.getResponseCode();
                InputStream stream=code>=200&&code<300?connection.getInputStream():connection.getErrorStream();
                String text=read(stream);
                if(code<200||code>=300)throw new IOException("HTTP "+code+(text.isEmpty()?"":" • "+clip(text,300)));

                JSONObject json=new JSONObject(text);
                String reply=json.optString("reply","");
                ArrayList<SuggestedAction>actions=new ArrayList<>();
                JSONArray arr=json.optJSONArray("actions");
                if(arr!=null){
                    for(int i=0;i<arr.length()&&i<20;i++){
                        JSONObject a=arr.optJSONObject(i);if(a==null)continue;
                        String command=a.optString("command","").trim();
                        String description=a.optString("description","").trim();
                        if(!command.isEmpty())actions.add(new SuggestedAction(command,description));
                    }
                }
                if(reply.trim().isEmpty()&&actions.isEmpty())throw new IOException("Gandalf yanıtı boş.");
                callback.onSuccess(new Response(reply,actions));
            }catch(Exception e){
                callback.onError("Gandalf bağlantısı başarısız: "+safeMessage(e));
            }finally{
                if(connection!=null)connection.disconnect();
            }
        },"MusaCAD-Gandalf").start();
    }

    private static String read(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream src=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[]buf=new byte[8192];int n;int total=0;
            while((n=src.read(buf))>=0){
                total+=n;if(total>2_000_000)throw new IOException("Yanıt boyutu sınırı aşıldı.");
                out.write(buf,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }
    private static String safeMessage(Exception e){
        String m=e==null?"":e.getMessage();
        return m==null||m.trim().isEmpty()?"Bilinmeyen ağ hatası":clip(m.trim(),320);
    }
    private static String clip(String s,int max){return s.length()<=max?s:s.substring(0,max)+"…";}
    private MusaAiGatewayClient(){}
}
