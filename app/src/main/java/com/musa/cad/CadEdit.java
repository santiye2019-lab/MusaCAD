package com.musa.cad;

import java.util.Arrays;

/** One MusaCAD overlay edit stored in drawing-content coordinates. */
public final class CadEdit {
    public enum Type { LINE, POLYLINE, RECTANGLE, CIRCLE, TEXT }
    public final Type type;
    public final float[] xy;
    public final String text;

    private CadEdit(Type type,float[] xy,String text){this.type=type;this.xy=xy;this.text=text;}
    public static CadEdit line(float x1,float y1,float x2,float y2){return new CadEdit(Type.LINE,new float[]{x1,y1,x2,y2},null);}
    public static CadEdit rectangle(float x1,float y1,float x2,float y2){return new CadEdit(Type.RECTANGLE,new float[]{x1,y1,x2,y2},null);}
    public static CadEdit circle(float cx,float cy,float px,float py){return new CadEdit(Type.CIRCLE,new float[]{cx,cy,px,py},null);}
    public static CadEdit polyline(float[] xy){return new CadEdit(Type.POLYLINE,xy.clone(),null);}
    public static CadEdit text(float x,float y,String text){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text);}
    public CadEdit copy(){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text);}
}
