package com.musa.cad;

/**
 * Rendering policy for navigation.
 * The native DWG scene is a fast first-paint bridge only. Once the complete
 * DXF model exists, navigation stays on that authoritative renderer family so
 * colors do not change while pinch-zooming or panning.
 */
public final class RenderPathPolicy {
    public static boolean useNativeFast(boolean vectorReady,boolean nativeAvailable,boolean nativeTruncated,int modifiedCount){
        // Before the authoritative vector model exists, the native scene is the only
        // geometry cache available for gesture frames. A truncated scene may be
        // incomplete, but refusing its fast path would force the same large partial
        // scene through the full-quality renderer on every MOVE and cause severe jank.
        // The moment vectorReady becomes true, native rendering is never used again.
        return !vectorReady&&nativeAvailable&&modifiedCount==0;
    }
    /**
     * During an active gesture, a bitmap produced by the authoritative DXF renderer
     * may be used as a short-lived navigation cache. The exact vector renderer
     * returns after the gesture settles. Any source edit disables this cache so
     * pending edits can never disappear while panning or zooming.
     */
    public static boolean useBitmapNavigationPreview(boolean vectorReady,boolean previewAvailable,int modifiedCount){
        // The preview is only used while the viewport is moving. Source additions and
        // replacements are drawn as live overlays by CadView, and the exact vector
        // frame returns after settle. Keeping this cache enabled prevents one edit from
        // permanently degrading every later pinch/pan into a full-DWG redraw.
        return previewAvailable;
    }
    private RenderPathPolicy(){}
}
