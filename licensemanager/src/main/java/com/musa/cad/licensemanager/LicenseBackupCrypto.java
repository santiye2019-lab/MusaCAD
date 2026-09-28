package com.musa.cad.licensemanager;

import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.Signature;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Arrays;
import javax.crypto.Cipher;
import javax.crypto.Mac;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/** Pure-Java encrypted backup format for the MusaCAD offline RSA signing key. */
final class LicenseBackupCrypto {
    static final String PREFIX="MUSACAD-LKEY1";
    private static final int ITERATIONS=210_000;
    private static final int KEY_BYTES=32;
    private static final int SALT_BYTES=16;
    private static final int NONCE_BYTES=12;
    private static final SecureRandom RNG=new SecureRandom();
    private static final char[] B64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".toCharArray();

    static final class Bundle {
        final byte[] privatePkcs8,publicX509;
        Bundle(byte[] privatePkcs8,byte[] publicX509){this.privatePkcs8=privatePkcs8;this.publicX509=publicX509;}
    }

    static String encrypt(byte[] privatePkcs8,byte[] publicX509,char[] password)throws Exception{
        requirePassword(password);
        if(privatePkcs8==null||privatePkcs8.length<256||publicX509==null||publicX509.length<128)
            throw new IllegalArgumentException("Lisans anahtarı verisi eksik");
        if(!matches(privatePkcs8,publicX509))throw new IllegalArgumentException("Private/public anahtar eşleşmiyor");

        byte[] salt=random(SALT_BYTES),nonce=random(NONCE_BYTES),key=pbkdf2(password,salt,ITERATIONS,KEY_BYTES);
        try{
            Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");
            cipher.init(Cipher.ENCRYPT_MODE,new SecretKeySpec(key,"AES"),new GCMParameterSpec(128,nonce));
            cipher.updateAAD(aad(publicX509));
            byte[] encrypted=cipher.doFinal(privatePkcs8);
            return PREFIX+"."+b64(salt)+"."+b64(nonce)+"."+b64(publicX509)+"."+b64(encrypted);
        }finally{Arrays.fill(key,(byte)0);}
    }

    static Bundle decrypt(String backup,char[] password)throws Exception{
        requirePassword(password);
        if(backup==null)throw new IllegalArgumentException("Yedek boş");
        String compact=backup.trim().replace("\n","").replace("\r","");
        String[] p=compact.split("\\.");
        if(p.length!=5||!PREFIX.equals(p[0]))throw new IllegalArgumentException("MusaCAD anahtar yedeği biçimi geçersiz");
        byte[] salt=unb64(p[1]),nonce=unb64(p[2]),publicX509=unb64(p[3]),encrypted=unb64(p[4]);
        if(salt.length!=SALT_BYTES||nonce.length!=NONCE_BYTES||publicX509.length<128||encrypted.length<32)
            throw new IllegalArgumentException("MusaCAD anahtar yedeği bozuk");
        byte[] key=pbkdf2(password,salt,ITERATIONS,KEY_BYTES);
        byte[] privatePkcs8;
        try{
            Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");
            cipher.init(Cipher.DECRYPT_MODE,new SecretKeySpec(key,"AES"),new GCMParameterSpec(128,nonce));
            cipher.updateAAD(aad(publicX509));
            privatePkcs8=cipher.doFinal(encrypted);
        }finally{Arrays.fill(key,(byte)0);}
        if(!matches(privatePkcs8,publicX509)){
            Arrays.fill(privatePkcs8,(byte)0);
            throw new IllegalArgumentException("Yedek anahtar doğrulanamadı");
        }
        return new Bundle(privatePkcs8,publicX509);
    }

    static String fingerprint(byte[] publicX509)throws Exception{
        byte[] digest=MessageDigest.getInstance("SHA-256").digest(publicX509);
        StringBuilder out=new StringBuilder();
        for(int i=0;i<12;i++){if(i>0&&i%4==0)out.append('-');out.append(String.format(java.util.Locale.ROOT,"%02X",digest[i]&255));}
        return out.toString();
    }

    private static void requirePassword(char[] password){
        if(password==null||password.length<12)throw new IllegalArgumentException("Yedek parolası en az 12 karakter olmalıdır");
    }

    private static byte[] aad(byte[] publicX509){
        byte[] prefix=PREFIX.getBytes(StandardCharsets.US_ASCII),out=new byte[prefix.length+publicX509.length];
        System.arraycopy(prefix,0,out,0,prefix.length);System.arraycopy(publicX509,0,out,prefix.length,publicX509.length);return out;
    }

    private static boolean matches(byte[] privatePkcs8,byte[] publicX509)throws Exception{
        KeyFactory kf=KeyFactory.getInstance("RSA");
        PrivateKey privateKey=kf.generatePrivate(new PKCS8EncodedKeySpec(privatePkcs8));
        PublicKey publicKey=kf.generatePublic(new X509EncodedKeySpec(publicX509));
        byte[] challenge="MusaCAD-License-Key-Match-v1".getBytes(StandardCharsets.US_ASCII);
        Signature s=Signature.getInstance("SHA256withRSA");s.initSign(privateKey);s.update(challenge);byte[] sig=s.sign();
        Signature v=Signature.getInstance("SHA256withRSA");v.initVerify(publicKey);v.update(challenge);return v.verify(sig);
    }

    /** PBKDF2-HMAC-SHA256 implemented directly for Android 7 compatibility. */
    private static byte[] pbkdf2(char[] password,byte[] salt,int iterations,int length)throws Exception{
        byte[] pw=new String(password).getBytes(StandardCharsets.UTF_8);
        try{
            Mac mac=Mac.getInstance("HmacSHA256");mac.init(new SecretKeySpec(pw,"HmacSHA256"));
            int hLen=mac.getMacLength(),blocks=(length+hLen-1)/hLen;byte[] out=new byte[length];
            for(int block=1;block<=blocks;block++){
                mac.reset();mac.update(salt);mac.update((byte)(block>>>24));mac.update((byte)(block>>>16));mac.update((byte)(block>>>8));mac.update((byte)block);
                byte[] u=mac.doFinal(),t=u.clone();
                for(int i=1;i<iterations;i++){u=mac.doFinal(u);for(int j=0;j<t.length;j++)t[j]^=u[j];}
                int at=(block-1)*hLen,n=Math.min(hLen,length-at);System.arraycopy(t,0,out,at,n);
                Arrays.fill(u,(byte)0);Arrays.fill(t,(byte)0);
            }
            return out;
        }finally{Arrays.fill(pw,(byte)0);}
    }

    private static byte[] random(int n){byte[] out=new byte[n];RNG.nextBytes(out);return out;}

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

    private static byte[] unb64(String s){
        if(s==null)return new byte[0];int mod=s.length()&3;if(mod==1)throw new IllegalArgumentException("Base64 bozuk");
        byte[] out=new byte[s.length()*3/4+3];int n=0,acc=0,bits=0;
        for(int i=0;i<s.length();i++){
            int v=val(s.charAt(i));if(v<0)throw new IllegalArgumentException("Base64 bozuk");
            acc=(acc<<6)|v;bits+=6;if(bits>=8){bits-=8;out[n++]=(byte)((acc>>>bits)&255);}
        }
        if(bits>0&&(acc&((1<<bits)-1))!=0)throw new IllegalArgumentException("Base64 canonical değil");
        return Arrays.copyOf(out,n);
    }
    private static int val(char c){
        if(c>='A'&&c<='Z')return c-'A';if(c>='a'&&c<='z')return c-'a'+26;if(c>='0'&&c<='9')return c-'0'+52;if(c=='-')return 62;if(c=='_')return 63;return -1;
    }
    private LicenseBackupCrypto(){}
}
