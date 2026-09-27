package com.musa.cad;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.LruCache;
import java.io.*;
import java.util.Locale;
import java.util.UUID;

/** Internal persistent store for user-inserted JPG/JPEG/PNG/WebP images. */
public final class CadImageStore {
    private static final String DIR="cad_images";
    private static final long MAX_BYTES=32L*1024L*1024L;
    private static final int MAX_DECODE=2048;
    private static final LruCache<String,Bitmap> CACHE=new LruCache<String,Bitmap>(16*1024){
        @Override protected int sizeOf(String key,Bitmap bitmap){return Math.max(1,bitmap.getByteCount()/1024);}
    };

    public static final class Imported {
        public final String key,name;
        public final int pixelWidth,pixelHeight;
        Imported(String key,String name,int pixelWidth,int pixelHeight){
            this.key=key;this.name=name;this.pixelWidth=pixelWidth;this.pixelHeight=pixelHeight;
        }
    }

    public static Imported importImage(Context context,Uri uri)throws IOException{
        if(context==null||uri==null)throw new IOException("Görüntü seçilmedi");
        String name=queryName(context,uri);String mime=context.getContentResolver().getType(uri);
        String ext=extension(name,mime);
        if(ext==null)throw new IOException("Yalnız JPG, JPEG, PNG veya WebP görüntüsü seçin");
        File dir=dir(context);if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Görüntü klasörü oluşturulamadı");
        String key=UUID.randomUUID().toString().replace("-","")+"."+ext;
        File target=new File(dir,key);
        try(InputStream in=context.getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(target)){
            if(in==null)throw new IOException("Görüntü açılamadı");
            byte[] buffer=new byte[64*1024];int n;long total=0L;
            while((n=in.read(buffer))!=-1){total+=n;if(total>MAX_BYTES)throw new IOException("Görüntü 32 MB sınırını aşıyor");out.write(buffer,0,n);}
        }catch(IOException e){target.delete();throw e;}
        BitmapFactory.Options bounds=new BitmapFactory.Options();bounds.inJustDecodeBounds=true;BitmapFactory.decodeFile(target.getAbsolutePath(),bounds);
        if(bounds.outWidth<=0||bounds.outHeight<=0){target.delete();throw new IOException("Geçerli bir görüntü dosyası değil");}
        Bitmap preview=decode(target);
        if(preview==null){target.delete();throw new IOException("Görüntü çözülemedi");}
        CACHE.put(key,preview);
        return new Imported(key,name,bounds.outWidth,bounds.outHeight);
    }

    public static Bitmap load(Context context,String key){
        if(context==null||key==null||key.trim().isEmpty())return null;
        String safe=safeKey(key);if(safe.isEmpty())return null;
        Bitmap cached=CACHE.get(safe);if(cached!=null&&!cached.isRecycled())return cached;
        File file=new File(dir(context),safe);if(!file.isFile())return null;
        Bitmap bitmap=decode(file);if(bitmap!=null)CACHE.put(safe,bitmap);return bitmap;
    }

    public static boolean exists(Context context,String key){
        if(context==null)return false;String safe=safeKey(key);return !safe.isEmpty()&&new File(dir(context),safe).isFile();
    }

    private static Bitmap decode(File file){
        BitmapFactory.Options bounds=new BitmapFactory.Options();bounds.inJustDecodeBounds=true;BitmapFactory.decodeFile(file.getAbsolutePath(),bounds);
        if(bounds.outWidth<=0||bounds.outHeight<=0)return null;
        int sample=1;while(bounds.outWidth/sample>MAX_DECODE||bounds.outHeight/sample>MAX_DECODE)sample*=2;
        BitmapFactory.Options options=new BitmapFactory.Options();options.inSampleSize=sample;options.inPreferredConfig=Bitmap.Config.ARGB_8888;
        try{return BitmapFactory.decodeFile(file.getAbsolutePath(),options);}catch(OutOfMemoryError e){return null;}
    }

    private static File dir(Context c){return new File(c.getFilesDir(),DIR);}
    private static String safeKey(String key){
        String base=new File(key).getName();return base.matches("[A-Za-z0-9._-]+")?base:"";
    }
    private static String extension(String name,String mime){
        String lower=name==null?"":name.toLowerCase(Locale.ROOT);
        if(lower.endsWith(".jpg")||lower.endsWith(".jpeg"))return "jpg";
        if(lower.endsWith(".png"))return "png";
        if(lower.endsWith(".webp"))return "webp";
        String m=mime==null?"":mime.toLowerCase(Locale.ROOT);
        if(m.equals("image/jpeg"))return "jpg";
        if(m.equals("image/png"))return "png";
        if(m.equals("image/webp"))return "webp";
        return null;
    }
    private static String queryName(Context c,Uri uri){
        try(android.database.Cursor cursor=c.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){
            if(cursor!=null&&cursor.moveToFirst()){int i=cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);if(i>=0){String v=cursor.getString(i);if(v!=null&&!v.trim().isEmpty())return v.trim();}}
        }catch(Exception ignored){}
        String last=uri.getLastPathSegment();return last==null?"gorsel":last;
    }
    private CadImageStore(){}
}
