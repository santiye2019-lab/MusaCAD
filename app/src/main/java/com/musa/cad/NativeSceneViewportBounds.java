package com.musa.cad;

/**
 * Validated viewport framing for a native v6 drawing's original XY bounds.
 * A real DWG with a single POINT, or exclusively vertical/horizontal LINE,
 * has a zero-width or zero-height geometric extent. This is VALID input:
 * pad ONLY the CAMERA bounds, never the source geometry/coordinates.
 *
 * Pure Java used by Android and checked by ordinary host JVM tests.
 */
public final class NativeSceneViewportBounds {
    private NativeSceneViewportBounds() {}

    /**
     * [minX,minY,maxX,maxY] for the Android viewport or null when invalid.
     * Original source XY bounds are never modified. No unbounded allocations.
     */
    public static float[] forScene(float minX,float minY,float maxX,float maxY) {
        if (!Float.isFinite(minX) || !Float.isFinite(minY) ||
            !Float.isFinite(maxX) || !Float.isFinite(maxY) ||
            maxX < minX || maxY < minY)
            return null;
        if (minX == maxX) {
            final float padding=padding(minX,maxY-minY);
            minX -= padding;
            maxX += padding;
        }
        if (minY == maxY) {
            final float padding=padding(minY,maxX-minX);
            minY -= padding;
            maxY += padding;
        }
        if (!Float.isFinite(minX) || !Float.isFinite(minY) ||
            !Float.isFinite(maxX) || !Float.isFinite(maxY) ||
            maxX <= minX || maxY <= minY)
            return null;
        return new float[]{minX,minY,maxX,maxY};
    }

    private static float padding(float center,float otherSpan) {
        /* Use a reproducible minimum screen framing of 1 drawing unit,
         * expand if the coordinate magnitude's float ULP is enormous. */
        final double pad=Math.max(1.0,
               Math.max((double)Math.ulp(center)*16.0,
                        (double)otherSpan*0.001));
        return pad<=Float.MAX_VALUE?(float)pad:Float.POSITIVE_INFINITY;
    }
}
