package com.musa.cad;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * Byte-precise fingerprint of a single decoded native DWG scene stream.
 *
 * The native page stream is an ordered sequence of IEEE-754 float32 words.
 * Hash the exact raw bits (not Float.toString or canonicalized float values).
 * A reloaded page with unchanged cursor/bounds/count but changed geometry or
 * color must NEVER be mistaken for its original cached content.
 *
 * Runs on the off-UI decoding worker only. Reuses an 8-KiB hash buffer;
 * no additional full-copy of multi-megabyte geometry is created.
 */
public final class NativeSceneFingerprint {
    private NativeSceneFingerprint(){}

    public static byte[] sha256(float[] words){
        if(words==null||words.length==0)
            throw new IllegalArgumentException("native scene stream missing");
        MessageDigest digest;
        try{digest=MessageDigest.getInstance("SHA-256");}
        catch(NoSuchAlgorithmException missing){
            throw new IllegalStateException("SHA-256 is not available",missing);
        }
        final byte[] buffer=new byte[8192];
        int at=0;
        for(float number:words){
            int bits=Float.floatToRawIntBits(number);
            if(at+4>buffer.length){
                digest.update(buffer,0,at);
                at=0;
            }
            buffer[at++]=(byte)(bits>>>24);
            buffer[at++]=(byte)(bits>>>16);
            buffer[at++]=(byte)(bits>>>8);
            buffer[at++]=(byte)bits;
        }
        if(at>0)digest.update(buffer,0,at);
        return digest.digest();
    }

    /** Constant-time comparison with no short circuit on changed bytes. */
    public static boolean matches(byte[] expected,byte[] actual){
        return expected!=null&&actual!=null&&
                expected.length==32&&actual.length==32&&
                MessageDigest.isEqual(expected,actual);
    }
}
