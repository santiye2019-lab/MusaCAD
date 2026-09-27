package com.musa.cad;

/** Pure navigation math shared by the Android view and vector renderer. */
public final class CadNavigationPolicy {
    public static final float MIN_RELATIVE_ZOOM=.05f;
    public static final float MAX_RELATIVE_ZOOM=8192f;
    public static final float EDGE_PAD_PIXELS=24f;

    public static float clampScale(float candidate,float current,float fitScale){
        float base=Float.isFinite(fitScale)&&fitScale>0f?fitScale:1e-6f;
        float min=Math.max(1e-6f,base*MIN_RELATIVE_ZOOM);
        float max=Math.min(1_000_000f,Math.max(base,base*MAX_RELATIVE_ZOOM));
        float value=Float.isFinite(candidate)&&candidate>0f?candidate:(Float.isFinite(current)&&current>0f?current:base);
        return Math.max(min,Math.min(max,value));
    }

    public static float cullingPadWorld(float worldPerPixel){
        return Float.isFinite(worldPerPixel)&&worldPerPixel>0f?worldPerPixel*EDGE_PAD_PIXELS:0f;
    }

    private CadNavigationPolicy(){}
}
