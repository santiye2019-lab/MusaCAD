package com.musa.cad;

/**
 * Conservative guard against misleading provisional DWG first frames.
 * It never modifies drawing colors or entities: suspect previews are simply
 * held back until the complete vector renderer is ready.
 */
public final class NativeFirstPaintColorGuard {
    public static boolean blueTint(int argb) {
        int r=(argb>>>16)&255,g=(argb>>>8)&255,b=argb&255;
        return b>=96 && b>=r*3/2 && b>=g*3/2 && b-Math.max(r,g)>=48;
    }

    public static boolean shouldDefer(int blueEntities,int totalEntities) {
        return totalEntities>=12 && blueEntities>=12
            && (long)blueEntities*100L >= (long)totalEntities*85L;
    }

    private NativeFirstPaintColorGuard() {}
}
