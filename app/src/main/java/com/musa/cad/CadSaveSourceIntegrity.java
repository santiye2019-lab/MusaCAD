package com.musa.cad;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

/**
 * Immutable SHA-256 snapshot for a source CAD file during two-phase export.
 * A same-size, same-timestamp edit must not slip through staging.
 * Android-free so the exact implementation is covered by JVM tests.
 */
public final class CadSaveSourceIntegrity {
    private final long length;
    private final byte[] digest;

    private CadSaveSourceIntegrity(long length,byte[] digest){
        this.length=length;this.digest=digest.clone();
    }

    public static CadSaveSourceIntegrity capture(File file)throws IOException{
        if(file==null||!file.isFile())throw new IOException("Kaynak CAD dosyası bulunamadı");
        long expected=file.length();
        if(expected<1)throw new IOException("Boş kaynak CAD dosyası");
        MessageDigest sha;
        try{sha=MessageDigest.getInstance("SHA-256");}
        catch(NoSuchAlgorithmException e){throw new IOException("SHA-256 kullanılamıyor",e);}
        byte[] buffer=new byte[64*1024];
        long total=0;
        try(BufferedInputStream stream=new BufferedInputStream(new FileInputStream(file))){
            int n;
            while((n=stream.read(buffer))!=-1){
                total+=n;
                if(total>expected)throw new IOException("Kaynak CAD okunurken değişti");
                sha.update(buffer,0,n);
            }
        }
        if(total!=expected||file.length()!=expected)
            throw new IOException("Kaynak CAD okunurken boyutu değişti");
        return new CadSaveSourceIntegrity(expected,sha.digest());
    }

    /** Fail closed; never publish a stage based on a changed CAD source. */
    public void requireUnchanged(File file)throws IOException{
        CadSaveSourceIntegrity current=capture(file);
        if(length!=current.length||!Arrays.equals(digest,current.digest))
            throw new IOException("Kaynak CAD dosyası kayıt sırasında değişti");
    }
}
