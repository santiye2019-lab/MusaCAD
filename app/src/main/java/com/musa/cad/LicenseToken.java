package com.musa.cad;

import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.X509EncodedKeySpec;
import java.util.Base64;

/** Pure-Java verifier for offline, installation-bound MusaCAD license tokens. */
public final class LicenseToken {
    public static final String PREFIX="MC1";

    public static final class Result {
        public final boolean valid;
        public final String installationId;
        public final long expiresAtMs;
        public final String error;
        Result(boolean valid,String installationId,long expiresAtMs,String error){
            this.valid=valid;this.installationId=installationId;this.expiresAtMs=expiresAtMs;this.error=error;
        }
        public boolean perpetual(){return valid&&expiresAtMs==0L;}
    }

    public static Result verify(String token,String expectedInstallationId,long nowMs,String publicKeyPem){
        try{
            if(token==null)return fail("empty");
            String compact=token.trim().replace("\n","").replace("\r","").replace(" ","");
            String[] parts=compact.split("\\.");
            if(parts.length!=3||!PREFIX.equals(parts[0]))return fail("format");
            Base64.Decoder dec=Base64.getUrlDecoder();
            byte[] payloadBytes=dec.decode(parts[1]);
            byte[] signatureBytes=dec.decode(parts[2]);
            String payload=new String(payloadBytes,StandardCharsets.UTF_8);
            String[] fields=payload.split("\\|",-1);
            if(fields.length!=3||!PREFIX.equals(fields[0]))return fail("payload");
            String installationId=fields[1];
            long expiresAt=Long.parseLong(fields[2]);
            if(expectedInstallationId==null||!constantEquals(installationId,expectedInstallationId))return fail("device");
            if(expiresAt<0L)return fail("expiry");
            if(expiresAt!=0L&&nowMs>expiresAt)return fail("expired");

            Signature verifier=Signature.getInstance("SHA256withRSA");
            verifier.initVerify(parsePublicKey(publicKeyPem));
            verifier.update(payloadBytes);
            if(!verifier.verify(signatureBytes))return fail("signature");
            return new Result(true,installationId,expiresAt,null);
        }catch(Exception e){return fail("invalid");}
    }

    public static String payload(String installationId,long expiresAtMs){
        if(installationId==null||installationId.trim().isEmpty())throw new IllegalArgumentException("installationId");
        if(expiresAtMs<0L)throw new IllegalArgumentException("expiresAtMs");
        return PREFIX+"|"+installationId.trim()+"|"+expiresAtMs;
    }

    public static String encode(String payload,byte[] signature){
        Base64.Encoder enc=Base64.getUrlEncoder().withoutPadding();
        return PREFIX+"."+enc.encodeToString(payload.getBytes(StandardCharsets.UTF_8))+"."+enc.encodeToString(signature);
    }

    private static PublicKey parsePublicKey(String pem)throws Exception{
        if(pem==null)throw new IllegalArgumentException("publicKey");
        String b64=pem.replace("-----BEGIN PUBLIC KEY-----","")
            .replace("-----END PUBLIC KEY-----","").replaceAll("\\s","");
        byte[] der=Base64.getDecoder().decode(b64);
        return KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(der));
    }

    private static boolean constantEquals(String a,String b){
        byte[] x=a.getBytes(StandardCharsets.UTF_8),y=b.getBytes(StandardCharsets.UTF_8);
        int diff=x.length^y.length,max=Math.max(x.length,y.length);
        for(int i=0;i<max;i++)diff|=(i<x.length?x[i]:0)^(i<y.length?y[i]:0);
        return diff==0;
    }

    private static Result fail(String error){return new Result(false,null,0L,error);}
    private LicenseToken(){}
}
