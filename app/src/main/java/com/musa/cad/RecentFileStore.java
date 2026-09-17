package com.musa.cad;

import android.content.*;
import android.graphics.*;
import android.net.Uri;
import org.json.*;
import java.io.*;
import java.security.MessageDigest;
import java.text.SimpleDateFormat;
import java.util.*;

/** Persistent MRU list and thumbnails for drawings successfully opened by MusaCAD. */
public final class RecentFileStore {
    private static final String PREFS="musacad_recent_files";
    private static final String KEY="recent_v1";
    private static final int MAX=20;
    private static final int THUMB_W=360,THUMB_H=240;

    public static final class Entry {
        public final String uri,name,thumbnail;
        public final long lastAccessMs;
        Entry(String uri,String name,long lastAccessMs,String thumbnail){
            this.uri=uri==null?"":uri;
            this.name=name==null||name.trim().isEmpty()?"Çizim":name.trim();
            this.lastAccessMs=lastAccessMs;
            this.thumbnail=thumbnail==null?"":thumbnail;
        }
        public boolean isDxf(){return name.toLowerCase(Locale.ROOT).endsWith(".dxf");}
        public String typeLabel(){return isDxf()?"DXF":"DWG";}
    }

    public static synchronized List<Entry> list(Context context){
        ArrayList<Entry> out=new ArrayList<>();
        if(context==null)return out;
        String raw=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY,"[]");
        try{
            JSONArray a=new JSONArray(raw);
            for(int i=0;i<a.length();i++){
                JSONObject o=a.optJSONObject(i);if(o==null)continue;
                String uri=o.optString("uri","").trim();if(uri.isEmpty())continue;
                out.add(new Entry(uri,o.optString("name","Çizim"),o.optLong("last",0L),o.optString("thumb","")));
            }
        }catch(Exception ignored){}
        Collections.sort(out,(a,b)->Long.compare(b.lastAccessMs,a.lastAccessMs));
        if(out.size()>MAX)return new ArrayList<>(out.subList(0,MAX));
        return out;
    }

    public static synchronized void record(Context context,Uri uri,String name,Bitmap preview){
        if(context==null||uri==null)return;
        String uriText=uri.toString(),thumb="";
        List<Entry> old=list(context);
        for(Entry e:old)if(uriText.equals(e.uri)){thumb=e.thumbnail;break;}
        if(preview!=null&&!preview.isRecycled()){
            try{thumb=saveThumbnail(context,uriText,preview);}catch(Exception ignored){}
        }
        ArrayList<Entry> next=new ArrayList<>();
        next.add(new Entry(uriText,name,System.currentTimeMillis(),thumb));
        for(Entry e:old)if(!uriText.equals(e.uri)&&next.size()<MAX)next.add(e);
        save(context,next);
        cleanupThumbnails(context,next);
    }

    public static synchronized void remove(Context context,String uri){
        if(context==null||uri==null)return;
        ArrayList<Entry> next=new ArrayList<>();String removedThumb="";
        for(Entry e:list(context)){
            if(uri.equals(e.uri))removedThumb=e.thumbnail;else next.add(e);
        }
        save(context,next);
        if(!removedThumb.isEmpty()){File f=new File(thumbnailDir(context),removedThumb);if(f.isFile())f.delete();}
    }

    public static Bitmap thumbnail(Context context,Entry entry){
        if(context==null||entry==null||entry.thumbnail.isEmpty())return null;
        File f=new File(thumbnailDir(context),entry.thumbnail);
        if(!f.isFile())return null;
        try{return BitmapFactory.decodeFile(f.getAbsolutePath());}catch(Exception e){return null;}
    }

    public static String accessLabel(long when){
        if(when<=0)return "Daha önce açıldı";
        long delta=Math.max(0L,System.currentTimeMillis()-when);
        if(delta<60_000L)return "Az önce";
        if(delta<60L*60_000L)return (delta/60_000L)+" dk önce";
        if(delta<24L*60L*60_000L)return (delta/(60L*60_000L))+" sa önce";
        return new SimpleDateFormat("dd.MM.yyyy HH:mm",Locale.getDefault()).format(new Date(when));
    }

    private static void save(Context context,List<Entry> entries){
        JSONArray a=new JSONArray();
        for(Entry e:entries){
            try{JSONObject o=new JSONObject();o.put("uri",e.uri);o.put("name",e.name);o.put("last",e.lastAccessMs);o.put("thumb",e.thumbnail);a.put(o);}catch(Exception ignored){}
        }
        context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().putString(KEY,a.toString()).commit();
    }

    private static String saveThumbnail(Context context,String uri,Bitmap source)throws Exception{
        File dir=thumbnailDir(context);if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("thumb dir");
        String fileName=hash(uri)+".jpg";File outFile=new File(dir,fileName);
        int sw=Math.max(1,source.getWidth()),sh=Math.max(1,source.getHeight());
        float scale=Math.min((float)THUMB_W/sw,(float)THUMB_H/sh);
        int w=Math.max(1,Math.round(sw*scale)),h=Math.max(1,Math.round(sh*scale));
        Bitmap canvasBitmap=Bitmap.createBitmap(THUMB_W,THUMB_H,Bitmap.Config.RGB_565);
        Canvas c=new Canvas(canvasBitmap);c.drawColor(Color.rgb(18,25,31));
        Paint p=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
        Rect src=new Rect(0,0,sw,sh);int left=(THUMB_W-w)/2,top=(THUMB_H-h)/2;
        c.drawBitmap(source,src,new Rect(left,top,left+w,top+h),p);
        try(OutputStream out=new BufferedOutputStream(new FileOutputStream(outFile))){
            if(!canvasBitmap.compress(Bitmap.CompressFormat.JPEG,82,out))throw new IOException("thumb encode");
        }finally{canvasBitmap.recycle();}
        return fileName;
    }

    private static File thumbnailDir(Context context){return new File(context.getFilesDir(),"recent-thumbnails");}
    private static String hash(String value)throws Exception{
        byte[] b=MessageDigest.getInstance("SHA-256").digest(value.getBytes("UTF-8"));
        StringBuilder s=new StringBuilder(24);for(int i=0;i<12;i++)s.append(String.format(Locale.ROOT,"%02x",b[i]&255));return s.toString();
    }
    private static void cleanupThumbnails(Context context,List<Entry> entries){
        HashSet<String> keep=new HashSet<>();for(Entry e:entries)if(!e.thumbnail.isEmpty())keep.add(e.thumbnail);
        File[] files=thumbnailDir(context).listFiles();if(files==null)return;
        for(File f:files)if(f.isFile()&&!keep.contains(f.getName()))f.delete();
    }
    private RecentFileStore(){}
}
