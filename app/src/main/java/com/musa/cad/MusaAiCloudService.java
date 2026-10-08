package com.musa.cad;

import android.content.Context;
import org.json.*;
import javax.net.ssl.HttpsURLConnection;
import java.io.*;
import java.net.URL;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.util.*;

/**
 * Secure MusaCAD cloud-AI client.
 *
 * The OpenAI API key never exists in the Android app. Requests go only to the
 * configured MusaCAD HTTPS gateway using a short-lived session token.
 */
public final class MusaAiCloudService {
    private static final int TIMEOUT_MS=30000;
    private static final int MAX_RESPONSE_BYTES=512*1024;

    public enum Status { OK, NOT_CONFIGURED, ACCESS_REQUIRED, NETWORK_ERROR, DENIED, INVALID_RESPONSE }

    public static final class Action {
        public final String name,arguments,reason;
        Action(String name,String arguments,String reason){
            this.name=name==null?"":name;
            this.arguments=arguments==null?"{}":arguments;
            this.reason=reason==null?"":reason;
        }
    }

    public static final class Source {
        public final String title,url;
        Source(String title,String url){
            this.title=title==null?"":title;
            this.url=url==null?"":url;
        }
    }

    public static final class Result {
        public final Status status;
        public final String text,message;
        public final List<Action> actions;
        public final List<Source> sources;
        public final boolean webUsed;
        Result(Status status,String text,String message,Collection<Action>actions,boolean webUsed){
            this(status,text,message,actions,null,webUsed);
        }
        Result(Status status,String text,String message,Collection<Action>actions,Collection<Source>sources,boolean webUsed){
            this.status=status;
            this.text=text==null?"":text;
            this.message=message==null?"":message;
            this.actions=Collections.unmodifiableList(new ArrayList<>(actions==null?Collections.emptyList():actions));
            this.sources=Collections.unmodifiableList(new ArrayList<>(sources==null?Collections.emptyList():sources));
            this.webUsed=webUsed;
        }
        public boolean ok(){return status==Status.OK;}
    }

    public static Result analyze(Context context,MusaAiDrawingIndex index,String fileName,String rawPrompt){
        return analyzeInternal(context,index,fileName,rawPrompt,null);
    }

    public static Result analyzePackage(Context context,MusaAiDrawingIndex index,String fileName,
                                        Collection<MusaAiProjectPackage.Drawing>drawings,String rawPrompt){
        if(drawings==null||drawings.isEmpty())
            return new Result(Status.INVALID_RESPONSE,"","Proje Paketi bağlamı hazırlanamadı",null,false);
        MusaAiProjectPackage.Result local=MusaAiProjectPackage.generate(drawings,"proje paketi");
        String packageJson=MusaAiCadPackageJson.build(drawings,local.matched?local.text:"");
        return analyzeInternal(context,index,fileName,rawPrompt,packageJson);
    }

    private static Result analyzeInternal(Context context,MusaAiDrawingIndex index,String fileName,String rawPrompt,String packageJson){
        if(context==null||index==null)return new Result(Status.INVALID_RESPONSE,"","Çizim bağlamı hazırlanamadı",null,false);

        String endpoint=BuildConfig.AI_API_URL==null?"":BuildConfig.AI_API_URL.trim();
        if(endpoint.isEmpty())return new Result(Status.NOT_CONFIGURED,"","Gandalf AI sunucusu henüz yapılandırılmadı",null,false);
        if(!endpoint.startsWith("https://"))return new Result(Status.NOT_CONFIGURED,"","Gandalf AI adresi HTTPS olmalıdır",null,false);

        MusaAiSessionService.Result session=MusaAiSessionService.get(context);
        if(!session.active()){
            Status mapped=session.status==MusaAiSessionService.Status.ACCESS_REQUIRED?Status.ACCESS_REQUIRED:
                session.status==MusaAiSessionService.Status.NOT_CONFIGURED?Status.NOT_CONFIGURED:
                session.status==MusaAiSessionService.Status.NETWORK_ERROR?Status.NETWORK_ERROR:
                session.status==MusaAiSessionService.Status.DENIED?Status.DENIED:Status.INVALID_RESPONSE;
            return new Result(mapped,"",session.message,null,false);
        }

        String prompt=MusaAiCloudPolicy.promptForCloud(rawPrompt);
        boolean allowWeb=MusaAiCloudPolicy.allowWeb(rawPrompt);
        boolean allowEditProposals=MusaAiCloudPolicy.allowEditProposals(rawPrompt);

        HttpsURLConnection connection=null;
        try{
            connection=(HttpsURLConnection)new URL(endpoint).openConnection();
            connection.setConnectTimeout(TIMEOUT_MS);connection.setReadTimeout(TIMEOUT_MS);
            connection.setInstanceFollowRedirects(false);connection.setUseCaches(false);
            connection.setRequestMethod("POST");connection.setDoOutput(true);
            connection.setRequestProperty("Authorization","Bearer "+session.token);
            connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
            connection.setRequestProperty("Accept","application/json");

            JSONObject body=new JSONObject();
            body.put("prompt",prompt);
            body.put("allowWeb",allowWeb);
            body.put("allowEditProposals",allowEditProposals);
            String expertProfile=MusaAiMechanicalExpert.cloudProfile(rawPrompt);
            if(expertProfile.isEmpty())expertProfile=MusaAiDisciplineExpert.cloudProfile(rawPrompt);
            if(!expertProfile.isEmpty())body.put("expertProfile",expertProfile);
            // The cloud model receives the whole bounded CAD index and also a
            // locally grounded review summary, so focus does not erase cross-discipline context.
            MusaAiReviewScope reviewScope=MusaAiReviewScope.parse(rawPrompt);
            if(reviewScope!=null){
                JSONObject scopeJson=new JSONObject();
                scopeJson.put("discipline",reviewScope.discipline.name());
                scopeJson.put("system",reviewScope.system);
                scopeJson.put("label",reviewScope.label);
                body.put("reviewScope",scopeJson);
                String evidence=reviewScope.localEvidence(index,fileName);
                if(evidence.length()>10000)evidence=evidence.substring(0,10000);
                body.put("localEvidence",evidence);
            }
            body.put("cad",new JSONObject(MusaAiCadJson.build(index,fileName)));
            if(packageJson!=null&&!packageJson.trim().isEmpty())
                body.put("cadPackage",new JSONObject(packageJson));
            JSONObject client=new JSONObject();
            client.put("app","MusaCAD");
            client.put("versionName",BuildConfig.VERSION_NAME);
            client.put("versionCode",BuildConfig.VERSION_CODE);
            client.put("distribution",BuildConfig.DISTRIBUTION_CHANNEL);
            body.put("client",client);

            byte[] bytes=body.toString().getBytes(StandardCharsets.UTF_8);
            connection.setFixedLengthStreamingMode(bytes.length);
            try(OutputStream out=connection.getOutputStream()){out.write(bytes);}

            int code=connection.getResponseCode();
            if(code>=300&&code<400)return new Result(Status.DENIED,"","Gandalf AI sunucusu yönlendirme döndürdü",null,false);
            String response=readLimited(code>=200&&code<300?connection.getInputStream():connection.getErrorStream());
            if(response.isEmpty())return new Result(Status.INVALID_RESPONSE,"","Boş Gandalf AI yanıtı",null,false);

            JSONObject json=new JSONObject(response);
            if(code==401||code==403){
                MusaAiSessionService.clearCache();
                return new Result(Status.DENIED,"",json.optString("message","Gandalf AI erişimi reddedildi"),null,false);
            }
            if(code<200||code>=300)return new Result(Status.INVALID_RESPONSE,"",json.optString("message","Gandalf AI isteği tamamlanamadı"),null,false);
            if(!"ok".equalsIgnoreCase(json.optString("status","")))
                return new Result(Status.INVALID_RESPONSE,"",json.optString("message","Gandalf AI yanıtı geçersiz"),null,false);

            String text=json.optString("reply","").trim();
            ArrayList<Action>actions=new ArrayList<>();
            JSONArray list=json.optJSONArray("actions");
            if(list!=null)for(int i=0;i<list.length();i++){
                JSONObject a=list.optJSONObject(i);if(a==null)continue;
                String name=a.optString("name","").trim();if(name.isEmpty())continue;
                Object args=a.opt("arguments");
                String arguments=args instanceof JSONObject||args instanceof JSONArray?args.toString():String.valueOf(args==null?"{}":args);
                actions.add(new Action(name,arguments,a.optString("reason","")));
            }
            ArrayList<Source>sources=new ArrayList<>();
            JSONArray sourceList=json.optJSONArray("sources");
            if(sourceList!=null)for(int i=0;i<sourceList.length()&&sources.size()<20;i++){
                JSONObject s=sourceList.optJSONObject(i);if(s==null)continue;
                String url=s.optString("url","").trim();
                if(!(url.startsWith("https://")||url.startsWith("http://")))continue;
                sources.add(new Source(s.optString("title","").trim(),url));
            }
            boolean webUsed=json.optBoolean("webUsed",false);
            if(text.isEmpty()&&!actions.isEmpty())text="Gandalf AI "+actions.size()+" adet çizim işlemi önerdi.";
            if(text.isEmpty())return new Result(Status.INVALID_RESPONSE,"","Gandalf AI boş yanıt döndürdü",actions,sources,webUsed);
            return new Result(Status.OK,text,"",actions,sources,webUsed);
        }catch(SocketTimeoutException e){
            return new Result(Status.NETWORK_ERROR,"","Gandalf AI sunucusundan süresi içinde yanıt alınamadı. Yerel analiz kullanılacak.",null,false);
        }catch(IOException e){
            return new Result(Status.NETWORK_ERROR,"","Gandalf AI için internet bağlantısını kontrol edin",null,false);
        }catch(Exception e){
            return new Result(Status.INVALID_RESPONSE,"","Gandalf AI yanıtı işlenemedi",null,false);
        }finally{if(connection!=null)connection.disconnect();}
    }

    private static String readLimited(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream input=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[8192];int n,total=0;
            while((n=input.read(b))!=-1){
                total+=n;if(total>MAX_RESPONSE_BYTES)throw new IOException("response too large");
                out.write(b,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private MusaAiCloudService(){}
}
