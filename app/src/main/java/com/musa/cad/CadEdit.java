package com.musa.cad;

import java.util.Arrays;

/** One MusaCAD overlay/source edit stored in drawing-content coordinates. */
public final class CadEdit {
    public enum Type { LINE, POLYLINE, RECTANGLE, CIRCLE, ARC, ELLIPSE, POINT, XLINE, TEXT }
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

    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees){this(type,xy,text,strokeWidth,closed,rotationDegrees,"STANDARD","sans",false,0f,1f,0f,0);}
    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees,String textStyleName,String textFamilyHint,boolean textShx,float textHeight,float textWidthFactor,float textOblique,int textGenerationFlags){
        this.type=type;this.xy=xy;this.text=text;this.strokeWidth=Math.max(1f,strokeWidth);this.closed=closed;this.rotationDegrees=normalize(rotationDegrees);
        this.textStyleName=textStyleName==null||textStyleName.trim().isEmpty()?"STANDARD":textStyleName.trim();this.textFamilyHint=textFamilyHint==null||textFamilyHint.trim().isEmpty()?"sans":textFamilyHint.trim();this.textShx=textShx;
        this.textHeight=Float.isFinite(textHeight)&&textHeight>0f?textHeight:0f;this.textWidthFactor=Float.isFinite(textWidthFactor)&&textWidthFactor>0f?textWidthFactor:1f;this.textOblique=Float.isFinite(textOblique)?textOblique:0f;this.textGenerationFlags=textGenerationFlags;
    }

    public static CadEdit line(float x1,float y1,float x2,float y2){return new CadEdit(Type.LINE,new float[]{x1,y1,x2,y2},null,3f,false,0f);}
    public static CadEdit rectangle(float x1,float y1,float x2,float y2){return new CadEdit(Type.RECTANGLE,new float[]{x1,y1,x2,y2},null,3f,true,0f);}
    public static CadEdit circle(float cx,float cy,float px,float py){return new CadEdit(Type.CIRCLE,new float[]{cx,cy,px,py},null,3f,true,0f);}
    public static CadEdit arc(float sx,float sy,float mx,float my,float ex,float ey){
        float ax=mx-sx,ay=my-sy,bx=ex-sx,by=ey-sy;
        float d=2f*(ax*by-ay*bx);
        if(Math.abs(d)<1e-6f)return null;
        float a2=ax*ax+ay*ay,b2=bx*bx+by*by;
        float cx=sx+(by*a2-ay*b2)/d;
        float cy=sy+(ax*b2-bx*a2)/d;
        if(!Float.isFinite(cx)||!Float.isFinite(cy))return null;
        return new CadEdit(Type.ARC,new float[]{sx,sy,mx,my,ex,ey,cx,cy},null,3f,false,0f);
    }
    public static CadEdit ellipse(float cx,float cy,float majorX,float majorY,float controlX,float controlY){
        float ux=majorX-cx,uy=majorY-cy,major=(float)Math.hypot(ux,uy);
        if(major<1e-6f)return null;
        ux/=major;uy/=major;
        float vx=-uy,vy=ux;
        float signedMinor=(controlX-cx)*vx+(controlY-cy)*vy;
        float minor=Math.abs(signedMinor);
        if(minor<1e-6f)return null;
        float sign=signedMinor<0f?-1f:1f;
        return new CadEdit(Type.ELLIPSE,new float[]{cx,cy,majorX,majorY,cx+vx*minor*sign,cy+vy*minor*sign},null,3f,true,0f);
    }
    public static CadEdit point(float x,float y){return new CadEdit(Type.POINT,new float[]{x,y},null,3f,false,0f);}
    public static CadEdit xline(float x1,float y1,float x2,float y2){if(Math.hypot(x2-x1,y2-y1)<1e-6)return null;return new CadEdit(Type.XLINE,new float[]{x1,y1,x2,y2},null,3f,false,0f);}
    public static CadEdit polyline(float[] xy){return polyline(xy,false);}
    public static CadEdit polyline(float[] xy,boolean closed){return new CadEdit(Type.POLYLINE,xy.clone(),null,3f,closed,0f);}
    public static CadEdit freehand(float[] xy,float strokeWidth){return new CadEdit(Type.POLYLINE,xy.clone(),null,strokeWidth,false,0f);}
    public static CadEdit text(float x,float y,String text){return text(x,y,text,0f);}
    public static CadEdit text(float x,float y,String text,float rotationDegrees){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees);}
    public static CadEdit styledText(float x,float y,String text,float rotationDegrees,String styleName,String familyHint,boolean shx,float height,float widthFactor,float oblique,int generationFlags){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees,styleName,familyHint,shx,height,widthFactor,oblique,generationFlags);}
    public boolean hasTextStyle(){return type==Type.TEXT&&textHeight>0f;}

    public CadEdit copy(){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);}
    public CadEdit translated(float dx,float dy){float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){out[i]+=dx;out[i+1]+=dy;}return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);}
    public CadEdit rotated(float degrees,float pivotX,float pivotY){double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){double x=out[i]-pivotX,y=out[i+1]-pivotY;out[i]=(float)(pivotX+x*co-y*si);out[i+1]=(float)(pivotY+x*si+y*co);}float textRotation=type==Type.TEXT?rotationDegrees+degrees:rotationDegrees;return new CadEdit(type,out,text,strokeWidth,closed,textRotation,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);}
    public CadEdit scaled(float factor,float pivotX,float pivotY){
        if(!Float.isFinite(factor)||factor<=0f)throw new IllegalArgumentException("scale");
        float[] out=xy.clone();
        for(int i=0;i+1<out.length;i+=2){out[i]=pivotX+(out[i]-pivotX)*factor;out[i+1]=pivotY+(out[i+1]-pivotY)*factor;}
        float scaledTextHeight=type==Type.TEXT&&textHeight>0f?textHeight*factor:textHeight;
        float scaledStroke=Math.max(1f,strokeWidth*factor);
        return new CadEdit(type,out,text,scaledStroke,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,scaledTextHeight,textWidthFactor,textOblique,textGenerationFlags);
    }
    public CadEdit mirrored(boolean verticalAxis,float pivotX,float pivotY){
        float[] out=xy.clone();
        for(int i=0;i+1<out.length;i+=2){
            if(verticalAxis)out[i]=2f*pivotX-out[i];
            else out[i+1]=2f*pivotY-out[i+1];
        }
        float textRotation=rotationDegrees;
        if(type==Type.TEXT)textRotation=verticalAxis?180f-rotationDegrees:-rotationDegrees;
        return new CadEdit(type,out,text,strokeWidth,closed,textRotation,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);
    }
    public CadEdit offset(float distance){
        if(!Float.isFinite(distance)||Math.abs(distance)<1e-6f)return null;
        switch(type){
            case LINE:{
                if(xy.length<4)return null;float dx=xy[2]-xy[0],dy=xy[3]-xy[1];float len=(float)Math.hypot(dx,dy);if(len<1e-6f)return null;
                float nx=-dy/len,ny=dx/len,ox=nx*distance,oy=ny*distance;
                return new CadEdit(type,new float[]{xy[0]+ox,xy[1]+oy,xy[2]+ox,xy[3]+oy},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);
            }
            case CIRCLE:{
                if(xy.length<4)return null;float dx=xy[2]-xy[0],dy=xy[3]-xy[1];float r=(float)Math.hypot(dx,dy);float nr=r+distance;if(r<1e-6f||nr<=1e-6f)return null;
                float k=nr/r;
                return new CadEdit(type,new float[]{xy[0],xy[1],xy[0]+dx*k,xy[1]+dy*k},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);
            }
            case RECTANGLE:{
                if(xy.length<4)return null;float l=Math.min(xy[0],xy[2])-distance,r=Math.max(xy[0],xy[2])+distance,t=Math.min(xy[1],xy[3])-distance,b=Math.max(xy[1],xy[3])+distance;
                if(r-l<=1e-6f||b-t<=1e-6f)return null;
                float x1=xy[0]<=xy[2]?l:r,x2=xy[0]<=xy[2]?r:l,y1=xy[1]<=xy[3]?t:b,y2=xy[1]<=xy[3]?b:t;
                return new CadEdit(type,new float[]{x1,y1,x2,y2},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);
            }
            default:return null;
        }
    }
    public CadEdit withTextHeight(float height){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,height,textWidthFactor,textOblique,textGenerationFlags);}
    public CadEdit withClosed(boolean value){return new CadEdit(type,xy.clone(),text,strokeWidth,value,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags);}

    public float centerX(){if((type==Type.CIRCLE||type==Type.ELLIPSE||type==Type.POINT)&&xy.length>=2)return xy[0];if(type==Type.ARC&&xy.length>=8)return xy[6];if(type==Type.TEXT&&xy.length>=2)return xy[0];return (minX()+maxX())*.5f;}
    public float centerY(){if((type==Type.CIRCLE||type==Type.ELLIPSE||type==Type.POINT)&&xy.length>=2)return xy[1];if(type==Type.ARC&&xy.length>=8)return xy[7];if(type==Type.TEXT&&xy.length>=2)return xy[1];return (minY()+maxY())*.5f;}
    public float minX(){float v=Float.POSITIVE_INFINITY;for(int i=0;i+1<xy.length;i+=2)v=Math.min(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float maxX(){float v=Float.NEGATIVE_INFINITY;for(int i=0;i+1<xy.length;i+=2)v=Math.max(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float minY(){float v=Float.POSITIVE_INFINITY;for(int i=1;i<xy.length;i+=2)v=Math.min(v,xy[i]);return Float.isFinite(v)?v:0f;}
    public float maxY(){float v=Float.NEGATIVE_INFINITY;for(int i=1;i<xy.length;i+=2)v=Math.max(v,xy[i]);return Float.isFinite(v)?v:0f;}

    public float hitDistance(float x,float y){switch(type){
        case LINE:return segmentDistance(x,y,xy[0],xy[1],xy[2],xy[3]);
        case RECTANGLE:{float l=Math.min(xy[0],xy[2]),r=Math.max(xy[0],xy[2]),t=Math.min(xy[1],xy[3]),b=Math.max(xy[1],xy[3]);return Math.min(Math.min(segmentDistance(x,y,l,t,r,t),segmentDistance(x,y,r,t,r,b)),Math.min(segmentDistance(x,y,r,b,l,b),segmentDistance(x,y,l,b,l,t)));}
        case CIRCLE:{float radius=(float)Math.hypot(xy[2]-xy[0],xy[3]-xy[1]);return Math.abs((float)Math.hypot(x-xy[0],y-xy[1])-radius);}
        case ARC:{if(xy.length<8)return Float.POSITIVE_INFINITY;float radius=(float)Math.hypot(xy[0]-xy[6],xy[1]-xy[7]);float radial=Math.abs((float)Math.hypot(x-xy[6],y-xy[7])-radius);float a=angle(xy[0]-xy[6],xy[1]-xy[7]),m=angle(xy[2]-xy[6],xy[3]-xy[7]),e=angle(xy[4]-xy[6],xy[5]-xy[7]),p=angle(x-xy[6],y-xy[7]);return onArc(a,m,e,p)?radial:Math.min((float)Math.hypot(x-xy[0],y-xy[1]),(float)Math.hypot(x-xy[4],y-xy[5]));}
        case ELLIPSE:{if(xy.length<6)return Float.POSITIVE_INFINITY;float cx=xy[0],cy=xy[1],ax=xy[2]-cx,ay=xy[3]-cy,bx=xy[4]-cx,by=xy[5]-cy;float aLen=(float)Math.hypot(ax,ay),bLen=(float)Math.hypot(bx,by);if(aLen<1e-6f||bLen<1e-6f)return Float.POSITIVE_INFINITY;float ux=ax/aLen,uy=ay/aLen,vx=bx/bLen,vy=by/bLen;float dx=x-cx,dy=y-cy,localX=dx*ux+dy*uy,localY=dx*vx+dy*vy;float q=(float)Math.sqrt((localX*localX)/(aLen*aLen)+(localY*localY)/(bLen*bLen));return Math.abs(q-1f)*Math.min(aLen,bLen);}
        case POINT:return (float)Math.hypot(x-xy[0],y-xy[1]);
        case XLINE:{if(xy.length<4)return Float.POSITIVE_INFINITY;float dx=xy[2]-xy[0],dy=xy[3]-xy[1],len=(float)Math.hypot(dx,dy);if(len<1e-6f)return Float.POSITIVE_INFINITY;return Math.abs((x-xy[0])*dy-(y-xy[1])*dx)/len;}
        case POLYLINE:{if(xy.length<4)return Float.POSITIVE_INFINITY;float best=Float.POSITIVE_INFINITY;for(int i=2;i+1<xy.length;i+=2)best=Math.min(best,segmentDistance(x,y,xy[i-2],xy[i-1],xy[i],xy[i+1]));if(closed&&xy.length>=6)best=Math.min(best,segmentDistance(x,y,xy[xy.length-2],xy[xy.length-1],xy[0],xy[1]));return best;}
        case TEXT:return (float)Math.hypot(x-xy[0],y-xy[1]);default:return Float.POSITIVE_INFINITY;}}
    private static float segmentDistance(float px,float py,float ax,float ay,float bx,float by){float dx=bx-ax,dy=by-ay;float len=dx*dx+dy*dy;if(len<=1e-12f)return (float)Math.hypot(px-ax,py-ay);float t=((px-ax)*dx+(py-ay)*dy)/len;t=Math.max(0f,Math.min(1f,t));return (float)Math.hypot(px-(ax+t*dx),py-(ay+t*dy));}
    private static float normalize(float degrees){if(!Float.isFinite(degrees))return 0f;float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
}
