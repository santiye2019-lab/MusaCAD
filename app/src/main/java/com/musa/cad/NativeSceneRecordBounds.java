package com.musa.cad;

/**
 * Strict, allocation-free v6 native-scene record boundaries.
 *
 * A DWG is untrusted input even when it came through the native decoder.
 * Check counts before multiplying/allocating. Never allow an overflowing
 * polyline vertex count or a huge packed text allocation to crash Android.
 */
public final class NativeSceneRecordBounds {
    private static final int MAX_TEXT_BYTES = 4 * 1024 * 1024;
    private NativeSceneRecordBounds(){}

    public static boolean hasVertexPairs(int at,int count,int streamLength) {
        return at>=0&&at<=streamLength&&count>=0&&
               count<=(streamLength-at)/2;
    }

    /** -1 means invalid, truncated or too large to decode safely. */
    public static int packedTextWords(int bytes,int availableWords) {
        if(bytes<0||bytes>MAX_TEXT_BYTES||availableWords<0)return -1;
        int words=bytes/3+(bytes%3==0?0:1);
        return words<=availableWords?words:-1;
    }

    /** A declared primitive count must match decoded record boundaries. */
    public static boolean countMatches(int expected,int parsed) {
        return expected>0&&expected==parsed;
    }
}
