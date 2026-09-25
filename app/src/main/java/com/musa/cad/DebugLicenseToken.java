package com.musa.cad;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;

/** Debug-only test license format. Release builds never accept these tokens. */
public final class DebugLicenseToken {
    public static final String PREFIX="MCT1";
    public static final String CONTEXT="MusaCAD-Debug-License-TEST-ONLY-v1";
    private static final char[] URL64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".toCharArray();

    public static String issue(String installationId,long expiresAtMs)throws Exception{
        if(installationId==null||installationId.trim().isEmpty())throw new IllegalArgumentException("installationId");
        if(expiresAtMs<0L)throw new IllegalArgumentException("expiresAtMs");
        String payload=PREFIX+"|"+installationId.trim()+"|"+expiresAtMs;
        byte[] sig=digest((payload+"|"+CONTEXT).getBytes(StandardCharsets.UTF_8));
        return PREFIX+"."+encode64(payload.getBytes(StandardCharsets.UTF_8))+"."+encode64(sig);
    }

    public static boolean verify(String token,String expectedInstallationId,long nowMs){
        try{
            if(token==null||expectedInstallationId==null)return false;
            String compact=token.trim().replace("\n","").replace("\r","").replace(" ","");
            String[] parts=compact.split("\\.");
            if(parts.length!=3||!PREFIX.equals(parts[0]))return false;
            byte[] payloadBytes=decode64(parts[1]);
            byte[] sig=decode64(parts[2]);
            String payload=new String(payloadBytes,StandardCharsets.UTF_8);
            String[] fields=payload.split("\\|",-1);
            if(fields.length!=3||!PREFIX.equals(fields[0]))return false;
            if(!constantEquals(fields[1],expectedInstallationId))return false;
            long expiry=Long.parseLong(fields[2]);
            if(expiry<0L||(expiry!=0L&&nowMs>expiry))return false;
            byte[] expected=digest((payload+"|"+CONTEXT).getBytes(StandardCharsets.UTF_8));
            return constantEquals(sig,expected);
        }catch(Exception e){return false;}
    }

    private static byte[] digest(byte[] data)throws Exception{return MessageDigest.getInstance("SHA-256").digest(data);}
    private static String encode64(byte[] data){
        StringBuilder out=new StringBuilder((data.length*4+2)/3);
        for(int i=0;i<data.length;i+=3){
            int b0=data[i]&255,b1=i+1<data.length?data[i+1]&255:0,b2=i+2<data.length?data[i+2]&255:0;
            out.append(URL64[b0>>>2]).append(URL64[((b0&3)<<4)|(b1>>>4)]);
            if(i+1<data.length)out.append(URL64[((b1&15)<<2)|(b2>>>6)]);
            if(i+2<data.length)out.append(URL64[b2&63]);
        }
        return out.toString();
    }
    private static byte[] decode64(String s)throws Exception{
        String clean=s.replace("=","").replaceAll("\\s","");
        int mod=clean.length()&3;if(mod==1)throw new IllegalArgumentException("base64");
        ByteArrayOutputStream out=new ByteArrayOutputStream(clean.length()*3/4);int acc=0,bits=0;
        for(int i=0;i<clean.length();i++){int v=value64(clean.charAt(i));if(v<0)throw new IllegalArgumentException("base64");acc=(acc<<6)|v;bits+=6;if(bits>=8){bits-=8;out.write((acc>>>bits)&255);}}
        if(bits>0&&(acc&((1<<bits)-1))!=0)throw new IllegalArgumentException("base64");
        return out.toByteArray();
    }
    private static int value64(char c){
        if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='-')return 62;if(c=='_')return 63;return -1;
    }
    private static boolean constantEquals(String a,String b){return constantEquals(a.getBytes(StandardCharsets.UTF_8),b.getBytes(StandardCharsets.UTF_8));}
    private static boolean constantEquals(byte[] a,byte[] b){int diff=a.length^b.length,max=Math.max(a.length,b.length);for(int i=0;i<max;i++)diff|=(i<a.length?a[i]:0)^(i<b.length?b[i]:0);return diff==0;}
    private DebugLicenseToken(){}
}
