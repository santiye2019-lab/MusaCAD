package com.musa.cad;

import java.util.Arrays;

/** One MusaCAD overlay edit stored in drawing-content coordinates. */
public final class CadEdit {
    public enum Type { LINE, POLYLINE, RECTANGLE, CIRCLE, TEXT }
    public final Type type;
    public final float[] xy;
    public final String text;
    /** Screen-oriented overlay width multiplier. DXF export keeps geometry as a polyline. */
    public final float strokeWidth;

    private CadEdit(Type type,float[] xy,String text,float strokeWidth){
        this.type=type;this.xy=xy;this.text=text;this.strokeWidth=Math.max(1f,strokeWidth);
    }
    public static CadEdit line(float x1,float y1,float x2,float y2){return new CadEdit(Type.LINE,new float[]{x1,y1,x2,y2},null,3f);}
    public static CadEdit rectangle(float x1,float y1,float x2,float y2){return new CadEdit(Type.RECTANGLE,new float[]{x1,y1,x2,y2},null,3f);}
    public static CadEdit circle(float cx,float cy,float px,float py){return new CadEdit(Type.CIRCLE,new float[]{cx,cy,px,py},null,3f);}
    public static CadEdit polyline(float[] xy){return new CadEdit(Type.POLYLINE,xy.clone(),null,3f);}
    public static CadEdit freehand(float[] xy,float strokeWidth){return new CadEdit(Type.POLYLINE,xy.clone(),null,strokeWidth);}
    public static CadEdit text(float x,float y,String text){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f);}
    public CadEdit copy(){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text,strokeWidth);}
}
