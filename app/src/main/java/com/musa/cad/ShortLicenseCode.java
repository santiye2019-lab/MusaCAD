package com.musa.cad;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;

/**
 * Compact 12-character MusaCAD license code.
 * Format: 4 characters expiry day + 8 characters device-bound digest.
 * 0000 expiry means perpetual.
 */
public final class ShortLicenseCode {
    private static final String ALPHABET="0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    private static final long DAY_MS=86_400_000L;
    private static final String CONTEXT="MUSACAD-SHORT-LICENSE-V1";

    public static String serialFromInstallationId(String installationId){
        String id=installationId==null?"":installationId.trim().toUpperCase(Locale.ROOT);
        String body=id.startsWith("MC-")?id.substring(3):id;
        StringBuilder clean=new StringBuilder();
        for(int i=0;i<body.length();i++){
            char c=body.charAt(i);
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9'))clean.append(c);
        }
        if(clean.length()>=12)return clean.substring(0,12);
        try{
            byte[] digest=MessageDigest.getInstance("SHA-256").digest(id.getBytes(StandardCharsets.UTF_8));
            StringBuilder hex=new StringBuilder();
            for(int i=0;i<6;i++)hex.append(String.format(Locale.ROOT,"%02X",digest[i]&255));
            return hex.toString();
        }catch(Exception e){
            String fallback=clean.toString()+"000000000000";
            return fallback.substring(0,12);
        }
    }

    public static String normalizeSerial(String serial){
        String s=serial==null?"":serial.trim().toUpperCase(Locale.ROOT);
        StringBuilder clean=new StringBuilder();
        for(int i=0;i<s.length();i++){
            char c=s.charAt(i);
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9'))clean.append(c);
        }
        if(clean.length()!=12)throw new IllegalArgumentException("Serial 12 karakter olmalıdır");
        return clean.toString();
    }

    public static String normalizeCode(String code){
        String s=code==null?"":code.trim().toUpperCase(Locale.ROOT);
        StringBuilder clean=new StringBuilder();
        for(int i=0;i<s.length();i++){
            char c=s.charAt(i);
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9'))clean.append(c);
        }
        return clean.toString();
    }

    public static boolean looksLikeShortCode(String code){
        return normalizeCode(code).length()==12;
    }

    public static String issue(String serial,int days,long nowMs)throws Exception{
        String s=normalizeSerial(serial);
        if(days<0||days>3650)throw new IllegalArgumentException("Lisans süresi geçersiz");
        long expiryDay=days==0?0L:(nowMs/DAY_MS)+days;
        if(expiryDay>0xFFFFFL)throw new IllegalArgumentException("Lisans tarihi desteklenmiyor");
        String expiry=encode(expiryDay,4);
        return expiry+digest8(s,expiry);
    }

    public static boolean verify(String code,String serial,long nowMs){
        try{
            String normalized=normalizeCode(code);
            if(normalized.length()!=12)return false;
            String s=normalizeSerial(serial);
            String expiry=normalized.substring(0,4);
            long expiryDay=decode(expiry);
            if(expiryDay!=0L&&nowMs/DAY_MS>expiryDay)return false;
            String expected=expiry+digest8(s,expiry);
            return constantEquals(normalized,expected);
        }catch(Exception e){return false;}
    }

    public static long expiryAtMs(String code){
        try{
            String normalized=normalizeCode(code);
            if(normalized.length()!=12)return 0L;
            long day=decode(normalized.substring(0,4));
            return day==0L?0L:(day+1L)*DAY_MS-1L;
        }catch(Exception e){return 0L;}
    }

    private static String digest8(String serial,String expiry)throws Exception{
        byte[] digest=MessageDigest.getInstance("SHA-256")
            .digest((CONTEXT+"|"+serial+"|"+expiry).getBytes(StandardCharsets.UTF_8));
        long value=0L;
        for(int i=0;i<5;i++)value=(value<<8)|(digest[i]&255L);
        return encode(value,8);
    }

    private static String encode(long value,int chars){
        char[] out=new char[chars];
        for(int i=chars-1;i>=0;i--){out[i]=ALPHABET.charAt((int)(value&31L));value>>>=5;}
        return new String(out);
    }

    private static long decode(String value){
        long out=0L;
        for(int i=0;i<value.length();i++){
            int v=ALPHABET.indexOf(value.charAt(i));
            if(v<0)throw new IllegalArgumentException("Geçersiz lisans karakteri");
            out=(out<<5)|v;
        }
        return out;
    }

    private static boolean constantEquals(String a,String b){
        byte[] x=a.getBytes(StandardCharsets.US_ASCII),y=b.getBytes(StandardCharsets.US_ASCII);
        int diff=x.length^y.length,max=Math.max(x.length,y.length);
        for(int i=0;i<max;i++)diff|=(i<x.length?x[i]:0)^(i<y.length?y[i]:0);
        return diff==0;
    }

    private ShortLicenseCode(){}
}
