package com.musa.cad;

import java.io.IOException;

/** Android is a transport-only adapter to the ONE first-party C core.
 * No Java conversion/geometry code, no downgrade with omitted entities.
 */
public final class Mc1032Migration {
    private static final boolean loaded;
    static {
        boolean ok=false;
        try{System.loadLibrary("musacad_dwg");ok=true;}
        catch(LinkageError | SecurityException ignored){}
        loaded=ok;
    }
    private static native byte[] upgradeNative(byte[] v1) throws IOException;
    private static native byte[] downgradeNative(byte[] v2) throws IOException;

    /** Losslessly upgrades supported v1 POINT/LINE to layered 3D v2, Z=0. */
    public static byte[] upgrade(byte[] v1)throws IOException{
        if(!loaded)throw new IOException("MC1032 ortak CAD cekirdegi yok");
        if(v1==null||v1.length<32||v1.length>Mc1032Native.MAX_BYTES)
            throw new IOException("MC1032 v1 kaynak boyutu gecersiz");
        try{return upgradeNative(v1);}
        catch(LinkageError missing){throw new IOException("MC1032 ortak gecis JNI yok",missing);}
    }
    /** Refuses to discard nonzero Z or non-default layer information. */
    public static byte[] downgrade(byte[] v2)throws IOException{
        if(!loaded)throw new IOException("MC1032 ortak CAD cekirdegi yok");
        if(v2==null||v2.length<48||v2.length>Mc1032V2Native.MAX_BYTES)
            throw new IOException("MC1032 v2 kaynak boyutu gecersiz");
        try{return downgradeNative(v2);}
        catch(LinkageError missing){throw new IOException("MC1032 ortak gecis JNI yok",missing);}
    }
    private Mc1032Migration(){}
}
