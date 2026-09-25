package com.musa.cad.licensemanager;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Locale;

/** Same 12-character MusaCAD license algorithm used by the main app. */
final class ShortLicenseCode {
    private static final String ALPHABET="0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    private static final long DAY_MS=86_400_000L;
    private static final String CONTEXT="MUSACAD-SHORT-LICENSE-V1";

    static String normalizeSerial(String serial){
        String s=serial==null?"":serial.trim().toUpperCase(Locale.ROOT);
        StringBuilder clean=new StringBuilder();
        for(int i=0;i<s.length();i++){
            char c=s.charAt(i);
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9'))clean.append(c);
        }
        if(clean.length()!=12)throw new IllegalArgumentException("Serial 12 karakter olmalıdır");
        return clean.toString();
    }

    static String issue(String serial,int days,long nowMs)throws Exception{
        String s=normalizeSerial(serial);
        if(days<0||days>3650)throw new IllegalArgumentException("Lisans süresi geçersiz");
        long expiryDay=days==0?0L:(nowMs/DAY_MS)+days;
        if(expiryDay>0xFFFFFL)throw new IllegalArgumentException("Lisans tarihi desteklenmiyor");
        String expiry=encode(expiryDay,4);
        return expiry+digest8(s,expiry);
    }

    static boolean isCode(String code){
        if(code==null)return false;
        String s=code.trim().replace(" ","").replace("-","").toUpperCase(Locale.ROOT);
        return s.length()==12;
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

    private ShortLicenseCode(){}
}
