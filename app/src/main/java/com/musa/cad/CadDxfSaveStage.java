package com.musa.cad;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * Safe source-side DXF save staging. Validate the entire generated DXF before
 * opening/truncating the user's Storage Access Framework destination.
 *
 * SAF providers do not guarantee atomic destination replacement: a failure
 * *during publish* may still leave a partial destination. Never claim more.
 */
public final class CadDxfSaveStage {
    public interface Writer { void write(OutputStream target)throws IOException; }
    private static final long MAX_OUTPUT_BYTES=512L*1024L*1024L;
    private CadDxfSaveStage(){}

    public static File prepare(File cacheDir,Writer writer)throws IOException{
        if(cacheDir==null||!cacheDir.isDirectory()||writer==null)
            throw new IOException("DXF kayıt alanı kullanılamıyor");
        File staged=File.createTempFile("MusaCAD_dxf_kayit_",".dxf",cacheDir);
        boolean ready=false;
        try{
            FileTransfer.checkCancelled();
            try(OutputStream out=new FileOutputStream(staged)){
                writer.write(out);
            }
            FileTransfer.checkCancelled();
            if(staged.length()==0||staged.length()>MAX_OUTPUT_BYTES)
                throw new IOException("Hazırlanan DXF boş veya dosya sınırını aşıyor");
            validate(staged);
            ready=true;
            return staged;
        }finally{
            if(!ready&&!staged.delete()&&staged.exists())staged.deleteOnExit();
        }
    }

    /** Verifies balanced ASCII-DXF sections, tag pairing and final 0/EOF. */
    public static void validate(File staged)throws IOException{
        if(staged==null||!staged.isFile())throw new IOException("DXF çalışma dosyası yok");
        boolean sectionPending=false,insideSection=false,inEntities=false,hasEntities=false,hasEof=false;
        int records=0;
        try(BufferedReader in=new BufferedReader(
              new InputStreamReader(new FileInputStream(staged),StandardCharsets.ISO_8859_1),128*1024)){
            String codeLine;
            while((codeLine=in.readLine())!=null){
                FileTransfer.checkCancelled();
                String valueLine=in.readLine();
                if(valueLine==null)throw new IOException("DXF etiket çiftinin değeri eksik");
                int code;
                try{code=Integer.parseInt(codeLine.trim());}
                catch(NumberFormatException bad){throw new IOException("Geçersiz DXF etiket kodu",bad);}
                String value=valueLine.trim();
                if(hasEof)throw new IOException("DXF EOF sonrasında beklenmeyen veri");
                if(sectionPending){
                    if(code!=2||value.isEmpty())throw new IOException("DXF SECTION adı geçersiz");
                    sectionPending=false;insideSection=true;inEntities="ENTITIES".equalsIgnoreCase(value);
                    if(inEntities)hasEntities=true;
                }else if(code==0){
                    if("SECTION".equals(value)){
                        if(insideSection)throw new IOException("İç içe DXF SECTION");
                        sectionPending=true;
                    }else if("ENDSEC".equals(value)){
                        if(!insideSection)throw new IOException("DXF ENDSEC için SECTION yok");
                        insideSection=false;inEntities=false;
                    }else if("EOF".equals(value)){
                        if(insideSection)throw new IOException("DXF kapanmamış SECTION içeriyor");
                        hasEof=true;
                    }
                }
                if(++records>80_000_000)throw new IOException("DXF etiket sayısı güvenlik sınırını aşıyor");
            }
        }
        if(sectionPending||insideSection||!hasEntities||!hasEof)
            throw new IOException("DXF ENTITIES veya EOF yapısı eksik");
    }

    public static long publish(File staged,OutputStream destination)throws IOException{
        if(staged==null||!staged.isFile()||destination==null)
            throw new IOException("DXF aktarma dosyası veya hedefi bulunamadı");
        // Last validation before destination open is caller's responsibility;
        // prepare() always validates first.
        try(InputStream source=new FileInputStream(staged)){
            return FileTransfer.copy(source,destination,MAX_OUTPUT_BYTES,null);
        }
    }
}
