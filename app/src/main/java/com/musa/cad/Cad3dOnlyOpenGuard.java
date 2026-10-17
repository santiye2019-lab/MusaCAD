package com.musa.cad;

import java.io.IOException;

/**
 * Android transport for the ONE cross-platform 3D-only drawing admission
 * policy in core/native/src/musacad_3d_admission.c.
 *
 * This class performs no independent CAD geometry decisions. If the shared
 * native engine is unavailable, fail closed rather than accept a synthetic
 * ACIS proxy or maintain a second Java implementation.
 */
public final class Cad3dOnlyOpenGuard {
    private Cad3dOnlyOpenGuard(){}

    private static final boolean sharedCoreLoaded;
    static {
        boolean loaded=false;
        try {
            System.loadLibrary("musacad_dwg");
            loaded=true;
        } catch (LinkageError | SecurityException unavailable) {
            // A missing portable core cannot silently fall back to Java CAD math.
        }
        sharedCoreLoaded=loaded;
    }

    /** C status codes: 0 accept, 1 no vertices, 2 proxy, 3 no faces/edges, 4 malformed. */
    private static native int admissionNative(int xyzFloats,int triangleIndices,
                                              int edgeIndices,boolean solidProxy);

    public static void requireRealGeometry(Dxf3dMesh mesh)throws IOException {
        if(!sharedCoreLoaded)
            throw new IOException("Ortak CAD çekirdeği yüklenemedi; 3B açılış güvenli biçimde durduruldu");
        final int status;
        try {
            status=admissionNative(mesh==null||mesh.xyz==null?0:mesh.xyz.length,
                    mesh==null||mesh.triangles==null?0:mesh.triangles.length,
                    mesh==null||mesh.edges==null?0:mesh.edges.length,
                    mesh!=null&&mesh.solidProxy);
        } catch (LinkageError unavailable) {
            throw new IOException("Ortak CAD çekirdeğinde 3B kontrol işlevi bulunamadı", unavailable);
        }
        switch(status) {
            case 0: return;
            case 1: throw new IOException("3B çizimde doğrulanmış köşe bulunamadı");
            case 2: throw new IOException("3B katı yüzeyleri çözümlenemedi; yalnız yaklaşık sınır kutusu var");
            case 3: throw new IOException("3B çizimde doğrulanmış yüzey veya kenar bulunamadı");
            default: throw new IOException("3B geometrinin dizi yapısı geçersiz veya ortak çekirdek reddetti");
        }
    }
}
