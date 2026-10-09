package com.musa.cad;

import android.content.Context;
import org.json.*;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLException;
import java.io.*;
import java.net.URL;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.net.ConnectException;
import java.net.SocketException;
import java.nio.charset.StandardCharsets;
import java.util.*;

/**
 * Secure MusaCAD cloud-AI client.
 *
 * The OpenAI API key never exists in the Android app. Requests go only to the
 * configured MusaCAD HTTPS gateway using a short-lived session token.
 */
public final class MusaAiCloudService {
    private static final int TIMEOUT_MS=90000;
    private static final int MAX_RESPONSE_BYTES=512*1024;
    private static final class OversizedResponseException extends IOException {}

    public enum Status { OK, NOT_CONFIGURED, ACCESS_REQUIRED, NETWORK_ERROR, QUOTA_EXHAUSTED, MODEL_TIMEOUT, UPSTREAM_UNAVAILABLE, DENIED, INVALID_RESPONSE }

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
        /** Verified response provider, not an Android-selected untrusted URL. */
        public final String provider,model;
        public final List<Action> actions;
        public final List<Source> sources;
        public final boolean webUsed;
        Result(Status status,String text,String message,Collection<Action>actions,boolean webUsed){
            this(status,text,message,actions,null,webUsed);
        }
        Result(Status status,String text,String message,Collection<Action>actions,Collection<Source>sources,boolean webUsed){
            this(status,text,message,actions,sources,webUsed,"","");
        }
        Result(Status status,String text,String message,Collection<Action>actions,Collection<Source>sources,
               boolean webUsed,String provider,String model){
            this.status=status;
            this.provider=provider==null?"":provider;
            this.model=model==null?"":model;
            this.text=text==null?"":text;
            this.message=message==null?"":message;
            this.actions=Collections.unmodifiableList(new ArrayList<>(actions==null?Collections.emptyList():actions));
            this.sources=Collections.unmodifiableList(new ArrayList<>(sources==null?Collections.emptyList():sources));
            this.webUsed=webUsed;
        }
        public boolean ok(){return status==Status.OK;}
    }

    public static Result analyze(Context context,MusaAiDrawingIndex index,String fileName,String rawPrompt){
        return analyzeInternal(context,index,fileName,rawPrompt,null,null,"all");
    }

    /** User-consented chat continuity and on-device engineering evidence.
     * Never includes raw DWG, image or entire local price book.
     */
    public static Result analyzeWithContext(Context context,MusaAiDrawingIndex index,
                                            String fileName,String rawPrompt,
                                            String recentTurns,String localEvidence){
        return analyzeInternal(context,index,fileName,rawPrompt,null,null,"all",
            recentTurns,localEvidence);
    }

    public static Result analyzePackage(Context context,MusaAiDrawingIndex index,String fileName,
                                        Collection<MusaAiProjectPackage.Drawing>drawings,String rawPrompt){
        if(drawings==null||drawings.isEmpty())
            return new Result(Status.INVALID_RESPONSE,"","Proje Paketi bağlamı hazırlanamadı",null,false);
        MusaAiProjectPackage.Result local=MusaAiProjectPackage.generate(drawings,"proje paketi");
        String packageJson=MusaAiCadPackageJson.build(drawings,local.matched?local.text:"");
        return analyzeInternal(context,index,fileName,rawPrompt,packageJson,null,"all");
    }

    public static Result analyzePackageWithContext(Context context,MusaAiDrawingIndex index,
                                                   String fileName,
                                                   Collection<MusaAiProjectPackage.Drawing>drawings,
                                                   String rawPrompt,String recentTurns,
                                                   String localEvidence){
        if(drawings==null||drawings.isEmpty())
            return new Result(Status.INVALID_RESPONSE,"","Proje Paketi bağlamı hazırlanamadı",null,false);
        MusaAiProjectPackage.Result local=MusaAiProjectPackage.generate(drawings,"proje paketi");
        String packageJson=MusaAiCadPackageJson.build(drawings,local.matched?local.text:"");
        return analyzeInternal(context,index,fileName,rawPrompt,packageJson,null,"all",
            recentTurns,localEvidence);
    }

    public static Result analyzeHybridWithContext(Context context,MusaAiDrawingIndex index,
                                                  String fileName,String rawPrompt,String scope,
                                                  JSONObject visualEvidence,String recentTurns,
                                                  String localEvidence){
        return analyzeInternal(context,index,fileName,rawPrompt,null,visualEvidence,scope,
            recentTurns,localEvidence);
    }

    public static Result analyzeHybrid(Context context,MusaAiDrawingIndex index,String fileName,
                                      String rawPrompt,String scope,JSONObject visualEvidence){
        return analyzeInternal(context,index,fileName,rawPrompt,null,visualEvidence,scope);
    }

    private static Result analyzeInternal(Context context,MusaAiDrawingIndex index,String fileName,
                                          String rawPrompt,String packageJson,JSONObject visualEvidence,String scope){
        return analyzeInternal(context,index,fileName,rawPrompt,packageJson,visualEvidence,scope,"","");
    }

    private static Result analyzeInternal(Context context,MusaAiDrawingIndex index,String fileName,
                                          String rawPrompt,String packageJson,JSONObject visualEvidence,String scope,
                                          String recentTurns,String localEvidence){
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
            body.put("analysisScope",scope==null?"all":scope);
            // Include only bounded, consented conversation continuity.
            if(recentTurns!=null&&!recentTurns.trim().isEmpty())
                body.put("previousChat",recentTurns.substring(0,Math.min(2600,recentTurns.length())));
            if(localEvidence!=null&&!localEvidence.trim().isEmpty())
                body.put("localEvidence",localEvidence.substring(0,Math.min(2200,localEvidence.length())));
            if(visualEvidence!=null)body.put("visualEvidence",visualEvidence);
            String expertProfile=MusaAiMechanicalExpert.cloudProfile(rawPrompt);
            if(expertProfile.isEmpty())expertProfile=MusaAiDisciplineExpert.cloudProfile(rawPrompt);
            if(!expertProfile.isEmpty())body.put("expertProfile",expertProfile);
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
            // HTTP errors can be HTML or empty. Recognize request-size errors
            // before JSON parsing so an oversized visual sweep is actionable.
            if(code==413||code==431)return new Result(Status.INVALID_RESPONSE,"",
                "Görsel/CAD isteği sunucu boyut sınırını aştı (HTTP "+code+
                "). Daha az görsel içeren gruplarla yeniden deneyin.",null,false);
            String response=readLimited(code>=200&&code<300?connection.getInputStream():connection.getErrorStream());
            if(response.isEmpty())return new Result(Status.INVALID_RESPONSE,"",
                "Gandalf AI sunucusu boş yanıt döndürdü (HTTP "+code+").",null,false);

            JSONObject json;
            try{json=new JSONObject(response);}
            catch(JSONException malformed){
                return new Result(Status.INVALID_RESPONSE,"",
                    "Gandalf AI sunucusu JSON olmayan yanıt döndürdü (HTTP "+code+
                    "). Sunucu yönlendirmesi veya ağ geçidi yanıtı kontrol edilmeli.",null,false);
            }
            if(code==401||code==403){
                MusaAiSessionService.clearCache();
                return new Result(Status.DENIED,"","AI oturumu reddedildi (HTTP "+code+"): "+
                    reason(json,"Giriş/lisans yetkisini kontrol edin."),null,false);
            }
            if(code==429)
                return new Result(Status.QUOTA_EXHAUSTED,"",
                    "AI kota sınırına ulaşıldı (HTTP 429). "+reason(json,
                    "Kota yenilenince tekrar deneyin; bu sırada çevrim dışı CAD analizi kullanılabilir."),null,false);
            if(code==504||code==408)
                return new Result(Status.MODEL_TIMEOUT,"",
                    "AI görsel yanıt süresi aşıldı (HTTP "+code+"). "+
                    reason(json,"Daha küçük görsel gruplarla yeniden deneyin."),null,false);
            if(code==502||code==503)
                return new Result(Status.UPSTREAM_UNAVAILABLE,"",
                    "AI sağlayıcısına erişilemiyor (HTTP "+code+"). "+
                    reason(json,"AI sunucusunu ve sağlayıcı durumunu kontrol edin."),null,false);
            if(code<200||code>=300)
                return new Result(Status.INVALID_RESPONSE,"",
                    "AI isteği reddedildi (HTTP "+code+"): "+
                    reason(json,"Sunucu isteği doğrulayamadı."),null,false);
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
            return new Result(Status.OK,text,"",actions,sources,webUsed,
                json.optString("provider",""),json.optString("model",""));
        }catch(SocketTimeoutException e){
            return new Result(Status.MODEL_TIMEOUT,"",
                "Gandalf model/görsel yanıtı "+(TIMEOUT_MS/1000)+
                " saniyelik sınırı aştı. Sunucu veya model gecikmesi olabilir; görsel analiz tamamlanmadı.",null,false);
        }catch(OversizedResponseException e){
            return new Result(Status.INVALID_RESPONSE,"",
                "Gandalf AI sunucu yanıtı "+(MAX_RESPONSE_BYTES/1024)+
                " KB güvenli sınırını aştı. Sunucudaki yanıt boyutu kontrol edilmeli.",null,false);
        }catch(UnknownHostException e){
            return new Result(Status.NETWORK_ERROR,"",
                "Gandalf AI sunucu adı çözümlenemedi (DNS). AI API URL'sini ve DNS bağlantısını kontrol edin.",null,false);
        }catch(SSLException e){
            return new Result(Status.NETWORK_ERROR,"",
                "Gandalf AI HTTPS/TLS bağlantısı kurulamadı. Sunucunun sertifikası, alan adı ve TLS yapılandırması kontrol edilmeli.",null,false);
        }catch(ConnectException e){
            return new Result(Status.NETWORK_ERROR,"",
                "Gandalf AI sunucusu TCP bağlantısını kabul etmedi. Worker adresi ve sunucu erişimi kontrol edilmeli.",null,false);
        }catch(SocketException e){
            return new Result(Status.NETWORK_ERROR,"",
                "Gandalf AI bağlantısı sunucu/ağ tarafından sıfırlandı veya kesildi. Görsel istek boyutunu ve sunucu günlüklerini kontrol edin.",null,false);
        }catch(IOException e){
            return new Result(Status.NETWORK_ERROR,"",
                "Gandalf AI veri gönderme/alma sırasında G/Ç hatası oluştu. Sunucu günlükleri ve ağ bağlantısı kontrol edilmeli.",null,false);
        }catch(Exception e){
            return new Result(Status.INVALID_RESPONSE,"","Gandalf AI yanıtı işlenemedi",null,false);
        }finally{if(connection!=null)connection.disconnect();}
    }

    private static String reason(JSONObject error,String fallback){
        String msg=error.optString("message","").trim();
        if(msg.isEmpty())msg=fallback;
        return msg.substring(0,Math.min(240,msg.length()));
    }

    private static String readLimited(InputStream in)throws IOException{
        if(in==null)return "";
        try(InputStream input=in;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] b=new byte[8192];int n,total=0;
            while((n=input.read(b))!=-1){
                total+=n;if(total>MAX_RESPONSE_BYTES)throw new OversizedResponseException();
                out.write(b,0,n);
            }
            return out.toString(StandardCharsets.UTF_8.name());
        }
    }

    private MusaAiCloudService(){}
}
