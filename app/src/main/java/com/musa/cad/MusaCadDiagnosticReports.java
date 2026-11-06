package com.musa.cad;

import android.content.Context;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Comparator;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONObject;

/**
 * Automatic, local, read-only CAD opening reports. No CAD name, URI, path,
 * text, coordinate, screenshot, or original file bytes ever leave the device.
 * Uploads require a user setting and a separately configured private-repo
 * HTTPS bridge; GitHub credentials must NEVER be embedded in an APK.
 */
public final class MusaCadDiagnosticReports {
    private static final String PREFS = "musacad_diagnostic_reports";
    private static final String CLOUD_ALLOWED = "cloud_upload_opt_in_v1";
    private static final int MAX_QUEUE_FILES = 100;
    private static final ExecutorService WORK = Executors.newSingleThreadExecutor(r->{
        Thread t=new Thread(r,"MusaCAD-SafeDiagnosticReports");
        t.setDaemon(true);
        return t;
    });

    private MusaCadDiagnosticReports(){}

    public static boolean autoUploadEnabled(Context context) {
        return context.getSharedPreferences(PREFS,Context.MODE_PRIVATE)
            .getBoolean(CLOUD_ALLOWED,false);
    }
    public static boolean serverConfigured() {
        String endpoint=BuildConfig.REPORTS_API_URL;
        return endpoint!=null && endpoint.startsWith("https://");
    }
    public static void setAutoUpload(Context context,boolean enabled) {
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE)
            .edit().putBoolean(CLOUD_ALLOWED,enabled).apply();
        if (enabled) flush(context);
    }

    private static File queueDir(Context context) {
        File dir=new File(context.getFilesDir(),"reports-pending");
        if (!dir.isDirectory() && !dir.mkdirs()) return null;
        return dir;
    }
    public static int pendingCount(Context context) {
        File dir=queueDir(context);
        File[] files=dir==null?null:dir.listFiles((d,name)->name.endsWith(".json"));
        return files==null?0:files.length;
    }
    public static void clearPending(Context context) {
        final Context app=context.getApplicationContext();
        WORK.execute(()->{
            File dir=queueDir(app);if(dir==null)return;
            File[] files=dir.listFiles((d,name)->name.endsWith(".json"));
            if(files!=null)for(File file:files)file.delete();
        });
    }

    private static String signature(File source,boolean dxf) {
        if(dxf)return "DXF";
        if(source==null||!source.isFile())return "UNKNOWN";
        byte[] six=new byte[6];
        try(FileInputStream input=new FileInputStream(source)) {
            if(input.read(six)!=6)return "UNKNOWN";
        }catch(IOException ignored){return "UNKNOWN";}
        String v=new String(six,StandardCharsets.US_ASCII);
        switch(v) {
            case "AC1009": case "AC1012": case "AC1014":
            case "AC1015": case "AC1018": case "AC1021":
            case "AC1024": case "AC1027": case "AC1032":return v;
            default:return "UNKNOWN";
        }
    }

    public static void recordOpen(Context context,File file,CadOpenTrace trace,
                                  boolean dxf,String event,int entities,String errorKind) {
        final Context app=context.getApplicationContext();
        final long size=file!=null&&file.isFile()?Math.max(0,file.length()):0;
        final String format=signature(file,dxf);
        final String traceText=trace==null?"":trace.safeSummary(size,entities);
        final String safeEvent=validEvent(event)?event:"open_failure";
        final String safeError=validError(errorKind)?errorKind:"unknown";
        final String version=BuildConfig.VERSION_NAME;
        final long recorded=System.currentTimeMillis();
        WORK.execute(()->{
            try{
                File dir=queueDir(app);if(dir==null)return;
                JSONObject data=new JSONObject();
                data.put("schema","musacad-diagnostic-v1");
                data.put("id",UUID.randomUUID().toString());
                data.put("createdAtMs",recorded);
                data.put("appVersion",version);
                data.put("format",format);
                data.put("event",safeEvent);
                data.put("fileBytes",Math.min(512L*1024L*1024L,size));
                data.put("entityCount",Math.max(0,Math.min(10000000,entities)));
                data.put("trace",traceText.length()>420?traceText.substring(0,420):traceText);
                data.put("errorKind",safeError);
                byte[] bytes=(data.toString()+"\n").getBytes(StandardCharsets.UTF_8);
                String fileName=data.getString("id")+".json";
                File temp=File.createTempFile("report-", ".tmp",dir);
                boolean wrote=false;
                try(FileOutputStream out=new FileOutputStream(temp)){
                    out.write(bytes);out.getFD().sync();wrote=true;
                }finally{
                    if(!wrote)temp.delete();
                }
                File destination=new File(dir,fileName);
                if(!temp.renameTo(destination)){temp.delete();return;}
                trimQueue(dir);
                if(autoUploadEnabled(app)) uploadQueue(app);
            }catch(Exception ignored){
                // Diagnostic collection must never break opening a CAD file.
            }
        });
    }
    private static boolean validEvent(String e){
        return "open_success".equals(e)||"open_failure".equals(e)||
            "open_partial".equals(e)||"open_3d".equals(e);
    }
    private static boolean validError(String e){
        return "none".equals(e)||"io".equals(e)||"memory".equals(e)||
            "runtime".equals(e)||"unknown".equals(e);
    }
    private static File[] queueFiles(File dir){
        File[] files=dir.listFiles((d,name)->name.endsWith(".json"));
        if(files==null)return new File[0];
        Arrays.sort(files,Comparator.comparingLong(File::lastModified));
        return files;
    }
    private static void trimQueue(File dir){
        File[] files=queueFiles(dir);
        for(int i=0;i<files.length-MAX_QUEUE_FILES;i++)files[i].delete();
    }
    public static void flush(Context context){
        final Context app=context.getApplicationContext();
        WORK.execute(()->uploadQueue(app));
    }
    private static void uploadQueue(Context app){
        if(!autoUploadEnabled(app)||!serverConfigured())return;
        File dir=queueDir(app);if(dir==null)return;
        File[] queued=queueFiles(dir);
        if(queued.length==0)return;
        MusaAiSessionService.Result session=MusaAiSessionService.get(app);
        if(!session.active())return;
        for(File file:queued){
            if(!autoUploadEnabled(app))break;
            if(!uploadOne(file,session.token))break;
            file.delete();
        }
    }
    private static boolean uploadOne(File file,String sessionToken){
        HttpsURLConnection connection=null;
        try{
            if(file.length()>8192L)return false;
            URL url=new URL(BuildConfig.REPORTS_API_URL);
            if(!"https".equalsIgnoreCase(url.getProtocol()))return false;
            connection=(HttpsURLConnection)url.openConnection();
            connection.setConnectTimeout(6000);
            connection.setReadTimeout(12000);
            connection.setInstanceFollowRedirects(false);
            connection.setRequestMethod("POST");
            connection.setDoOutput(true);
            connection.setRequestProperty("Authorization","Bearer "+sessionToken);
            connection.setRequestProperty("Content-Type","application/json; charset=utf-8");
            connection.setRequestProperty("Cache-Control","no-store");
            try(InputStream source=new FileInputStream(file);
                OutputStream out=connection.getOutputStream()){
                byte[] chunk=new byte[2048];int n;
                while((n=source.read(chunk))!=-1)out.write(chunk,0,n);
            }
            if(connection.getResponseCode()!=200)return false;
            try(InputStream in=connection.getInputStream()){
                java.io.ByteArrayOutputStream response=new java.io.ByteArrayOutputStream();
                byte[] chunk=new byte[256];int n;
                while((n=in.read(chunk))!=-1){
                    if(response.size()+n>2048)return false;
                    response.write(chunk,0,n);
                }
                if(response.size()==0)return false;
                JSONObject ack=new JSONObject(response.toString("UTF-8"));
                String expected=file.getName().replace(".json","");
                return "stored".equals(ack.optString("status")) &&
                    expected.equals(ack.optString("id"));
            }
        }catch(Exception ignored){return false;}
        finally{if(connection!=null)connection.disconnect();}
    }
}
