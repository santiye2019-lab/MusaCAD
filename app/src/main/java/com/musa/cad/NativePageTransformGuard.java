package com.musa.cad;

/**
 * Ensures every independently decoded DWG preview page uses one identical
 * world-to-content coordinate system. Individual pages can carry different
 * fallback extents if a malformed source DWG has no trustworthy model extents.
 * Rendering such pages together creates plausible-looking but misplaced CAD
 * geometry. Reject the page BEFORE publishing it to the compositor.
 *
 * Pure Java: exact deterministic matrix regressions run on host without
 * private DWG files or the Android graphics runtime.
 */
public final class NativePageTransformGuard {
    private static final int MATRIX_WORDS=9;
    private NativePageTransformGuard() {}

    public static void requireValid(float[] matrix) {
        if(matrix==null||matrix.length!=MATRIX_WORDS)
            throw new IllegalStateException("invalid DWG page transformation");
        for(float value:matrix)
            if(!Float.isFinite(value))
                throw new IllegalStateException("non-finite DWG page transformation");
        // NativeScene's normalization matrix is affine, not projective.
        if(matrix[6]!=0f||matrix[7]!=0f||matrix[8]!=1f)
            throw new IllegalStateException("unsupported DWG page projection");
        double determinant=(double)matrix[0]*matrix[4]-
                           (double)matrix[1]*matrix[3];
        if(!Double.isFinite(determinant)||determinant==0d)
            throw new IllegalStateException("singular DWG page transformation");
    }

    public static void requireSame(float[] expected,float[] candidate) {
        requireValid(expected);
        requireValid(candidate);
        for(int i=0;i<MATRIX_WORDS;i++)
            if(expected[i]!=candidate[i])
                throw new IllegalStateException("DWG page coordinate systems differ");
    }
}
