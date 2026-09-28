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
     * During an active gesture, a bitmap produced by the authoritative DXF renderer
     * may be used as a short-lived navigation cache. The exact vector renderer
     * returns after the gesture settles. Any source edit disables this cache so
     * pending edits can never disappear while panning or zooming.
     */
    public static boolean useBitmapNavigationPreview(boolean vectorReady,boolean previewAvailable,int modifiedCount){
        return previewAvailable&&modifiedCount==0;
    }
    private RenderPathPolicy(){}
}
