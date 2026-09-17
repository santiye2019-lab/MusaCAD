package com.musa.cad;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.X509EncodedKeySpec;

/** Pure-Java verifier for offline, installation-bound MusaCAD license tokens. */
public final class LicenseToken {
    public static final String PREFIX="MC1";
    private static final char[] URL64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".toCharArray();

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
            byte[] payloadBytes=decode64(parts[1],true);
            byte[] signatureBytes=decode64(parts[2],true);
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
        return PREFIX+"."+encodeUrl64(payload.getBytes(StandardCharsets.UTF_8))+"."+encodeUrl64(signature);
    }

    private static PublicKey parsePublicKey(String pem)throws Exception{
        if(pem==null)throw new IllegalArgumentException("publicKey");
        String b64=pem.replace("-----BEGIN PUBLIC KEY-----","").replace("-----END PUBLIC KEY-----","").replaceAll("\\s","");
        byte[] der=decode64(b64,false);
        return KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(der));
    }

    private static String encodeUrl64(byte[] data){
        StringBuilder out=new StringBuilder((data.length*4+2)/3);
        for(int i=0;i<data.length;i+=3){
            int b0=data[i]&255,b1=i+1<data.length?data[i+1]&255:0,b2=i+2<data.length?data[i+2]&255:0;
            out.append(URL64[b0>>>2]);out.append(URL64[((b0&3)<<4)|(b1>>>4)]);
            if(i+1<data.length)out.append(URL64[((b1&15)<<2)|(b2>>>6)]);
            if(i+2<data.length)out.append(URL64[b2&63]);
        }
        return out.toString();
    }

    private static byte[] decode64(String s,boolean url)throws Exception{
        String clean=s.replace("=","").replaceAll("\\s","");
        int mod=clean.length()&3;
        if(mod==1)throw new IllegalArgumentException("base64 length");
        ByteArrayOutputStream out=new ByteArrayOutputStream(clean.length()*3/4);
        int acc=0,bits=0;
        for(int i=0;i<clean.length();i++){
            int v=value64(clean.charAt(i),url);if(v<0)throw new IllegalArgumentException("base64");
            acc=(acc<<6)|v;bits+=6;
            if(bits>=8){bits-=8;out.write((acc>>>bits)&255);}
        }
        // Unpadded Base64 has 2 or 4 unused low bits. They must be zero; otherwise
        // multiple textual license codes could decode to the same signed byte sequence.
        if(bits>0&&(acc&((1<<bits)-1))!=0)throw new IllegalArgumentException("noncanonical base64");
        return out.toByteArray();
    }

    private static int value64(char c,boolean url){
        if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;
        if(c=='+'||url&&c=='-')return 62;if(c=='/'||url&&c=='_')return 63;return -1;
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
