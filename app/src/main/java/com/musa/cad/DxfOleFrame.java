package com.musa.cad;

import java.util.*;

/** Decodes the visible DXF OLE2FRAME boundary without interpreting embedded OLE content. */
public final class DxfOleFrame {
    public static final class Result {
        public final double x1,y1,x2,y2;
        Result(double x1,double y1,double x2,double y2){this.x1=x1;this.y1=y1;this.x2=x2;this.y2=y2;}
        public boolean valid(){return Double.isFinite(x1)&&Double.isFinite(y1)&&Double.isFinite(x2)&&Double.isFinite(y2)&&Math.abs(x2-x1)+Math.abs(y2-y1)>1e-9;}
    }
    public static Result parse(List<String>a,int from,int to){
        double x1=Double.NaN,y1=Double.NaN,x2=Double.NaN,y2=Double.NaN;
        for(int i=from;i+1<to;i+=2){
            int code=intOf(a.get(i));double value;
            switch(code){
                case 10:value=d(a.get(i+1));x1=value;break;
                case 20:value=d(a.get(i+1));y1=value;break;
                case 11:value=d(a.get(i+1));x2=value;break;
                case 21:value=d(a.get(i+1));y2=value;break;
                default:break;
            }
        }
        return new Result(x1,y1,x2,y2);
    }
    private static int intOf(String s){try{return Integer.parseInt(s.trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static double d(String s){try{return Double.parseDouble(s.trim());}catch(Exception e){return Double.NaN;}}
    private DxfOleFrame(){}
}
