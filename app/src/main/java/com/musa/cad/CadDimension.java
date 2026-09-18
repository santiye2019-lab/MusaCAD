package com.musa.cad;

import java.util.*;

public final class CadDimension {
    public static List<CadEdit> aligned(float x1,float y1,float x2,float y2,float lineX,float lineY,double drawingValue,float textHeight,float arrowSize,int precision){
        float dx=x2-x1,dy=y2-y1,len=(float)Math.hypot(dx,dy);if(len<1e-6f)return Collections.emptyList();
        float ux=dx/len,uy=dy/len,nx=-uy,ny=ux,mx=(x1+x2)*.5f,my=(y1+y2)*.5f;
        float offset=(lineX-mx)*nx+(lineY-my)*ny;if(Math.abs(offset)<Math.max(arrowSize,textHeight))offset=(offset<0?-1f:1f)*Math.max(arrowSize,textHeight)*2f;
        float q1x=x1+nx*offset,q1y=y1+ny*offset,q2x=x2+nx*offset,q2y=y2+ny*offset;
        float sign=offset<0f?-1f:1f;
        return build(x1,y1,x2,y2,q1x,q1y,q2x,q2y,ux,uy,nx,ny,sign,drawingValue,textHeight,arrowSize,precision,(float)Math.toDegrees(Math.atan2(dy,dx)));
    }

    public static List<CadEdit> linear(float x1,float y1,float x2,float y2,float lineX,float lineY,double horizontalValue,double verticalValue,float textHeight,float arrowSize,int precision){
        float mx=(x1+x2)*.5f,my=(y1+y2)*.5f;boolean horizontal=Math.abs(lineY-my)>=Math.abs(lineX-mx);
        if(horizontal){
            float y=lineY;if(Math.abs(y-my)<Math.max(arrowSize,textHeight))y=my+(lineY<my?-1f:1f)*Math.max(arrowSize,textHeight)*2f;
            float sign=y<my?-1f:1f;float ux=x2>=x1?1f:-1f,uy=0f,nx=0f,ny=1f;
            return build(x1,y1,x2,y2,x1,y,x2,y,ux,uy,nx,ny,sign,horizontalValue,textHeight,arrowSize,precision,0f);
        }else{
            float x=lineX;if(Math.abs(x-mx)<Math.max(arrowSize,textHeight))x=mx+(lineX<mx?-1f:1f)*Math.max(arrowSize,textHeight)*2f;
            float sign=x<mx?-1f:1f;float ux=0f,uy=y2>=y1?1f:-1f,nx=1f,ny=0f;
            return build(x1,y1,x2,y2,x,y1,x,y2,ux,uy,nx,ny,sign,verticalValue,textHeight,arrowSize,precision,-90f);
        }
    }

    private static List<CadEdit> build(float x1,float y1,float x2,float y2,float q1x,float q1y,float q2x,float q2y,float ux,float uy,float nx,float ny,float side,double value,float textHeight,float arrowSize,int precision,float rotation){
        ArrayList<CadEdit> out=new ArrayList<>();float ext=Math.max(arrowSize*.5f,textHeight*.25f);
        out.add(CadEdit.line(x1,y1,q1x+nx*side*ext,q1y+ny*side*ext));
        out.add(CadEdit.line(x2,y2,q2x+nx*side*ext,q2y+ny*side*ext));
        out.add(CadEdit.line(q1x,q1y,q2x,q2y));
        addArrow(out,q1x,q1y,ux,uy,nx,ny,arrowSize,1f);
        addArrow(out,q2x,q2y,ux,uy,nx,ny,arrowSize,-1f);
        float tx=(q1x+q2x)*.5f+nx*side*textHeight*.65f,ty=(q1y+q2y)*.5f+ny*side*textHeight*.65f;
        out.add(CadEdit.styledText(tx,ty,format(value,precision),rotation,"STANDARD","sans",false,textHeight,1f,0f,0));
        return out;
    }

    private static void addArrow(List<CadEdit> out,float x,float y,float ux,float uy,float nx,float ny,float size,float direction){
        float backX=ux*size*direction,backY=uy*size*direction,sideX=nx*size*.35f,sideY=ny*size*.35f;
        out.add(CadEdit.line(x,y,x+backX+sideX,y+backY+sideY));
        out.add(CadEdit.line(x,y,x+backX-sideX,y+backY-sideY));
    }

    public static String format(double value,int precision){
        int p=Math.max(0,Math.min(6,precision));String text=String.format(Locale.US,"%."+p+"f",Math.abs(value));
        if(p>0){while(text.endsWith("0"))text=text.substring(0,text.length()-1);if(text.endsWith("."))text=text.substring(0,text.length()-1);}
        return text;
    }

    private CadDimension(){}
}
