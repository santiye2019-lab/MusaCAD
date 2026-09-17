package com.musa.cad.licensemanager;

import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import java.security.*;
import java.security.cert.Certificate;
import android.util.Base64;

final class LicenseKeyStore {
    private static final String STORE="AndroidKeyStore";
    private static final String ALIAS="musacad_paid_license_signing_v1";

    static void ensureKey() throws Exception {
        KeyStore ks=KeyStore.getInstance(STORE);ks.load(null);
        if(ks.containsAlias(ALIAS))return;
        KeyPairGenerator g=KeyPairGenerator.getInstance(KeyProperties.KEY_ALGORITHM_RSA,STORE);
        KeyGenParameterSpec spec=new KeyGenParameterSpec.Builder(
            ALIAS,
            KeyProperties.PURPOSE_SIGN|KeyProperties.PURPOSE_VERIFY)
            .setKeySize(2048)
            .setDigests(KeyProperties.DIGEST_SHA256)
            .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
            .build();
        g.initialize(spec);
        g.generateKeyPair();
    }

    static PrivateKey privateKey() throws Exception {
        KeyStore ks=KeyStore.getInstance(STORE);ks.load(null);
        KeyStore.Entry e=ks.getEntry(ALIAS,null);
        if(!(e instanceof KeyStore.PrivateKeyEntry))throw new GeneralSecurityException("Lisans anahtarı bulunamadı");
        return ((KeyStore.PrivateKeyEntry)e).getPrivateKey();
    }

    static PublicKey publicKey() throws Exception {
        KeyStore ks=KeyStore.getInstance(STORE);ks.load(null);
        Certificate c=ks.getCertificate(ALIAS);
        if(c==null)throw new GeneralSecurityException("Public key bulunamadı");
        return c.getPublicKey();
    }

    static String publicKeyPem() throws Exception {
        String raw=Base64.encodeToString(publicKey().getEncoded(),Base64.NO_WRAP);
        StringBuilder b64=new StringBuilder();
        for(int i=0;i<raw.length();i+=64){if(i>0)b64.append('\n');b64.append(raw, i, Math.min(raw.length(),i+64));}
        return "-----BEGIN PUBLIC KEY-----\n"+b64+"\n-----END PUBLIC KEY-----\n";
    }

    static String fingerprint() throws Exception {
        byte[] d=MessageDigest.getInstance("SHA-256").digest(publicKey().getEncoded());
        StringBuilder s=new StringBuilder();
        for(int i=0;i<12;i++){if(i>0&&i%4==0)s.append('-');s.append(String.format(java.util.Locale.ROOT,"%02X",d[i]&255));}
        return s.toString();
    }

    private LicenseKeyStore(){}
}
