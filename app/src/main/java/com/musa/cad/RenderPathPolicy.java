package com.musa.cad;

/**
 * Rendering policy for navigation.
 * The native DWG scene is a fast first-paint bridge only. Once the complete
 * DXF model exists, navigation stays on that authoritative renderer family so
 * colors do not change while pinch-zooming or panning.
 */
public final class RenderPathPolicy {
    public static boolean useNativeFast(boolean vectorReady,boolean nativeAvailable,boolean nativeTruncated,int modifiedCount){
        return !vectorReady&&nativeAvailable&&!nativeTruncated&&modifiedCount==0;
    }
    /**
     * Once the authoritative vector model is ready, never fall back to its raster
     * preview during pinch/pan. This keeps glyph outlines, thin geometry and colors
     * visually identical before, during and after navigation.
     */
    public static boolean useBitmapNavigationPreview(boolean vectorReady,boolean previewAvailable,int modifiedCount){
        return !vectorReady&&previewAvailable&&modifiedCount==0;
    }
    private RenderPathPolicy(){}
}
