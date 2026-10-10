package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;
import org.json.JSONArray;
import org.json.JSONObject;
import javax.net.ssl.HttpsURLConnection;
import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicLong;

/** Resumable background-only file transfer. Never deletes a local drawing. */
public final class MusaAiProjectSync {
    private static final int CHUNK_BYTES=1024*1024, TIMEOUT_MS=20000;
    private static final String PREF="gandalf_auto_project_upload";
    private static final AtomicLong newest=new AtomicLong();
    private static final ExecutorService WORK=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-project-upload");t.setPriority(Thread.MIN_PRIORITY);return t;
    });
    public interface Listener{void status(String id,int percent,String message);}
    public static final class Entry{
        public final String id,name; public final File file;
        public final DxfParser.Result parsed;
        public Entry(String id,String name,File file){this(id,name,file,null);}
        public Entry(String id,String name,File file,DxfParser.Result parsed){
            this.id=id;this.name=name;this.file=file;this.parsed=parsed;
        }
    }
    public static boolean enabled(Context app){
        return app.getSharedPreferences(PREF,0).getBoolean("enabled",
            MusaAiSessionService.developerCached());
    }
    public static void setEnabled(Context app,boolean value){
        app.getSharedPreferences(PREF,0).edit().putBoolean("enabled",value).apply();
    }
    public static void submit(Context app,List<Entry> entries,Listener listener){
        final Context context=app.getApplicationContext();
        final List<Entry> open=new ArrayList<>(entries);
        if(open.size()>4)return;
        SharedPreferences pref=context.getSharedPreferences(PREF,0);
        long revision;
        synchronized(MusaAiProjectSync.class){
            revision=Math.max(newest.get(),pref.getLong("revision",0))+1;
            if(!pref.edit().putLong("revision",revision).commit())return;
            newest.set(revision);
        }
        final long rev=revision;
        WORK.execute(()->{
            if(rev!=newest.get())return;
            try{
                MusaAiSessionService.Result session=MusaAiSessionService.get(context);
                if(!session.active()||!enabled(context))return;
                JSONArray ids=new JSONArray();for(Entry e:open)ids.put(e.id);
                JSONObject body=new JSONObject().put("revision",rev).put("openProjectIds",ids);
                post("/v1/projects/sync-open","POST",session.token,
                    body.toString().getBytes(StandardCharsets.UTF_8),null);
                for(Entry e:open){
                    if(rev!=newest.get())return;
                    if(e.file!=null&&e.file.isFile())upload(e,session.token,rev,listener);
                }
            }catch(Exception error){
                if(rev==newest.get()&&!open.isEmpty()&&listener!=null){
                    String message=String.valueOf(error.getMessage()).contains("HTTP 503")
                        ?"Gandalf proje deposu sunucuda henüz etkin değil"
                        :"Gandalf bulut eşitlemesi başarısız • yeniden denenecek";
                    listener.status(open.get(0).id,-1,message);
                }
            }
        });
    }
    private static void status(Listener l,Entry e,int percent,String msg){
        if(l!=null)l.status(e.id,percent,msg);
    }
    private static void upload(Entry e,String token,long rev,Listener listener){
        try{
            long size=e.file.length();
            if(size<1||size>512L*1024*1024){
                status(listener,e,-1,"Proje 512 MB aktarım sınırını aşıyor");return;
            }
            status(listener,e,0,"Gandalf paketi hazırlanıyor");
            MessageDigest fileDigest=MessageDigest.getInstance("SHA-256");
            MessageDigest chunkRootDigest=MessageDigest.getInstance("SHA-256");
            byte[] block=new byte[CHUNK_BYTES];
            try(InputStream in=new FileInputStream(e.file)){
                int n;
                while((n=readPart(in,block))>0){
                    if(rev!=newest.get())return;
                    fileDigest.update(block,0,n);
                    MessageDigest each=MessageDigest.getInstance("SHA-256");
                    each.update(block,0,n);
                    chunkRootDigest.update(each.digest());
                }
            }
            JSONObject manifest=new JSONObject().put("projectId",e.id)
                .put("fileName",e.name).put("sizeBytes",size)
                .put("sha256",hex(fileDigest.digest()))
                .put("chunkRootSha256",hex(chunkRootDigest.digest()));
            JSONObject init=post("/v1/projects/init","POST",token,
                manifest.toString().getBytes(StandardCharsets.UTF_8),null);
            if(init.optBoolean("complete",false)){
                status(listener,e,100,"DWG sunucuda hazır • %100");
                uploadNumericIndex(e,token,rev,listener);
                return;
            }
            int count=(int)((size+CHUNK_BYTES-1)/CHUNK_BYTES);
            if(init.optInt("chunkBytes")!=CHUNK_BYTES||init.optInt("partCount")!=count)
                throw new IOException("chunk metadata mismatch");
            Set<Integer> received=new HashSet<>();
            JSONArray done=init.optJSONArray("receivedParts");
            if(done!=null)for(int i=0;i<done.length();i++){
                int part=done.optInt(i,-1);if(part>=0&&part<count)received.add(part);
            }
            byte[] chunk=new byte[CHUNK_BYTES];long confirmed=0;
            MessageDigest transmittedDigest=MessageDigest.getInstance("SHA-256");
            for(int part:received)confirmed+=Math.min(CHUNK_BYTES,size-(long)part*CHUNK_BYTES);
            try(InputStream in=new FileInputStream(e.file)){
                for(int part=0;part<count;part++){
                    if(rev!=newest.get())return;
                    int amount=(int)Math.min(CHUNK_BYTES,size-(long)part*CHUNK_BYTES);
                    int countRead=readPart(in,chunk,amount);
                    if(countRead!=amount)throw new EOFException("DWG changed");
                    transmittedDigest.update(chunk,0,amount);
                    if(!received.contains(part)){
                        byte[] payload=Arrays.copyOf(chunk,amount);
                        String hash=hex(MessageDigest.getInstance("SHA-256").digest(payload));
                        post("/v1/projects/"+e.id+"/chunks/"+part,"PUT",token,
                            payload,hash);
                        confirmed+=amount;
                    }
                    int pct=(int)Math.min(99,confirmed*100/size);
                    status(listener,e,pct,"Gandalf sunucu aktarımı • %"+pct);
                }
            }
            if(rev!=newest.get())return;
            if(!hex(transmittedDigest.digest()).equals(manifest.getString("sha256")))
                throw new IOException("project changed during transfer");
            JSONObject complete=post("/v1/projects/"+e.id+"/complete","POST",
                token,new byte[0],null);
            if(!complete.optBoolean("complete")||complete.optLong("verifiedBytes",-1)!=size)
                throw new IOException("server verification failed");
            status(listener,e,100,"DWG sunucuda hazır • %100");
            uploadNumericIndex(e,token,rev,listener);
        }catch(Exception ex){
            if(rev==newest.get()){
                String message=String.valueOf(ex.getMessage()).contains("HTTP 503")
                    ?"Gandalf proje deposu sunucuda henüz etkin değil"
                    :"Aktarım tamamlanamadı • bağlantıyı kontrol edin";
                status(listener,e,-1,message);
            }
        }
    }
    private static int readPart(InputStream in,byte[] buffer)throws IOException{
        return readPart(in,buffer,buffer.length);
    }
    private static int readPart(InputStream in,byte[] buffer,int limit)throws IOException{
        int offset=0;
        while(offset<limit){
            int n=in.read(buffer,offset,limit-offset);
            if(n<0)break;
            if(n==0)continue;
            offset+=n;
        }
        return offset;
    }
    /**
     * Bounded numerical companion to the complete raw DWG/DXF upload.
     * Reports partial coverage honestly if the CAD item cap is reached.
     * Full high-resolution visual sheet packs are a separate task.
     */
    private static void uploadNumericIndex(Entry e,String token,long rev,Listener listener){
        if(e.parsed==null||rev!=newest.get())return;
        try{
            status(listener,e,0,"Gandalf sayısal CAD indeksi hazırlanıyor");
            MusaAiDrawingIndex index=e.parsed.aiDrawingIndex(40001);
            if(rev!=newest.get())return;
            int actual=index.items().size(),count=Math.min(40000,actual);
            boolean completeCoverage=actual<40001;
            int pages=Math.max(1,(count+499)/500);
            JSONObject init=new JSONObject().put("pageCount",pages)
                .put("itemCount",count).put("coverageComplete",completeCoverage);
            JSONObject remote=post("/v1/projects/"+e.id+"/index/init","POST",token,
                init.toString().getBytes(StandardCharsets.UTF_8),null);
            if(remote.optBoolean("ready",false)){
                status(listener,e,100,completeCoverage
                    ?"DWG ve sayısal CAD indeksi hazır"
                    :"DWG hazır • CAD indeksi kısmi (40.000 nesne sınırı)");
                return;
            }
            Set<Integer> received=new HashSet<>();
            JSONArray prior=remote.optJSONArray("receivedPages");
            if(prior!=null)for(int i=0;i<prior.length();i++){
                int n=prior.optInt(i,-1);if(n>=0&&n<pages)received.add(n);
            }
            for(int page=0;page<pages;page++){
                if(rev!=newest.get())return;
                if(!received.contains(page)){
                    int first=page*500,last=Math.min(count,first+500);
                    MusaAiDrawingIndex slice=new MusaAiDrawingIndex(
                        index.layout,index.entityCount,index.oleObjectCount,
                        index.allLayers,index.visibleLayers,
                        index.items().subList(first,last),index.unitName);
                    JSONObject json=new JSONObject()
                        .put("schema","musacad-index-page/v1").put("projectId",e.id)
                        .put("pageIndex",page).put("pageCount",pages)
                        .put("cad",new JSONObject(MusaAiCadJson.build(slice,e.name,500)));
                    byte[] body=json.toString().getBytes(StandardCharsets.UTF_8);
                    if(body.length>512*1024)
                        throw new IOException("CAD numeric page too large");
                    String checksum=hex(MessageDigest.getInstance("SHA-256").digest(body));
                    JSONObject response=post("/v1/projects/"+e.id+"/index/pages/"+page,
                        "PUT",token,body,checksum);
                    if(!response.optBoolean("received",false))
                        throw new IOException("CAD page not acknowledged");
                }
                int percentage=Math.min(99,100*(page+1)/pages);
                status(listener,e,percentage,"Sayısal CAD indeks aktarımı • %"+percentage);
            }
            JSONObject done=post("/v1/projects/"+e.id+"/index/complete",
                "POST",token,new byte[0],null);
            if(!done.optBoolean("ready",false))
                throw new IOException("CAD index verification failed");
            status(listener,e,100,completeCoverage
                ?"DWG ve sayısal CAD indeksi hazır"
                :"DWG hazır • CAD indeksi kısmi (40.000 nesne sınırı)");
        }catch(Exception ex){
            if(rev==newest.get())
                status(listener,e,-1,"DWG aktarıldı; sayısal CAD indeksi tamamlanamadı");
        }
    }

    private static String hex(byte[] src){
        StringBuilder sb=new StringBuilder();
        for(byte b:src)sb.append(String.format(Locale.ROOT,"%02x",b&255));
        return sb.toString();
    }
    private static JSONObject post(String path,String method,String token,byte[] bytes,
                                   String digest)throws Exception{
        URL base=new URL(BuildConfig.AI_API_URL);
        if(!"https".equalsIgnoreCase(base.getProtocol()))throw new IOException("HTTPS required");
        URL url=new URL(base.getProtocol(),base.getHost(),base.getPort(),path);
        HttpsURLConnection c=(HttpsURLConnection)url.openConnection();
        try{
            c.setConnectTimeout(TIMEOUT_MS);c.setReadTimeout(TIMEOUT_MS);
            c.setInstanceFollowRedirects(false);c.setRequestMethod(method);
            c.setRequestProperty("Authorization","Bearer "+token);
            c.setRequestProperty("Content-Type",
                digest==null?"application/json":"application/octet-stream");
            if(digest!=null)c.setRequestProperty(
                path.contains("/index/pages/")?"X-Page-SHA256":"X-Chunk-SHA256",
                digest);
            c.setDoOutput(true);c.setFixedLengthStreamingMode(bytes.length);
            try(OutputStream out=c.getOutputStream()){out.write(bytes);}
            if(c.getResponseCode()!=200)throw new IOException("HTTP "+c.getResponseCode());
            try(InputStream in=c.getInputStream();ByteArrayOutputStream out=new ByteArrayOutputStream()){
                byte[] b=new byte[2048];int n;
                while((n=in.read(b))!=-1){
                    if(out.size()+n>32768)throw new IOException("response too large");
                    out.write(b,0,n);
                }
                return new JSONObject(out.toString(StandardCharsets.UTF_8.name()));
            }
        }finally{c.disconnect();}
    }
    private MusaAiProjectSync(){}
}
