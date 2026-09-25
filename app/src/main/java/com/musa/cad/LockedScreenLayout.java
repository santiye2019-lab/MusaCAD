package com.musa.cad;

/** Pure aspect-fit math used by the locked full-screen intro/about/license artwork. */
public final class LockedScreenLayout {
    public static final class Size {
        public final int width,height;
        Size(int width,int height){this.width=width;this.height=height;}
    }

    public static Size fit(int rootW,int rootH,float artW,float artH){
        if(rootW<=0||rootH<=0||!Float.isFinite(artW)||!Float.isFinite(artH)||artW<=0f||artH<=0f){
            return new Size(1,1);
        }
        float scale=Math.min(rootW/artW,rootH/artH);
        int width=Math.max(1,Math.round(artW*scale));
        int height=Math.max(1,Math.round(artH*scale));
        return new Size(Math.min(rootW,width),Math.min(rootH,height));
    }

    private LockedScreenLayout(){}
}
