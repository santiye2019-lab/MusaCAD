package com.musa.cad.licensemanager;

import android.content.Context;
import android.content.SharedPreferences;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Arrays;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;

final class LicenseKeyStore {
    private static final String STORE="AndroidKeyStore";
    private static final String WRAP_ALIAS="musacad_license_private_wrap_v2";
    private static final String PREFS="musacad_license_signing_key_v2";
    private static final String K_PRIVATE="private_wrapped",K_NONCE="private_nonce",K_PUBLIC="public_x509";

    static boolean hasKey(Context c){
        SharedPreferences p=c.getSharedPreferences(PREFS,0);
        return !p.getString(K_PRIVATE,"").isEmpty()&&!p.getString(K_NONCE,"").isEmpty()&&!p.getString(K_PUBLIC,"").isEmpty();
    }

    /** Creates a password-encrypted backup for a new RSA key without activating it on the device. */
    static String generateBackup(char[] backupPassword)throws Exception{
        KeyPairGenerator g=KeyPairGenerator.getInstance("RSA");
        g.initialize(2048);
        KeyPair pair=g.generateKeyPair();
        byte[] priv=pair.getPrivate().getEncoded(),pub=pair.getPublic().getEncoded();
        try{return LicenseBackupCrypto.encrypt(priv,pub,backupPassword);}
        finally{Arrays.fill(priv,(byte)0);}
    }

    static String backup(Context c,char[] backupPassword)throws Exception{
        byte[] priv=privateBytes(c),pub=publicBytes(c);
        try{return LicenseBackupCrypto.encrypt(priv,pub,backupPassword);}
        finally{Arrays.fill(priv,(byte)0);}
    }

    static void restore(Context c,String backup,char[] password)throws Exception{
        LicenseBackupCrypto.Bundle b=LicenseBackupCrypto.decrypt(backup,password);
        try{storeOperational(c,b.privatePkcs8,b.publicX509);}
        finally{Arrays.fill(b.privatePkcs8,(byte)0);}
    }

    static PrivateKey privateKey(Context c)throws Exception{
        byte[] pkcs8=privateBytes(c);
        try{return KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(pkcs8));}
        finally{Arrays.fill(pkcs8,(byte)0);}
    }

    static PublicKey publicKey(Context c)throws Exception{
        return KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(publicBytes(c)));
    }

    static String publicKeyPem(Context c)throws Exception{
        String raw=Base64.encodeToString(publicBytes(c),Base64.NO_WRAP);
        StringBuilder b=new StringBuilder();
        for(int i=0;i<raw.length();i+=64){if(i>0)b.append('\n');b.append(raw,i,Math.min(raw.length(),i+64));}
        return "-----BEGIN PUBLIC KEY-----\n"+b+"\n-----END PUBLIC KEY-----\n";
    }

    static String fingerprint(Context c)throws Exception{return LicenseBackupCrypto.fingerprint(publicBytes(c));}

    private static void storeOperational(Context c,byte[] privatePkcs8,byte[] publicX509)throws Exception{
        SecretKey key=wrapKey();
        Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");

        // AndroidKeyStore GCM keys with randomized encryption enabled must generate
        // their own IV. Supplying one here triggers:
        // "Caller-provided IV not permitted".
        cipher.init(Cipher.ENCRYPT_MODE,key);
        byte[] nonce=cipher.getIV();
        if(nonce==null||nonce.length==0)throw new IllegalStateException("Android Keystore nonce üretemedi");

        cipher.updateAAD(publicX509);
        byte[] wrapped=cipher.doFinal(privatePkcs8);
        boolean ok=c.getSharedPreferences(PREFS,0).edit()
            .putString(K_PRIVATE,Base64.encodeToString(wrapped,Base64.NO_WRAP))
            .putString(K_NONCE,Base64.encodeToString(nonce,Base64.NO_WRAP))
            .putString(K_PUBLIC,Base64.encodeToString(publicX509,Base64.NO_WRAP))
            .commit();
        if(!ok)throw new IllegalStateException("Lisans anahtarı güvenli depoya yazılamadı");
    }

    private static byte[] privateBytes(Context c)throws Exception{
        SharedPreferences p=c.getSharedPreferences(PREFS,0);
        byte[] wrapped=decodeRequired(p.getString(K_PRIVATE,""),"Özel lisans anahtarı bulunamadı");
        byte[] nonce=decodeRequired(p.getString(K_NONCE,""),"Lisans anahtarı nonce bulunamadı");
        byte[] pub=publicBytes(c);
        Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(Cipher.DECRYPT_MODE,wrapKey(),new GCMParameterSpec(128,nonce));
        cipher.updateAAD(pub);
        return cipher.doFinal(wrapped);
    }

    private static byte[] publicBytes(Context c){
        String value=c.getSharedPreferences(PREFS,0).getString(K_PUBLIC,"");
        return decodeRequired(value,"Public lisans anahtarı bulunamadı");
    }

    private static byte[] decodeRequired(String value,String error){
        if(value==null||value.isEmpty())throw new IllegalStateException(error);
        return Base64.decode(value,Base64.DEFAULT);
    }

    private static SecretKey wrapKey()throws Exception{
        KeyStore ks=KeyStore.getInstance(STORE);ks.load(null);
        if(ks.containsAlias(WRAP_ALIAS)){
            KeyStore.Entry entry=ks.getEntry(WRAP_ALIAS,null);
            if(entry instanceof KeyStore.SecretKeyEntry)return ((KeyStore.SecretKeyEntry)entry).getSecretKey();
            throw new IllegalStateException("Android Keystore sarma anahtarı geçersiz");
        }
        KeyGenerator g=KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES,STORE);
        g.init(new KeyGenParameterSpec.Builder(WRAP_ALIAS,KeyProperties.PURPOSE_ENCRYPT|KeyProperties.PURPOSE_DECRYPT)
            .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
            .setKeySize(256)
            .setRandomizedEncryptionRequired(true)
            .build());
        return g.generateKey();
    }

    private LicenseKeyStore(){}
}
