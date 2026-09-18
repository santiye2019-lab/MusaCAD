package com.musa.cad;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Base64;
import android.widget.ImageView;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class EmbeddedProfilePhoto {
    public static void loadInto(Context context, ImageView target){
        if(context==null||target==null)return;
        try(InputStream in=context.getAssets().open("musa_profile.b64");
            ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buffer=new byte[4096];int n;
            while((n=in.read(buffer))!=-1)out.write(buffer,0,n);
            String encoded=new String(out.toByteArray(),StandardCharsets.US_ASCII).trim();
            byte[] data=Base64.decode(encoded,Base64.DEFAULT);
            Bitmap bitmap=BitmapFactory.decodeByteArray(data,0,data.length);
            if(bitmap!=null)target.setImageBitmap(bitmap);
        }catch(Exception ignored){}
    }
    private EmbeddedProfilePhoto(){}
}
