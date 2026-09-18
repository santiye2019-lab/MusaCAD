package com.musa.cad;

import java.util.Arrays;

/** One MusaCAD overlay/source edit stored in drawing-content coordinates. */
public final class CadEdit {
    public enum Type { LINE, POLYLINE, RECTANGLE, CIRCLE, TEXT }
    public static final int COLOR_BYLAYER=0,COLOR_BYBLOCK=1,COLOR_ACI=2,COLOR_TRUECOLOR=3;
    public final Type type;
    public final float[] xy;
    public final String text;
    public final float strokeWidth;
    public final boolean closed;
    public final float rotationDegrees;
    /** Source TEXT style metadata. textHeight==0 means a MusaCAD-added text using default display sizing. */
    public final String textStyleName,textFamilyHint;
    public final boolean textShx;
    public final float textHeight,textWidthFactor,textOblique;
    public final int textGenerationFlags;
    public final String layerName;
    public final int colorMode,colorValue;

    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees){this(type,xy,text,strokeWidth,closed,rotationDegrees,"STANDARD","sans",false,0f,1f,0f,0,"0",COLOR_BYLAYER,7);}
    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees,String textStyleName,String textFamilyHint,boolean textShx,float textHeight,float textWidthFactor,float textOblique,int textGenerationFlags){this(type,xy,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,"0",COLOR_BYLAYER,7);}
    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees,String textStyleName,String textFamilyHint,boolean textShx,float textHeight,float textWidthFactor,float textOblique,int textGenerationFlags,String layerName,int colorMode,int colorValue){
        this.type=type;this.xy=xy;this.text=text;this.strokeWidth=Math.max(1f,strokeWidth);this.closed=closed;this.rotationDegrees=normalize(rotationDegrees);
        this.textStyleName=textStyleName==null||textStyleName.trim().isEmpty()?"STANDARD":textStyleName.trim();this.textFamilyHint=textFamilyHint==null||textFamilyHint.trim().isEmpty()?"sans":textFamilyHint.trim();this.textShx=textShx;
        this.textHeight=Float.isFinite(textHeight)&&textHeight>0f?textHeight:0f;this.textWidthFactor=Float.isFinite(textWidthFactor)&&textWidthFactor>0f?textWidthFactor:1f;this.textOblique=Float.isFinite(textOblique)?textOblique:0f;this.textGenerationFlags=textGenerationFlags;
        this.layerName=layerName==null||layerName.trim().isEmpty()?"0":layerName.trim();this.colorMode=colorMode>=COLOR_BYLAYER&&colorMode<=COLOR_TRUECOLOR?colorMode:COLOR_BYLAYER;
        this.colorValue=this.colorMode==COLOR_ACI?Math.max(1,Math.min(255,colorValue)):this.colorMode==COLOR_TRUECOLOR?(colorValue&0x00FFFFFF):colorValue;
    }

    public static CadEdit line(float x1,float y1,float x2,float y2){return new CadEdit(Type.LINE,new float[]{x1,y1,x2,y2},null,3f,false,0f);}
    public static CadEdit rectangle(float x1,float y1,float x2,float y2){return new CadEdit(Type.RECTANGLE,new float[]{x1,y1,x2,y2},null,3f,true,0f);}
    public static CadEdit circle(float cx,float cy,float px,float py){return new CadEdit(Type.CIRCLE,new float[]{cx,cy,px,py},null,3f,true,0f);}
    public static CadEdit polyline(float[] xy){return polyline(xy,false);}
    public static CadEdit polyline(float[] xy,boolean closed){return new CadEdit(Type.POLYLINE,xy.clone(),null,3f,closed,0f);}
    public static CadEdit freehand(float[] xy,float strokeWidth){return new CadEdit(Type.POLYLINE,xy.clone(),null,strokeWidth,false,0f);}
    public static CadEdit text(float x,float y,String text){return text(x,y,text,0f);}
    public static CadEdit text(float x,float y,String text,float rotationDegrees){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees);}
    public static CadEdit styledText(float x,float y,String text,float rotationDegrees,String styleName,String familyHint,boolean shx,float height,float widthFactor,float oblique,int generationFlags){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees,styleName,familyHint,shx,height,widthFactor,oblique,generationFlags);}
    public boolean hasTextStyle(){return type==Type.TEXT&&textHeight>0f;}

    public CadEdit copy(){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerName,colorMode,colorValue);}
    public CadEdit translated(float dx,float dy){float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){out[i]+=dx;out[i+1]+=dy;}return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerName,colorMode,colorValue);}
    public CadEdit rotated(float degrees,float pivotX,float pivotY){double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){double x=out[i]-pivotX,y=out[i+1]-pivotY;out[i]=(float)(pivotX+x*co-y*si);out[i+1]=(float)(pivotY+x*si+y*co);}float textRotation=type==Type.TEXT?rotationDegrees+degrees:rotationDegrees;return new CadEdit(type,out,text,strokeWidth,closed,textRotation,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerName,colorMode,colorValue);}
    public CadEdit withTextHeight(float height){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,height,textWidthFactor,textOblique,textGenerationFlags,layerName,colorMode,colorValue);}
    public CadEdit withTextStyle(String styleName,String familyHint,boolean shx,float height,float widthFactor){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,styleName,familyHint,shx,height,widthFactor,textOblique,textGenerationFlags,layerName,colorMode,colorValue);}
    public CadEdit withCadProperties(String layer,int mode,int value){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layer,mode,value);}

    public float centerX(){if((type==Type.CIRCLE||type==Type.TEXT)&&xy.length>=2)return xy[0];return (minX()+maxX())*.5f;}
    public float centerY(){if((type==Type.CIRCLE||type==Type.TEXT)&&xy.length>=2)return xy[1];return (minY()+maxY())*.5f;}
    public float minX(){float v=Float.POSITIVE_INFINITY;for(int i=0;i+1<xy.length;i+=2)v=Math.min(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float maxX(){float v=Float.NEGATIVE_INFINITY;for(int i=0;i+1<xy.length;i+=2)v=Math.max(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float minY(){float v=Float.POSITIVE_INFINITY;for(int i=1;i<xy.length;i+=2)v=Math.min(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float maxY(){float v=Float.NEGATIVE_INFINITY;for(int i=1;i<xy.length;i+=2)v=Math.max(v,xy[i]);return Float.isFinite(v)?v:0f;}

    public float hitDistance(float x,float y){switch(type){case LINE:return segmentDistance(x,y,xy[0],xy[1],xy[2],xy[3]);case RECTANGLE:{float l=Math.min(xy[0],xy[2]),r=Math.max(xy[0],xy[2]),t=Math.min(xy[1],xy[3]),b=Math.max(xy[1],xy[3]);return Math.min(Math.min(segmentDistance(x,y,l,t,r,t),segmentDistance(x,y,r,t,r,b)),Math.min(segmentDistance(x,y,r,b,l,b),segmentDistance(x,y,l,b,l,t)));}case CIRCLE:{float radius=(float)Math.hypot(xy[2]-xy[0],xy[3]-xy[1]);return Math.abs((float)Math.hypot(x-xy[0],y-xy[1])-radius);}case POLYLINE:{if(xy.length<4)return Float.POSITIVE_INFINITY;float best=Float.POSITIVE_INFINITY;for(int i=2;i+1<xy.length;i+=2)best=Math.min(best,segmentDistance(x,y,xy[i-2],xy[i-1],xy[i],xy[i+1]));if(closed&&xy.length>=6)best=Math.min(best,segmentDistance(x,y,xy[xy.length-2],xy[xy.length-1],xy[0],xy[1]));return best;}case TEXT:return (float)Math.hypot(x-xy[0],y-xy[1]);default:return Float.POSITIVE_INFINITY;}}
    private static float segmentDistance(float px,float py,float ax,float ay,float bx,float by){float dx=bx-ax,dy=by-ay;float len=dx*dx+dy*dy;if(len<=1e-12f)return (float)Math.hypot(px-ax,py-ay);float t=((px-ax)*dx+(py-ay)*dy)/len;t=Math.max(0f,Math.min(1f,t));return (float)Math.hypot(px-(ax+t*dx),py-(ay+t*dy));}
    private static float normalize(float degrees){if(!Float.isFinite(degrees))return 0f;float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
}
