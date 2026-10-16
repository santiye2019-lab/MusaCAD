package com.musa.cad;

import java.io.IOException;

/**
 * Admission gate for an otherwise empty 2D drawing routed to the 3D viewer.
 * The bounding box derived from $EXTMIN/$EXTMAX is only a warning proxy:
 * it is NOT evidence that the ACIS surface or topology has been read.
 */
public final class Cad3dOnlyOpenGuard {
    private Cad3dOnlyOpenGuard(){}

    public static void requireRealGeometry(Dxf3dMesh mesh)throws IOException {
        if(mesh==null||mesh.xyz.length==0)
            throw new IOException("3B çizimde doğrulanmış köşe bulunamadı");
        if(mesh.solidProxy)
            throw new IOException("3B katı yüzeyleri çözümlenemedi; yalnız yaklaşık sınır kutusu var");
        if(mesh.triangles.length==0&&mesh.edges.length==0)
            throw new IOException("3B çizimde doğrulanmış yüzey veya kenar bulunamadı");
    }
}
