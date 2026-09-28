package com.musa.cad.licensemanager;

import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.util.Arrays;

public final class LicenseBackupCryptoTest {
    public static void main(String[] args)throws Exception{
        KeyPairGenerator g=KeyPairGenerator.getInstance("RSA");g.initialize(2048);KeyPair pair=g.generateKeyPair();
        char[] password="MusaCAD-Backup-2026!".toCharArray();
        String backup=LicenseBackupCrypto.encrypt(pair.getPrivate().getEncoded(),pair.getPublic().getEncoded(),password);
        if(!backup.startsWith(LicenseBackupCrypto.PREFIX+"."))throw new AssertionError("backup prefix");
        LicenseBackupCrypto.Bundle restored=LicenseBackupCrypto.decrypt(backup,password);
        if(!Arrays.equals(pair.getPrivate().getEncoded(),restored.privatePkcs8))throw new AssertionError("private key round trip");
        if(!Arrays.equals(pair.getPublic().getEncoded(),restored.publicX509))throw new AssertionError("public key round trip");
        String fp=LicenseBackupCrypto.fingerprint(restored.publicX509);
        if(fp.split("-").length!=3)throw new AssertionError("fingerprint format");

        boolean wrong=false;try{LicenseBackupCrypto.decrypt(backup,"Wrong-Password-2026!".toCharArray());}catch(Exception expected){wrong=true;}
        if(!wrong)throw new AssertionError("wrong password must fail");

        String tampered=backup.substring(0,backup.length()-1)+(backup.endsWith("A")?"B":"A");
        boolean corrupt=false;try{LicenseBackupCrypto.decrypt(tampered,password);}catch(Exception expected){corrupt=true;}
        if(!corrupt)throw new AssertionError("tampered backup must fail");

        boolean shortPw=false;try{LicenseBackupCrypto.encrypt(pair.getPrivate().getEncoded(),pair.getPublic().getEncoded(),"short".toCharArray());}catch(Exception expected){shortPw=true;}
        if(!shortPw)throw new AssertionError("short backup password must fail");

        System.out.println("License backup crypto passed: AES-GCM round trip, wrong-password/tamper rejection and fingerprint.");
    }
}
