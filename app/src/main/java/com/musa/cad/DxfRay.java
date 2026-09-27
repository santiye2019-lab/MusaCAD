package com.musa.cad;

import java.util.List;

/** Parses the 2D display geometry of an AutoCAD DXF RAY entity. */
public final class DxfRay {
    public static final class Data {
        public final double x,y,dx,dy;
        Data(double x,double y,double dx,double dy){this.x=x;this.y=y;this.dx=dx;this.dy=dy;}
    }

    public static Data parse(List<String> tags,int from,int to){
        if(tags==null||from<0||to>tags.size()||from>to)return null;
        double x=Double.NaN,y=Double.NaN,dx=Double.NaN,dy=Double.NaN;
        try{
            for(int i=from;i+1<to;i+=2){
                int code=Integer.parseInt(tags.get(i).trim());
                double value;
                if(code==10||code==20||code==11||code==21)value=Double.parseDouble(tags.get(i+1).trim());else continue;
                if(!Double.isFinite(value))return null;
                if(code==10)x=value;else if(code==20)y=value;else if(code==11)dx=value;else dy=value;
            }
        }catch(RuntimeException e){return null;}
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(dx)||!Double.isFinite(dy))return null;
        double length=Math.hypot(dx,dy);if(!Double.isFinite(length)||length<1e-12)return null;
        return new Data(x,y,dx/length,dy/length);
    }

    private DxfRay(){}
}
