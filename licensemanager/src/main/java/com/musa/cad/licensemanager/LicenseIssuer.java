package com.musa.cad.licensemanager;

import android.content.Context;
import java.nio.charset.StandardCharsets;
import java.security.Signature;

final class LicenseIssuer {
    static final String PREFIX="MC1";
    private static final char[] B64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".toCharArray();

    static String issue(Context context,String installationId,int days)throws Exception{
        String id=normalize(installationId);
        if(days<0||days>3650)throw new IllegalArgumentException("Lisans süresi geçersiz");
        long expires=days<=0?0L:System.currentTimeMillis()+days*86_400_000L;
        String payload=PREFIX+"|"+id+"|"+expires;
        byte[] payloadBytes=payload.getBytes(StandardCharsets.UTF_8);
        Signature signer=Signature.getInstance("SHA256withRSA");
        signer.initSign(LicenseKeyStore.privateKey(context));signer.update(payloadBytes);
        return PREFIX+"."+b64(payloadBytes)+"."+b64(signer.sign());
    }

    static String normalize(String value){
        if(value==null)throw new IllegalArgumentException("Güvenli Lisans Kimliği boş");
        String id=value.trim();
        if(id.length()<8||id.length()>200||id.contains("|")||id.contains("\n")||id.contains("\r"))
            throw new IllegalArgumentException("Güvenli Lisans Kimliği geçersiz");
        return id;
    }

    static boolean isToken(String value){return value!=null&&value.trim().startsWith(PREFIX+".");}

    private static String b64(byte[] data){
        StringBuilder out=new StringBuilder((data.length*4+2)/3);
        for(int i=0;i<data.length;i+=3){
            int a=data[i]&255,b=i+1<data.length?data[i+1]&255:0,c=i+2<data.length?data[i+2]&255:0;
            out.append(B64[a>>>2]).append(B64[((a&3)<<4)|(b>>>4)]);
            if(i+1<data.length)out.append(B64[((b&15)<<2)|(c>>>6)]);
            if(i+2<data.length)out.append(B64[c&63]);
        }
        return out.toString();
    }
    private LicenseIssuer(){}
}
