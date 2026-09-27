package com.musa.cad;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import org.json.*;

/** Persists raster overlay images and their CAD placement outside the DXF payload. */
public final class CadImageOverlayStore {
    private static final String ROOT="cad_image_overlays";
    private static final String MANIFEST="manifest.json";

    private CadImageOverlayStore(){}

    public static void save(Context context,String projectKey,List<CadImageOverlay> overlays)throws IOException{
        if(context==null||projectKey==null||projectKey.trim().isEmpty())return;
        File dir=projectDir(context,projectKey);
        if(overlays==null||overlays.isEmpty()){deleteTree(dir);return;}
        if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Görüntü kayıt klasörü oluşturulamadı");

        JSONArray items=new JSONArray();HashSet<String> keep=new HashSet<>();
        for(CadImageOverlay image:overlays){
            if(image==null||image.bitmap==null||image.bitmap.isRecycled())continue;
            String fileName=hash(image.uri+"|"+image.bitmap.getWidth()+"x"+image.bitmap.getHeight())+".png";
            File imageFile=new File(dir,fileName);keep.add(fileName);
            if(!imageFile.isFile()||imageFile.length()==0){
                File temp=new File(dir,fileName+".tmp");
                try(OutputStream out=new BufferedOutputStream(new FileOutputStream(temp))){
                    if(!image.bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("Görüntü kaydedilemedi");
                }
                replace(temp,imageFile);
            }
            JSONObject item=new JSONObject();
            try{
                item.put("file",fileName);item.put("name",image.name);item.put("uri",image.uri);
                item.put("cx",image.centerX());item.put("cy",image.centerY());item.put("w",image.width());item.put("h",image.height());item.put("rot",image.rotationDegrees());
            }catch(JSONException e){throw new IOException("Görüntü bilgisi yazılamadı",e);}
            items.put(item);
        }
        JSONObject root=new JSONObject();
        try{root.put("version",1);root.put("items",items);}catch(JSONException e){throw new IOException("Görüntü manifesti oluşturulamadı",e);}
        File tempManifest=new File(dir,MANIFEST+".tmp");
        try(Writer out=new OutputStreamWriter(new FileOutputStream(tempManifest),StandardCharsets.UTF_8)){out.write(root.toString());}
        replace(tempManifest,new File(dir,MANIFEST));

        File[] stale=dir.listFiles((d,n)->n.endsWith(".png")&&!keep.contains(n));
        if(stale!=null)for(File file:stale)file.delete();
    }

    public static List<CadImageOverlay> load(Context context,String projectKey){
        ArrayList<CadImageOverlay> out=new ArrayList<>();
        if(context==null||projectKey==null||projectKey.trim().isEmpty())return out;
        File dir=projectDir(context,projectKey),manifest=new File(dir,MANIFEST);if(!manifest.isFile())return out;
        try{
            String json=readAll(manifest);JSONObject root=new JSONObject(json);JSONArray items=root.optJSONArray("items");if(items==null)return out;
            for(int i=0;i<items.length();i++){
                JSONObject item=items.optJSONObject(i);if(item==null)continue;String file=item.optString("file","");if(file.isEmpty())continue;
                Bitmap bitmap=BitmapFactory.decodeFile(new File(dir,file).getAbsolutePath());if(bitmap==null)continue;
                float cx=(float)item.optDouble("cx",Float.NaN),cy=(float)item.optDouble("cy",Float.NaN),w=(float)item.optDouble("w",Float.NaN),h=(float)item.optDouble("h",Float.NaN),rot=(float)item.optDouble("rot",0d);
                if(!Float.isFinite(cx)||!Float.isFinite(cy)||!Float.isFinite(w)||!Float.isFinite(h)||w<=0f||h<=0f){bitmap.recycle();continue;}
                out.add(new CadImageOverlay(bitmap,item.optString("name","Görüntü"),item.optString("uri",""),cx,cy,w,h,rot));
            }
        }catch(Exception ignored){for(CadImageOverlay image:out)if(image.bitmap!=null&&!image.bitmap.isRecycled())image.bitmap.recycle();out.clear();}
        return out;
    }

    private static File projectDir(Context context,String key){return new File(new File(context.getFilesDir(),ROOT),hash(key));}
    private static String readAll(File file)throws IOException{
        StringBuilder out=new StringBuilder();char[] buf=new char[8192];try(Reader in=new InputStreamReader(new FileInputStream(file),StandardCharsets.UTF_8)){int n;while((n=in.read(buf))!=-1)out.append(buf,0,n);}return out.toString();
    }
    private static void replace(File from,File to)throws IOException{
        if(to.exists()&&!to.delete())throw new IOException("Eski görüntü kaydı silinemedi");
        if(!from.renameTo(to)){try(InputStream in=new FileInputStream(from);OutputStream out=new FileOutputStream(to)){byte[] buf=new byte[64*1024];int n;while((n=in.read(buf))!=-1)out.write(buf,0,n);}if(!from.delete())from.deleteOnExit();}
    }
    private static void deleteTree(File file){
        if(file==null||!file.exists())return;File[] children=file.listFiles();if(children!=null)for(File child:children)deleteTree(child);file.delete();
    }
    private static String hash(String value){
        try{MessageDigest md=MessageDigest.getInstance("SHA-256");byte[] bytes=md.digest(value.getBytes(StandardCharsets.UTF_8));StringBuilder out=new StringBuilder(64);for(byte b:bytes)out.append(String.format(Locale.US,"%02x",b&0xff));return out.toString();}
        catch(Exception e){return Integer.toHexString(value.hashCode());}
    }
}
