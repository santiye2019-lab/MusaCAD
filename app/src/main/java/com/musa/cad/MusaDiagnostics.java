package com.musa.cad;

import android.content.Context;
import android.content.SharedPreferences;
import org.json.JSONObject;
import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import javax.net.ssl.HttpsURLConnection;

/**
 * Device-side bounded, anonymous DWG/DXF open diagnostics.
 * Debug builds default to automatic reporting; release builds opt in.
 * The Worker requires a signed developer session; no GitHub credentials
 * exist in the APK. File name, URI, path, drawing text and coordinates
 * NEVER enter a stored report.
 */
public final class MusaDiagnostics {
    private static final String PREFS="musacad_diagnostics_v1";
    private static final String KEY_ENABLED="automatic_report_upload";
    private static final int MAX_REPORT_FILES=24;
    private static final int MAX_JSON_BYTES=4096;
    private static final ExecutorService IO=Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-diagnostics");
        t.setPriority(Thread.MIN_PRIORITY);
        return t;
    });
    private MusaDiagnostics(){}

    public static boolean enabled(Context context){
        return context.getSharedPreferences(PREFS,Context.MODE_PRIVATE)
            .getBoolean(KEY_ENABLED,BuildConfig.DEBUG);
    }

    public static void setEnabled(Context context,boolean enabled){
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE)
            .edit().putBoolean(KEY_ENABLED,enabled).apply();
        if(enabled)flush(context);
    }

    public static void clear(Context context){
        final Context app=context.getApplicationContext();
        IO.execute(()->{
            File[] files=reportFiles(app);
            for(File file:files)file.delete();
        });
    }

    public static void flush(Context context){
        final Context app=context.getApplicationContext();
        IO.execute(()->uploadPending(app));
    }

    /** Capture only enum statuses and non-sensitive counts. Safe for the UI thread. */
    public static void record(Context context,File source,boolean dxf,CadOpenTrace trace,
                              DxfParser.Result parsed,String result,Throwable failure){
        if(!enabled(context))return;
        final Context app=context.getApplicationContext();
        long inputBytes=source!=null&&source.isFile()?
            Math.max(0L,Math.min(536870912L,source.length())):0L;
        String signature=dxf?"DXF":signature(source);
        int entities=parsed==null?0:Math.max(0,Math.min(10000000,parsed.entityCount));
        int layers=parsed==null?0:Math.max(0,Math.min(1000000,parsed.layerCount));
        int warnings=parsed==null?0:Math.max(0,Math.min(1000000,parsed.conversionWarnings));
        String error="NONE";
        if(failure!=null){
            if(failure instanceof OutOfMemoryError)error="OOM";
            else if(Thread.currentThread().isInterrupted())error="CANCELLED";
            else if(failure instanceof IOException)error="IO";
            else error="OTHER";
        }
        try{
            JSONObject data=new JSONObject();
            data.put("schema","musacad-diagnostics/v1");
            data.put("reportId",UUID.randomUUID().toString());
            data.put("occurredAt",utcNow());
            data.put("appVersion",BuildConfig.VERSION_NAME);
            data.put("format",dxf?"DXF":"DWG");
            data.put("signature",signature);
            data.put("result",result);
            data.put("stage",trace==null?"UNKNOWN":trace.latestStage());
            data.put("elapsedMs",trace==null?0L:Math.min(86400000L,trace.totalMs()));
            data.put("inputBytes",inputBytes);
            data.put("parsedEntities",entities);
            data.put("layerCount",layers);
            data.put("conversionWarnings",warnings);
            data.put("errorCode",error);
            final byte[] bytes=data.toString().getBytes(StandardCharsets.UTF_8);
            if(bytes.length>MAX_JSON_BYTES)return;
            final String name=data.getString("reportId")+".json";
            IO.execute(()->{
                if(!enabled(app))return;
                persist(app,name,bytes);
                uploadPending(app);
            });
        }catch(Exception ignored){
            // Diagnostics must never block drawing open or mutate the DWG.
        }
    }

    private static String utcNow(){
        SimpleDateFormat f=new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'",Locale.ROOT);
        f.setTimeZone(TimeZone.getTimeZone("UTC"));
        return f.format(new Date());
    }

    private static String signature(File source){
        if(source==null||!source.isFile())return "UNKNOWN";
        byte[] head=new byte[6];
        try(FileInputStream in=new FileInputStream(source)){
            int n=0;
            while(n<6){int read=in.read(head,n,6-n);if(read<0)break;n+=read;}
            if(n==6){
                String value=new String(head,StandardCharsets.US_ASCII);
                switch(value){
                    case "AC1009":case "AC1012":case "AC1014":case "AC1015":
                    case "AC1018":case "AC1021":case "AC1024":case "AC1027":
                    case "AC1032":return value;
                    default:break;
                }
            }
        }catch(IOException ignored){}
        return "UNKNOWN";
    }

    private static File directory(Context app){
        return new File(app.getFilesDir(),"dwg_diagnostics_v1");
    }
    private static File[] reportFiles(Context app){
        File[] files=directory(app).listFiles(file->file.isFile()&&
            file.getName().matches("[0-9a-f-]{36}\\.json"));
        if(files==null)return new File[0];
        Arrays.sort(files,Comparator.comparingLong(File::lastModified));
        return files;
    }
    private static void persist(Context app,String name,byte[] bytes){
        File dir=directory(app);
        if(!dir.isDirectory()&&!dir.mkdirs())return;
        File temp=null;
        try{
            temp=File.createTempFile("pending-",".tmp",dir);
            try(FileOutputStream out=new FileOutputStream(temp)){out.write(bytes);out.getFD().sync();}
            File dest=new File(dir,name);
            if(!temp.renameTo(dest))return;
            File[] files=reportFiles(app);
            for(int i=0;i<files.length-MAX_REPORT_FILES;i++)files[i].delete();
        }catch(IOException ignored){}finally{if(temp!=null)temp.delete();}
    }

    private static URL endpoint()throws IOException{
        String configured=BuildConfig.AI_API_URL==null?"":BuildConfig.AI_API_URL.trim();
        if(!configured.startsWith("https://"))throw new IOException("Missing HTTPS AI gateway");
        URL api=new URL(configured);
        if(!"/v1/analyze".equals(api.getPath())||
            api.getUserInfo()!=null||api.getQuery()!=null||api.getRef()!=null)
            throw new IOException("Unexpected AI gateway URL");
        return new URL("https",api.getHost(),api.getPort(),"/v1/diagnostics");
    }

    private static void uploadPending(Context app){
        if(!enabled(app))return;
        File[] files=reportFiles(app);
        if(files.length==0)return;
        final URL url;
        try{url=endpoint();}catch(IOException e){return;}
        MusaAiSessionService.Result session=MusaAiSessionService.get(app);
        if(!session.developer())return;
        for(File file:files){
            if(!enabled(app))return;
            if(file.length()<=0L||file.length()>MAX_JSON_BYTES){file.delete();continue;}
            HttpsURLConnection connection=null;
            try{
                byte[] bytes=new byte[(int)file.length()];
                try(FileInputStream in=new FileInputStream(file)){
                    int offset=0;while(offset<bytes.length){
                        int n=in.read(bytes,offset,bytes.length-offset);
                        if(n<=0)throw new IOException("Incomplete diagnostic file");
                        offset+=n;
                    }
                }
                connection=(HttpsURLConnection)url.openConnection();
                connection.setConnectTimeout(9000);
                connection.setReadTimeout(12000);
                connection.setInstanceFollowRedirects(false);
                connection.setRequestMethod("POST");
                connection.setRequestProperty("Authorization","Bearer "+session.token);
                connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
                connection.setRequestProperty("Accept","application/json");
                connection.setDoOutput(true);
                connection.setFixedLengthStreamingMode(bytes.length);
                try(OutputStream out=connection.getOutputStream()){out.write(bytes);}
                int code=connection.getResponseCode();
                if(code!=200&&code!=201)return;
                String body=readResponse(connection.getInputStream());
                if(!"stored".equals(new JSONObject(body).optString("status","")))return;
                if(!file.delete())return;
            }catch(Exception e){
                // Preserve the queued report and retry on a future app open.
                return;
            }finally{if(connection!=null)connection.disconnect();}
        }
    }

    private static String readResponse(InputStream stream)throws IOException{
        try(InputStream in=stream;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buf=new byte[512];int n;
            while((n=in.read(buf))!=-1){
                if(out.size()+n>4096)throw new IOException("Oversized gateway reply");
                out.write(buf,0,n);
            }
            return out.toString("UTF-8");
        }
    }
}
