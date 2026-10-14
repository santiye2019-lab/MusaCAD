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

    /** Caller owns the staged cache file and must delete it in a finally block. */
    public static File stage(File source,Dxf3dMesh edited,float[] original,File cacheDir)throws IOException{
        if(source==null||!source.isFile()||edited==null||original==null||cacheDir==null)
            throw new IOException("3B kaynak dosyası veya geçici kayıt alanı bulunamadı");
        // Verify the original CAD bytes, not only size and mtime.
        // A concurrent same-length edit must never produce a valid-looking
        // export based on a different source drawing.
        CadSaveSourceIntegrity baseline=CadSaveSourceIntegrity.capture(source);
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
                ||!Arrays.equals(edited.edges,reopened.edges))
                throw new IOException("3B DXF yeniden açıldığında geometri değişti; kayıt yapılmadı");
            // Validate AFTER output creation and reopening. If another editor
            // changed the source during the work, never return this stage for
            // publish to a SAF destination. Failure cleanup runs in finally.
            baseline.requireUnchanged(source);
            ready=true;
            return staged;
        }finally{
            if(!ready)staged.delete();
        }
    }

    /** The destination stream is opened by the caller only after stage() succeeds. */
    public static long publish(File staged,OutputStream destination)throws IOException{
        if(staged==null||!staged.isFile()||staged.length()==0||destination==null)
            throw new IOException("Doğrulanmış 3B DXF veya kayıt hedefi bulunamadı");
        try(BufferedInputStream input=new BufferedInputStream(new FileInputStream(staged))){
            return FileTransfer.copy(input,destination,0,null);
        }
    }
}
