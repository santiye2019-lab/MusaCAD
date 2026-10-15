package com.musa.cad;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;

/**
 * Two-phase 3D DXF export. SAF content URIs may truncate on openOutputStream("wt");
 * all geometry edits must therefore be written and round-trip checked first.
 * This is DXF mesh/vertex editing, not a native ACIS solid or DWG writer.
 */
public final class Dxf3dSafeSave {
    private Dxf3dSafeSave(){}

    /** Opens the destination only after the validated stage is checked again. */
    public interface DestinationOpener {
        OutputStream open() throws IOException;
    }

    /** Carries the verified byte identity across the stage/publish boundary. */
    public static final class VerifiedStage {
        private final File file;
        private final CadSaveSourceIntegrity verifiedBytes;
        private VerifiedStage(File file)throws IOException{
            this.file=file;
            this.verifiedBytes=CadSaveSourceIntegrity.capture(file);
        }
        public File file(){return file;}
        public void verify()throws IOException{
            // Android API 24 compatible symlink guard (no java.nio.file).
            if(!file.isFile()||!file.getCanonicalFile().equals(file.getAbsoluteFile()))
                throw new IOException("Doğrulanmış 3B geçici dosya değiştirildi");
            verifiedBytes.requireUnchanged(file);
        }
        public boolean delete(){return file.delete();}
    }

    /** Caller owns the staged cache file and must delete it in a finally block. */
    public static VerifiedStage stage(File source,Dxf3dMesh edited,float[] original,File cacheDir,
            CadSaveSourceIntegrity openedSource)throws IOException{
        if(source==null||!source.isFile()||edited==null||original==null||cacheDir==null||openedSource==null)
            throw new IOException("3B kaynak dosyası veya geçici kayıt alanı bulunamadı");
        // Refuse a source changed any time after the 3D Activity opened it,
        // including layer names, XREF paths and other unrendered DXF data.
        // A geometry-only preflight cannot detect those modifications.
        openedSource.requireUnchanged(source);
        // Verify the original CAD bytes, not only size and mtime.
        // A concurrent same-length edit must never produce a valid-looking
        // export based on a different source drawing.
        CadSaveSourceIntegrity baseline=CadSaveSourceIntegrity.capture(source);
        // A caller may have opened the drawing hours earlier. Hashing the
        // source only at save time cannot detect changes predating stage().
        // Reopen the source and match the exact geometry/identity that the
        // edit session originally read before applying any coordinate patch.
        Dxf3dMesh current=Dxf3dMesh.read(source);
        if(!Arrays.equals(current.xyz,original)||
            !Arrays.equals(current.triangles,edited.triangles)||
            !Arrays.equals(current.edges,edited.edges)||
            !Arrays.equals(current.sourceLines,edited.sourceLines)||
            !Arrays.equals(current.coordinateSlots,edited.coordinateSlots)||
            current.acisSolids!=edited.acisSolids||
            current.unsupportedSolids!=edited.unsupportedSolids||
            current.skippedBlockReferences!=edited.skippedBlockReferences)
            throw new IOException("3B kaynak çizim açıldıktan sonra değişmiş; yeniden açmadan kayıt yapılamaz");
        baseline.requireUnchanged(source);
        openedSource.requireUnchanged(source);
        File staged=File.createTempFile("musacad-3d-", ".dxf", cacheDir);
        boolean ready=false;
        try{
            try(OutputStream output=new BufferedOutputStream(new FileOutputStream(staged))){
                Dxf3dEditor.write(source,output,edited,original);
            }
            if(staged.length()==0)throw new IOException("Hazırlanan 3B DXF boş");
            // Verify that the saved geometry, not just the syntax, survived the writer.
            Dxf3dMesh reopened=Dxf3dMesh.read(staged);
            if(!Arrays.equals(edited.xyz,reopened.xyz)
                ||!Arrays.equals(edited.triangles,reopened.triangles)
                ||!Arrays.equals(edited.edges,reopened.edges)
                ||!Arrays.equals(edited.sourceLines,reopened.sourceLines)
                ||!Arrays.equals(edited.coordinateSlots,reopened.coordinateSlots)
                ||edited.acisSolids!=reopened.acisSolids
                ||edited.unsupportedSolids!=reopened.unsupportedSolids
                ||edited.skippedBlockReferences!=reopened.skippedBlockReferences)
                throw new IOException("3B DXF yeniden açıldığında geometri değişti; kayıt yapılmadı");
            // Validate AFTER output creation and reopening. If another editor
            // changed the source during the work, never return this stage for
            // publish to a SAF destination. Failure cleanup runs in finally.
            baseline.requireUnchanged(source);
            openedSource.requireUnchanged(source);
            VerifiedStage checked=new VerifiedStage(staged);
            checked.verify();
            ready=true;
            return checked;
        }finally{
            if(!ready)staged.delete();
        }
    }

    /**
     * Never call SAF's potentially truncating openOutputStream until the staged
     * bytes are reauthenticated. No unverified File/output-stream overload.
     */
    public static long publish(VerifiedStage staged,DestinationOpener opener)throws IOException{
        if(staged==null||opener==null)
            throw new IOException("Doğrulanmış 3B DXF veya kayıt hedefi bulunamadı");
        staged.verify(); // MUST run before opening/truncating the destination.
        try(BufferedInputStream input=new BufferedInputStream(new FileInputStream(staged.file));
            OutputStream destination=opener.open()){
            if(destination==null)throw new IOException("3B DXF kayıt hedefi açılamadı");
            long copied=FileTransfer.copy(input,destination,0,null);
            staged.verify(); // detect concurrent mutation during the transfer
            return copied;
        }
    }
}
