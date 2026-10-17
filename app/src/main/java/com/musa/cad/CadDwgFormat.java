package com.musa.cad;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * Android adapter for the ONE portable DWG header classifier in
 * core/native/src/musacad_dwg_signature.c. The display labels stay in Java,
 * but Java NEVER decides whether any DWG signature is supported.
 *
 * AC1032 is a DWG file-family signature, not an AutoCAD 2026 feature claim.
 * This read-only check never edits source CAD bytes.
 */
public final class CadDwgFormat {
    private CadDwgFormat(){}
    private static final boolean sharedCoreLoaded;
    static {
        boolean ready=false;
        try {
            System.loadLibrary("musacad_dwg");
            ready=true;
        } catch (LinkageError | SecurityException unavailable) {
            // No fallback classifier: the shared engine is authoritative.
        }
        sharedCoreLoaded=ready;
    }

    private static native int identifyNative(byte[] firstSixBytes);

    public static final class Info {
        public final String signature,formatFamily;
        public final boolean supportedSignature,modern2018Family;
        Info(String signature,String family,boolean supported,boolean modern){
            this.signature=signature;
            formatFamily=family;
            supportedSignature=supported;
            modern2018Family=modern;
        }
        public String label(){
            return supportedSignature?
                    formatFamily+" DWG ("+signature+")":
                    "Bilinmeyen DWG biçimi";
        }
    }

    public static Info identify(File source)throws IOException{
        if(source==null||!source.isFile())
            throw new IOException("Kaynak DWG dosyası bulunamadı");
        byte[] six=new byte[6];
        int off=0;
        try(FileInputStream in=new FileInputStream(source)){
            while(off<6){
                int n=in.read(six,off,6-off);
                if(n<0)throw new IOException("Eksik DWG dosya başlığı");
                if(n==0)continue;
                off+=n;
            }
        }
        return identify(six);
    }

    public static Info identify(byte[] header){
        if(header==null||header.length<6)
            throw new IllegalArgumentException("eksik DWG imzası");
        if(!sharedCoreLoaded)
            throw new IllegalStateException("Ortak CAD çekirdeği olmadan DWG biçimi belirlenemez");
        final int version;
        try {
            version=identifyNative(header);
        } catch(LinkageError unavailable) {
            throw new IllegalStateException("Ortak CAD çekirdeğinde DWG imza tanıyıcısı bulunamadı",unavailable);
        }
        // Negative return = non-printable/binary header, no printable signature.
        if(version<0)return new Info("","",false,false);
        String signature=new String(header,0,6,StandardCharsets.US_ASCII);
        final String family;
        switch(version){
            case 1:family="R12";break;
            case 2:family="R13";break;
            case 3:family="R14";break;
            case 4:family="2000";break;
            case 5:family="2004";break;
            case 6:family="2007";break;
            case 7:family="2010";break;
            case 8:family="2013";break;
            case 9:family="2018–2026";break;
            default:return new Info(signature,"",false,false);
        }
        return new Info(signature,family,true,version==9);
    }
}
