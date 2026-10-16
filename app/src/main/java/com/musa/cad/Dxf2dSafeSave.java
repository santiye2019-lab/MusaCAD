package com.musa.cad;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

/**
 * Two-phase DXF publisher for 2D drawings.
 *
 * Content providers can truncate a SAF document as soon as "wt" is opened;
 * DXFWriter, source guards, disk-space failures, and output validation MUST
 * finish on a private temporary file before the public destination opens.
 * This does not make a remote/provider-side write atomic.
 */
public final class Dxf2dSafeSave {
    private Dxf2dSafeSave(){}

    public interface StageWriter {
        void write(OutputStream destination)throws IOException;
    }
    public interface StageVerifier {
        void verify(File stage)throws IOException;
    }
    public interface DestinationOpener {
        OutputStream open()throws IOException;
    }

    public static final class VerifiedStage {
        private final File file,source;
        private final CadSaveSourceIntegrity sourceBefore,bytesBefore;
        private VerifiedStage(File file,File source,CadSaveSourceIntegrity sourceBefore)
                throws IOException{
            this.file=file;
            this.source=source;
            this.sourceBefore=sourceBefore;
            this.bytesBefore=CadSaveSourceIntegrity.capture(file);
        }
        public File file(){return file;}
        public void verify()throws IOException{
            // No file swap or concurrent source modification may bypass the
            // fingerprint; do not trust filename/size/mtime alone.
            if(!file.isFile()||!file.getCanonicalFile().equals(file.getAbsoluteFile()))
                throw new IOException("DXF geçici kayıt dosyası değiştirildi");
            bytesBefore.requireUnchanged(file);
            sourceBefore.requireUnchanged(source);
        }
        public boolean delete(){return file.delete();}
    }

    /** Caller owns and deletes a returned stage in finally. */
    public static VerifiedStage stage(File source,File cacheDir,
                                      StageWriter writer,StageVerifier verifier)
            throws IOException{
        if(source==null||!source.isFile()||cacheDir==null||!cacheDir.isDirectory()||
           writer==null||verifier==null)
            throw new IOException("2B DXF kaynak veya geçici kayıt alanı bulunamadı");
        if(!cacheDir.getCanonicalFile().isDirectory())
            throw new IOException("DXF geçici kayıt klasörü geçersiz");
        CadSaveSourceIntegrity sourceBefore=CadSaveSourceIntegrity.capture(source);
        File stage=File.createTempFile("musacad-2d-", ".dxf",cacheDir);
        boolean complete=false;
        try{
            try(FileOutputStream raw=new FileOutputStream(stage,false);
                BufferedOutputStream out=new BufferedOutputStream(raw,128*1024)){
                writer.write(out);
                out.flush();
                raw.getFD().sync();
            }
            if(stage.length()<12||stage.length()>512L*1024*1024)
                throw new IOException("Oluşturulan 2B DXF geçersiz boyutta");
            sourceBefore.requireUnchanged(source);
            verifier.verify(stage);
            sourceBefore.requireUnchanged(source);
            VerifiedStage verified=new VerifiedStage(stage,source,sourceBefore);
            verified.verify();
            complete=true;
            return verified;
        }finally{
            if(!complete)stage.delete();
        }
    }

    /** Provider output is opened only after the staged bytes pass verification. */
    public static long publish(VerifiedStage stage,DestinationOpener opener)throws IOException{
        if(stage==null||opener==null)
            throw new IOException("DXF kayıt dosyası veya hedefi bulunamadı");
        stage.verify();
        try(BufferedInputStream input=new BufferedInputStream(
                new FileInputStream(stage.file),128*1024);
            OutputStream destination=opener.open()){
            if(destination==null)throw new IOException("DXF kayıt hedefi açılamadı");
            long copied=FileTransfer.copy(input,destination,0,null);
            destination.flush();
            stage.verify();
            return copied;
        }
    }
}
