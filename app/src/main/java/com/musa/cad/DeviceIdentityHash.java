package com.musa.cad;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;

/** Pure-Java derivation for a privacy-preserving, reinstall-stable MusaCAD license identifier. */
public final class DeviceIdentityHash {
    private static final String DOMAIN="MUSACAD-LICENSE-ID-V2";

    public static String derive(String androidId,String packageName){
        String raw=androidId==null?"":androidId.trim().toLowerCase(Locale.ROOT);
        String pkg=packageName==null?"":packageName.trim();
        if(raw.isEmpty()||pkg.isEmpty()||"9774d56d682e549c".equals(raw))return "";
        try{
            MessageDigest digest=MessageDigest.getInstance("SHA-256");
            byte[] hash=digest.digest((DOMAIN+"|"+pkg+"|"+raw).getBytes(StandardCharsets.UTF_8));
            StringBuilder hex=new StringBuilder(32);
            for(int i=0;i<12;i++)hex.append(String.format(Locale.ROOT,"%02X",hash[i]&0xFF));
            return "MC-"+hex.substring(0,8)+"-"+hex.substring(8,16)+"-"+hex.substring(16,24);
        }catch(Exception e){return "";}
    }

    public static boolean isValidPublicId(String value){
        return value!=null&&value.matches("MC-[0-9A-F]{8}-[0-9A-F]{8}-[0-9A-F]{8}");
    }

    private DeviceIdentityHash(){}
}
