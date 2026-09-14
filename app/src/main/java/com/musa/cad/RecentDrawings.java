package com.musa.cad;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import org.json.*;
import java.io.*;
import java.util.*;

/** Device-local recent drawing references. Original drawings are never moved. */
final class RecentDrawings {
    static final class Entry {
        String uri,name,thumbnail;long time;boolean favorite;
    }
    static List<Entry> read(Context c){
        List<Entry> list=new ArrayList<>();
        try{JSONArray a=new JSONArray(c.getSharedPreferences("drawings",0).getString("recent","[]"));
            for(int i=0;i<a.length();i++){JSONObject o=a.getJSONObject(i);Entry e=new Entry();e.uri=o.getString("uri");e.name=o.getString("name");e.time=o.optLong("time");e.favorite=o.optBoolean("favorite");e.thumbnail=o.optString("thumbnail");list.add(e);}
        }catch(JSONException ignored){}
        return list;
    }
    private static void write(Context c,List<Entry> list){
        JSONArray a=new JSONArray();
        try{for(Entry e:list){JSONObject o=new JSONObject();o.put("uri",e.uri);o.put("name",e.name);o.put("time",e.time);o.put("favorite",e.favorite);o.put("thumbnail",e.thumbnail);a.put(o);}}
        catch(JSONException impossible){throw new IllegalStateException(impossible);}
        c.getSharedPreferences("drawings",0).edit().putString("recent",a.toString()).apply();
    }
    static void remember(Context c,Uri uri,String name,Bitmap drawing){
        List<Entry> list=read(c);Entry entry=null;
        for(Iterator<Entry> it=list.iterator();it.hasNext();){Entry e=it.next();if(e.uri.equals(uri.toString())){entry=e;it.remove();break;}}
        if(entry==null){entry=new Entry();entry.uri=uri.toString();entry.thumbnail="recent-"+UUID.randomUUID()+".png";}
        entry.name=name;entry.time=System.currentTimeMillis();
        Bitmap small=Bitmap.createScaledBitmap(drawing,160,160,true);
        try(OutputStream out=new FileOutputStream(new File(c.getFilesDir(),entry.thumbnail))){small.compress(Bitmap.CompressFormat.PNG,100,out);}catch(IOException ignored){}finally{if(small!=drawing)small.recycle();}
        list.add(0,entry);
        while(list.size()>40){Entry removed=list.remove(list.size()-1);new File(c.getFilesDir(),removed.thumbnail).delete();}
        write(c,list);
    }
    static void toggleFavorite(Context c,String uri){List<Entry> list=read(c);for(Entry e:list)if(e.uri.equals(uri))e.favorite=!e.favorite;write(c,list);}
    private RecentDrawings(){}
}
