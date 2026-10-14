package com.musa.cad;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * Reads the six-byte ACxxxx DWG file signature, NOT AutoCAD's app version.
 *
 * AutoCAD 2026 uses AC1032 (2018-family DWG). A modern-format header is
 * only a format identifier, NOT proof of a working AC1032 writer, valid file,
 * lossless object mapping or ACIS feature parity.
 *
 * This sniffer never modifies a source DWG and does not trigger a converter.
 */
public final class CadDwgFormat {
    private CadDwgFormat(){}

    public static final class Info {
        public final String signature,formatFamily;
        public final boolean supportedSignature,modern2018Family;
        Info(String signature,String family,boolean supported){
            this.signature=signature;
            formatFamily=family;
            supportedSignature=supported;
            modern2018Family="AC1032".equals(signature);
        }
        /** User-facing diagnosis, NOT an offer of native DWG Save As. */
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
        for(int i=0;i<6;i++){
            int c=header[i]&0xff;
            if(c<0x20||c>0x7e)
                return new Info("", "",false);
        }
        String magic=new String(header,0,6,StandardCharsets.US_ASCII);
        switch(magic){
            case "AC1009": return new Info(magic,"R12",true);
            case "AC1012": return new Info(magic,"R13",true);
            case "AC1014": return new Info(magic,"R14",true);
            case "AC1015": return new Info(magic,"2000",true);
            case "AC1018": return new Info(magic,"2004",true);
            case "AC1021": return new Info(magic,"2007",true);
            case "AC1024": return new Info(magic,"2010",true);
            case "AC1027": return new Info(magic,"2013",true);
            case "AC1032": return new Info(magic,"2018–2026",true);
            default: return new Info(magic,"",false);
        }
    }
}
