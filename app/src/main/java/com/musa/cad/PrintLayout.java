package com.musa.cad;

/** Pure page-fit math used by Android printing without stretching CAD output. */
public final class PrintLayout {
    /** Returns x, y, width, height fitted inside the printable content rectangle. */
    public static double[] fit(int bitmapWidth,int bitmapHeight,int left,int top,int right,int bottom){
        if(bitmapWidth<=0||bitmapHeight<=0||right<=left||bottom<=top)return new double[0];
        double availableWidth=right-left,availableHeight=bottom-top;
        double scale=Math.min(availableWidth/bitmapWidth,availableHeight/bitmapHeight);
        if(!Double.isFinite(scale)||scale<=0)return new double[0];
        double width=bitmapWidth*scale,height=bitmapHeight*scale;
        double x=left+(availableWidth-width)*.5d;
        double y=top+(availableHeight-height)*.5d;
        return new double[]{x,y,width,height};
    }

    private PrintLayout(){}
}
