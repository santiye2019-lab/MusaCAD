package com.musa.cad;

/** Resolves DXF TEXT/ATTRIB alignment and MTEXT attachment into concrete transforms. */
public final class DxfTextAlign {
    public static final class Result {
        public final float x,y,angleDegrees,widthScale,heightScale,localOffsetX,localOffsetY;
        Result(float x,float y,float angleDegrees,float widthScale,float heightScale,float localOffsetX,float localOffsetY){
            this.x=x;this.y=y;this.angleDegrees=normalize(angleDegrees);this.widthScale=safeScale(widthScale);this.heightScale=safeScale(heightScale);this.localOffsetX=localOffsetX;this.localOffsetY=localOffsetY;
        }
    }

    /**
     * DXF horizontal alignment: 0 left, 1 center, 2 right, 3 aligned, 4 middle, 5 fit.
     * Vertical alignment: 0 baseline, 1 bottom, 2 middle, 3 top.
     * For non-default alignment the second alignment point (11/21) is authoritative.
     */
    public static Result resolve(float x,float y,float x2,float y2,boolean hasSecond,int horizontal,int vertical,
                                 float rotationDegrees,float measuredWidth,float textHeight){
        float width=Math.max(.0001f,Math.abs(measuredWidth)),height=Math.max(.0001f,Math.abs(textHeight));
        int h=Math.max(0,Math.min(5,horizontal)),v=Math.max(0,Math.min(3,vertical));
        if((h==3||h==5)&&hasSecond){
            float dx=x2-x,dy=y2-y,distance=(float)Math.hypot(dx,dy);
            if(distance>.0001f){
                float angle=(float)Math.toDegrees(Math.atan2(dy,dx));
                float scale=distance/width;
                return new Result(x,y,angle,scale,h==3?scale:1f,0f,0f);
            }
        }
        float anchorX=(h!=0||v!=0)&&hasSecond?x2:x;
        float anchorY=(h!=0||v!=0)&&hasSecond?y2:y;
        float ox=0f,oy=0f;
        if(h==1||h==4)ox=-width*.5f;else if(h==2)ox=-width;
        if(h==4)oy=-height*.5f;
        else if(v==1)oy=0f;else if(v==2)oy=-height*.5f;else if(v==3)oy=-height;
        return new Result(anchorX,anchorY,rotationDegrees,1f,1f,ox,oy);
    }

    /**
     * Returns the local translation needed to place an already world-oriented MTEXT path's
     * requested attachment point at the insertion origin. Bounds use mathematical/world Y,
     * so the visual top is maxY (the RectF bottom value after Android path bounds calculation).
     */
    public static float[] mtextOffset(int attachment,float left,float top,float right,float bottom){
        int a=attachment>=1&&attachment<=9?attachment:1;int column=(a-1)%3,row=(a-1)/3;
        float anchorX=column==0?left:column==1?(left+right)*.5f:right;
        float anchorY=row==0?bottom:row==1?(top+bottom)*.5f:top;
        return new float[]{-anchorX,-anchorY};
    }

    private static float safeScale(float value){return Float.isFinite(value)&&value>0f?value:1f;}
    private static float normalize(float degrees){if(!Float.isFinite(degrees))return 0f;float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
    private DxfTextAlign(){}
}
