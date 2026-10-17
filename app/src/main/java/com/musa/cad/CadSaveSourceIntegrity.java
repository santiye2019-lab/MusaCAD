package com.musa.cad;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Read-only CAD file source fingerprint for safe two-phase DXF/DWG exports.
 * All SHA-256 operations are delegated to one reusable portable C core;
 * Java handles only platform file transport and failure decisions.
 *
 * Hash matching proves same observed bytes, not authenticity, signature,
 * atomicity of remote provider writes or concurrent writer exclusion.
 */
public final class CadSaveSourceIntegrity {
    private final long length;
    private final byte[] digest;
    private static final boolean coreLoaded;
    static {
        boolean loaded=false;
        try {
            System.loadLibrary("musacad_dwg");
            loaded=true;
        } catch(LinkageError | SecurityException unavailable) {
            // Missing native core must NEVER silently bypass source fingerprint.
        }
        coreLoaded=loaded;
    }
    private static native long createDigestNative();
    private static native boolean updateDigestNative(long context,byte[] bytes,int length);
    private static native byte[] finishDigestNative(long context);
    private static native void destroyDigestNative(long context);

    private CadSaveSourceIntegrity(long length,byte[] digest){
        this.length=length;this.digest=digest.clone();
    }

    public static CadSaveSourceIntegrity capture(File file)throws IOException{
        if(file==null||!file.isFile())throw new IOException("Kaynak CAD dosyası bulunamadı");
        long expected=file.length();
        if(expected<1)throw new IOException("Boş kaynak CAD dosyası");
        if(!coreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan SHA-256 kaynak kontrolü yapılamaz");

        long state=0;
        try {
            state=createDigestNative();
            if(state==0)throw new IOException("Ortak CAD çekirdeğinde SHA-256 başlatılamadı");
            byte[] buffer=new byte[64*1024];
            long total=0;
            try(BufferedInputStream stream=new BufferedInputStream(new FileInputStream(file))){
                int n;
                while((n=stream.read(buffer))!=-1){
                    if(n==0)continue;
                    // Avoid overflow before adding a file read result.
                    if(n>expected-total)throw new IOException("Kaynak CAD okunurken değişti");
                    total+=n;
                    if(!updateDigestNative(state,buffer,n))
                        throw new IOException("Ortak CAD çekirdeğinde SHA-256 güncellenemedi");
                }
            }
            if(total!=expected||file.length()!=expected)
                throw new IOException("Kaynak CAD okunurken boyutu değişti");
            byte[] hash=finishDigestNative(state);
            if(hash==null||hash.length!=32)
                throw new IOException("Ortak CAD çekirdeği SHA-256 sonucu geçersiz");
            return new CadSaveSourceIntegrity(expected,hash);
        }catch(LinkageError missing){
            throw new IOException("Ortak CAD çekirdeğinde SHA-256 işlevi bulunamadı",missing);
        }finally{
            if(state!=0){
                try{destroyDigestNative(state);}
                catch(LinkageError unavailable){
                    // Native ABI was unloaded/corrupted; cannot recover. The
                    // enclosing capture never treats missing core as success.
                }
            }
        }
    }

    /** Fail closed: original file and staged bytes must be unchanged. */
    public void requireUnchanged(File file)throws IOException{
        CadSaveSourceIntegrity current=capture(file);
        if(length!=current.length||!MessageDigest.isEqual(digest,current.digest))
            throw new IOException("Kaynak CAD dosyası kayıt sırasında değişti");
    }
}
