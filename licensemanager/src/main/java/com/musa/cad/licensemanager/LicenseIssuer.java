package com.musa.cad.licensemanager;

import android.util.Base64;
import java.nio.charset.StandardCharsets;
import java.security.Signature;

final class LicenseIssuer {
    static final String PREFIX="MC1";

    static String issue(String installationId,int days) throws Exception {
        String id=normalize(installationId);
        long expires=days<=0?0L:System.currentTimeMillis()+days*86_400_000L;
        String payload=PREFIX+"|"+id+"|"+expires;
        byte[] payloadBytes=payload.getBytes(StandardCharsets.UTF_8);

        Signature signer=Signature.getInstance("SHA256withRSA");
        signer.initSign(LicenseKeyStore.privateKey());
        signer.update(payloadBytes);
        byte[] sig=signer.sign();

        return PREFIX+"."+b64(payloadBytes)+"."+b64(sig);
    }

    static String normalize(String value){
        if(value==null)throw new IllegalArgumentException("Cihaz ID boş");
        String id=value.trim();
        if(id.length()<8)throw new IllegalArgumentException("Cihaz ID geçersiz");
        if(id.contains("|")||id.contains("\n")||id.contains("\r"))throw new IllegalArgumentException("Cihaz ID geçersiz");
        return id;
    }

    private static String b64(byte[] value){
        return Base64.encodeToString(value,Base64.URL_SAFE|Base64.NO_WRAP|Base64.NO_PADDING);
    }
    private LicenseIssuer(){}
}
