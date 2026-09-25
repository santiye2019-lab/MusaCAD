package com.musa.cad.licensemanager;

import android.content.Context;
import org.json.*;
import java.text.SimpleDateFormat;
import java.util.*;

final class LicenseHistory {
    private static final String PREFS="license_history",KEY="items",SEP=" • ";

    static void add(Context c,String customer,String device,String duration,String token){
        try{
            JSONArray old=new JSONArray(c.getSharedPreferences(PREFS,0).getString(KEY,"[]"));
            JSONArray next=new JSONArray();
            JSONObject n=new JSONObject();
            n.put("customer",customer==null?"":customer.trim());
            n.put("device",device);n.put("duration",duration);n.put("token",token);n.put("time",System.currentTimeMillis());
            next.put(n);
            for(int i=0;i<old.length()&&i<49;i++)next.put(old.getJSONObject(i));
            c.getSharedPreferences(PREFS,0).edit().putString(KEY,next.toString()).apply();
        }catch(Exception ignored){}
    }

    static List<String> labels(Context c){
        ArrayList<String> out=new ArrayList<>();
        try{
            JSONArray a=new JSONArray(c.getSharedPreferences(PREFS,0).getString(KEY,"[]"));
            SimpleDateFormat f=new SimpleDateFormat("dd.MM HH:mm",Locale.getDefault());
            for(int i=0;i<a.length();i++){
                JSONObject o=a.getJSONObject(i);
                String who=o.optString("customer","").trim();if(who.isEmpty())who=o.optString("device","");
                out.add(who+SEP+o.optString("duration","")+SEP+f.format(new Date(o.optLong("time"))));
            }
        }catch(Exception ignored){}
        return out;
    }

    private LicenseHistory(){}
}
