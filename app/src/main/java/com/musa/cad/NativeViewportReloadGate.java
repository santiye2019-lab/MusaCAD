package com.musa.cad;

/**
 * Android-independent request suppression for opt-in DWG page loading.
 *
 * Viewport bounds alone are NOT a sufficient cache key while the decoder is
 * discovering pages. Newly decoded pages can evict visible geometry without
 * changing the user's pan/zoom coordinates. Re-request exactly once per new
 * discovery generation, without making every onDraw a reload request.
 *
 * This is a scheduling key, not a claim of DWG/viewport completeness.
 */
public final class NativeViewportReloadGate {
    private float[] previous;
    private int previousPageCount=-1;

    public synchronized boolean shouldRequest(
            float minX,float minY,float maxX,float maxY,int decodedPages) {
        if(!Float.isFinite(minX)||!Float.isFinite(minY)||
           !Float.isFinite(maxX)||!Float.isFinite(maxY)||
           maxX<minX||maxY<minY||decodedPages<0)
            throw new IllegalArgumentException("invalid DWG viewport generation");
        if(decodedPages<previousPageCount)
            throw new IllegalArgumentException("DWG page count regressed");
        if(previous!=null&&previousPageCount==decodedPages&&
           previous[0]==minX&&previous[1]==minY&&
           previous[2]==maxX&&previous[3]==maxY)
            return false;
        previous=new float[]{minX,minY,maxX,maxY};
        previousPageCount=decodedPages;
        return true;
    }
}
