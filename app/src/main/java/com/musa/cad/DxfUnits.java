package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;

/** Explicit modelspace INSUNITS only; never infer a physical unit from formatting. */
public final class DxfUnits {
    public static int read(File file,Charset charset)throws IOException {
        try(DxfStream input=new DxfStream(file,charset)){
            DxfBlocks.Record r;
            while((r=input.next())!=null){
                if(!r.type.equals("SECTION"))continue;
                if(!r.text(2,"").equals("HEADER"))return 0;
                String variable="";
                for(int i=r.from;i+1<r.to;i+=2){
                    int code=Integer.parseInt(r.tags.get(i));String value=r.tags.get(i+1).trim();
                    if(code==9)variable=value;
                    else if(code==70&&variable.equals("$INSUNITS")){
                        try{return Integer.parseInt(value);}catch(NumberFormatException e){return 0;}
                    }
                }
                return 0;
            }
            return 0;
        }
    }
    public static double metersPerUnit(int code){
        switch(code){
            case 1:return .0254;case 2:return .3048;case 3:return 1609.344;
            case 4:return .001;case 5:return .01;case 6:return 1;case 7:return 1000;
            case 10:return .9144;case 14:return .1;
            default:return Double.NaN;
        }
    }
    public static String label(int code){
        switch(code){
            case 1:return "inç";case 2:return "ft";case 3:return "mil";
            case 4:return "mm";case 5:return "cm";case 6:return "m";case 7:return "km";
            case 10:return "yd";case 14:return "dm";default:return "belirsiz";
        }
    }
    public static double metersPerPixel(int code,double pixelsPerUnit){
        if(!Double.isFinite(pixelsPerUnit)||pixelsPerUnit<=0)return Double.NaN;
        return metersPerUnit(code)/pixelsPerUnit;
    }
    private DxfUnits(){}
}
