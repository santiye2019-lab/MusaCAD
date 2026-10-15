package com.musa.cad;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;

/**
 * HOST/EXPERIMENTAL modern DWG two-phase publisher. NOT connected to Android,
 * not a DWG encoder and NOT evidence that an AC1032 writer exists.
 *
 * Requires a separately implemented AC1032 binary encoder AND an independent
 * semantic CAD reopen verifier. A correct six-byte header alone is NOT enough.
 * Original DWG and editable DXF are never modified by this class. Publication
 * is create-new-only using an exclusive hard link in the same directory;
 * unsupported filesystems fail closed (no non-atomic overwrite fallback).
 */
public final class CadAc1032StagedSave {
    private CadAc1032StagedSave(){}

    /** Real encoder must produce AC1032 binary DWG; DXF bytes are not accepted. */
    public interface Encoder {
        void encode(File originalDwg,File editedDxf,OutputStream target)
                throws IOException;
    }

    /**
     * MUST independently decode the staged binary and compare actual geometry,
     * colors, blocks/XREF, layouts, handles and unsupported object semantics
     * with expected edited project state. Caller must NOT approve merely
     * because a file exists, has AC1032 magic, or matches entity counts.
     */
    public interface IndependentReopenVerifier {
        boolean verify(File originalDwg,File editedDxf,File stagedAc1032)
                throws IOException;
    }

    /**
     * Save only after BOTH input snapshots are stable and the candidate's
     * full bytes are stable across semantic verification.
     *
     * No writer/verifier configured => IOException, never a published DWG.
     */
    public static void publishNew(File originalDwg,File editedDxf,
            File requestedOutput,Encoder encoder,IndependentReopenVerifier verifier)
            throws IOException{
        if(encoder==null||verifier==null)
            throw new IOException("AC1032 yazıcı ve bağımsız CAD doğrulayıcı gerekli");
        if(originalDwg==null||editedDxf==null||requestedOutput==null)
            throw new IOException("AC1032 kaynak veya hedef belirtilmedi");
        Path source=originalDwg.toPath().toAbsolutePath().normalize();
        Path edit=editedDxf.toPath().toAbsolutePath().normalize();
        Path target=requestedOutput.toPath().toAbsolutePath().normalize();
        if(source.equals(edit)||source.equals(target)||edit.equals(target))
            throw new IOException("Kaynak DWG/DXF üzerine kaydetmek yasak");
        if(Files.isSymbolicLink(source)||Files.isSymbolicLink(edit)||
           Files.isSymbolicLink(target))
            throw new IOException("Sembolik bağlantı ile CAD kaydı yasak");
        if(!Files.isRegularFile(source)||!Files.isRegularFile(edit))
            throw new IOException("DWG/DXF kaynak dosyaları bulunamadı");
        if(Files.isSameFile(source,edit))
            throw new IOException("Kaynak DWG ve DXF aynı dosyaya bağlı");
        if(!CadDwgFormat.identify(originalDwg).modern2018Family)
            throw new IOException("Kaynak dosya AC1032 değil");
        if(Files.exists(target,LinkOption.NOFOLLOW_LINKS))
            throw new FileAlreadyExistsException(target.toString());
        Path parent=target.getParent();
        if(parent==null||!Files.isDirectory(parent))
            throw new IOException("DWG hedef klasörü yok");
        CadSaveSourceIntegrity sourceBefore=CadSaveSourceIntegrity.capture(originalDwg);
        CadSaveSourceIntegrity editBefore=CadSaveSourceIntegrity.capture(editedDxf);
        Path staged=Files.createTempFile(parent,".musacad-ac1032-", ".dwg");
        try{
            try(FileOutputStream out=new FileOutputStream(staged.toFile(),false)){
                encoder.encode(originalDwg,editedDxf,out);
                out.flush();
                out.getFD().sync();
            }
            sourceBefore.requireUnchanged(originalDwg);
            editBefore.requireUnchanged(editedDxf);
            if(Files.isSymbolicLink(staged)||
               !Files.isRegularFile(staged,LinkOption.NOFOLLOW_LINKS)||
               Files.size(staged)<64||
               !CadDwgFormat.identify(staged.toFile()).modern2018Family)
                throw new IOException("Kodlayıcı geçerli AC1032 DWG üretmedi");
            CadSaveSourceIntegrity candidateBefore=
                    CadSaveSourceIntegrity.capture(staged.toFile());
            if(!verifier.verify(originalDwg,editedDxf,staged.toFile()))
                throw new IOException("AC1032 bağımsız yeniden açma/semantik testi başarısız");
            // A malicious/broken verifier could mutate the staged output,
            // or an editor may modify a source during its longer analysis.
            if(Files.isSymbolicLink(staged)||
               !Files.isRegularFile(staged,LinkOption.NOFOLLOW_LINKS))
                throw new IOException("AC1032 sahne dosyası değiştirildi");
            candidateBefore.requireUnchanged(staged.toFile());
            sourceBefore.requireUnchanged(originalDwg);
            editBefore.requireUnchanged(editedDxf);
            // Atomically creates a destination entry only if it does not
            // already exist. Never rename with overwrite semantics.
            Files.createLink(target,staged);
        }finally{
            Files.deleteIfExists(staged);
        }
    }
}
