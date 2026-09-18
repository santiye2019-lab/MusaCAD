package com.musa.cad;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Pure geometry helper for flattened CAD dimensions. */
public final class CadDimension {
    public static final class Style {
        public final float textHeight,arrowSize;
        public final int decimals;
        public Style(float textHeight,float arrowSize,int decimals){
            if(!Float.isFinite(textHeight)||textHeight<=0f)throw new IllegalArgumentException("textHeight");
            if(!Float.isFinite(arrowSize)||arrowSize<=0f)throw new IllegalArgumentException("arrowSize");
            this.textHeight=textHeight;this.arrowSize=arrowSize;this.decimals=Math.max(0,Math.min(6,decimals));
        }
    }

    public static List<CadEdit> linear(float x1,float y1,float x2,float y2,float px,float py,double unitsPerContentPixel,Style style){
        if(style==null)throw new IllegalArgumentException("style");
        ArrayList<CadEdit> out=new ArrayList<>();
        float dx=Math.abs(x2-x1),dy=Math.abs(y2-y1);
        if(dx<1e-6f&&dy<1e-6f)return out;
        if(dx>=dy){
            float y=py;
            out.add(CadEdit.line(x1,y1,x1,y));
            out.add(CadEdit.line(x2,y2,x2,y));
            out.add(CadEdit.line(x1,y,x2,y));
            arrows(out,x1,y,x2,y,style.arrowSize);
            float value=(float)(Math.abs(x2-x1)*safeUnits(unitsPerContentPixel));
            out.add(text((x1+x2)*.5f,y-style.textHeight*.35f,format(value,style.decimals),style.textHeight,0f));
        }else{
            float x=px;
            out.add(CadEdit.line(x1,y1,x,y1));
            out.add(CadEdit.line(x2,y2,x,y2));
            out.add(CadEdit.line(x,y1,x,y2));
            arrows(out,x,y1,x,y2,style.arrowSize);
            float value=(float)(Math.abs(y2-y1)*safeUnits(unitsPerContentPixel));
            out.add(text(x+style.textHeight*.35f,(y1+y2)*.5f,format(value,style.decimals),style.textHeight,90f));
        }
        return out;
    }

    public static List<CadEdit> aligned(float x1,float y1,float x2,float y2,float px,float py,double unitsPerContentPixel,Style style){
        if(style==null)throw new IllegalArgumentException("style");
        ArrayList<CadEdit> out=new ArrayList<>();
        float dx=x2-x1,dy=y2-y1,len=(float)Math.hypot(dx,dy);
        if(len<1e-6f)return out;
        float ux=dx/len,uy=dy/len,nx=-uy,ny=ux;
        float mx=(x1+x2)*.5f,my=(y1+y2)*.5f;
        float signed=(px-mx)*nx+(py-my)*ny;
        float ox=nx*signed,oy=ny*signed;
        float a1x=x1+ox,a1y=y1+oy,a2x=x2+ox,a2y=y2+oy;
        out.add(CadEdit.line(x1,y1,a1x,a1y));
        out.add(CadEdit.line(x2,y2,a2x,a2y));
        out.add(CadEdit.line(a1x,a1y,a2x,a2y));
        arrows(out,a1x,a1y,a2x,a2y,style.arrowSize);
        float angle=(float)Math.toDegrees(Math.atan2(dy,dx));
        float tx=(a1x+a2x)*.5f+nx*style.textHeight*.35f,ty=(a1y+a2y)*.5f+ny*style.textHeight*.35f;
        out.add(text(tx,ty,format((float)(len*safeUnits(unitsPerContentPixel)),style.decimals),style.textHeight,angle));
        return out;
    }

    private static CadEdit text(float x,float y,String value,float height,float angle){
        return CadEdit.styledText(x,y,value,angle,"STANDARD","sans",false,height,1f,0f,0);
    }

    private static void arrows(List<CadEdit> out,float x1,float y1,float x2,float y2,float size){
        float dx=x2-x1,dy=y2-y1,len=(float)Math.hypot(dx,dy);if(len<1e-6f)return;
        float ux=dx/len,uy=dy/len,nx=-uy,ny=ux,s=Math.min(size,len*.25f);
        float back1x=x1+ux*s,back1y=y1+uy*s,back2x=x2-ux*s,back2y=y2-uy*s,w=s*.45f;
        out.add(CadEdit.line(x1,y1,back1x+nx*w,back1y+ny*w));
        out.add(CadEdit.line(x1,y1,back1x-nx*w,back1y-ny*w));
        out.add(CadEdit.line(x2,y2,back2x+nx*w,back2y+ny*w));
        out.add(CadEdit.line(x2,y2,back2x-nx*w,back2y-ny*w));
    }

    private static double safeUnits(double value){return Double.isFinite(value)&&value>0d?value:1d;}
    private static String format(float value,int decimals){return String.format(Locale.US,"%."+decimals+"f",value);}
    private CadDimension(){}
}
