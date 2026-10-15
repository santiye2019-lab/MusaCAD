package com.musa.cad;

import java.util.Arrays;

/** One MusaCAD overlay/source edit stored in drawing-content coordinates. */
public final class CadEdit {
    public enum Type { LINE, POLYLINE, RECTANGLE, CIRCLE, ARC, ELLIPSE, POINT, XLINE, HATCH, INSERT, TEXT }
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
    /** Optional per-entity layer override used by AI/imported additions; null uses the project default layer. */
    public final String layerOverride;

    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees){this(type,xy,text,strokeWidth,closed,rotationDegrees,"STANDARD","sans",false,0f,1f,0f,0);}
    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees,String textStyleName,String textFamilyHint,boolean textShx,float textHeight,float textWidthFactor,float textOblique,int textGenerationFlags){
        this(type,xy,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,null);
    }
    private CadEdit(Type type,float[] xy,String text,float strokeWidth,boolean closed,float rotationDegrees,String textStyleName,String textFamilyHint,boolean textShx,float textHeight,float textWidthFactor,float textOblique,int textGenerationFlags,String layerOverride){
        this.type=type;this.xy=xy;this.text=text;this.strokeWidth=Math.max(1f,strokeWidth);this.closed=closed;this.rotationDegrees=normalize(rotationDegrees);
        this.textStyleName=textStyleName==null||textStyleName.trim().isEmpty()?"STANDARD":textStyleName.trim();this.textFamilyHint=textFamilyHint==null||textFamilyHint.trim().isEmpty()?"sans":textFamilyHint.trim();this.textShx=textShx;
        this.textHeight=Float.isFinite(textHeight)&&textHeight>0f?textHeight:0f;this.textWidthFactor=Float.isFinite(textWidthFactor)&&textWidthFactor>0f?textWidthFactor:1f;this.textOblique=Float.isFinite(textOblique)?textOblique:0f;this.textGenerationFlags=textGenerationFlags;
        String cleanLayer=layerOverride==null?"":layerOverride.trim();
        this.layerOverride=cleanLayer.isEmpty()?null:cleanLayer;
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
    public static CadEdit hatch(float[] xy,String pattern,float angleDegrees,float patternScale){
        if(xy==null||xy.length<6||xy.length%2!=0)return null;
        String p=pattern==null?"SOLID":pattern.trim().toUpperCase(java.util.Locale.ROOT);
        if(!"ANSI31".equals(p))p="SOLID";
        float scale=Float.isFinite(patternScale)&&patternScale>0f?patternScale:1f;
        return new CadEdit(Type.HATCH,xy.clone(),p,1f,true,angleDegrees,"STANDARD","sans",false,scale,1f,0f,0);
    }
    public static CadEdit freehand(float[] xy,float strokeWidth){return new CadEdit(Type.POLYLINE,xy.clone(),null,strokeWidth,false,0f);}
    public static CadEdit text(float x,float y,String text){return text(x,y,text,0f);}
    public static CadEdit text(float x,float y,String text,float rotationDegrees){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees);}
    public static CadEdit styledText(float x,float y,String text,float rotationDegrees,String styleName,String familyHint,boolean shx,float height,float widthFactor,float oblique,int generationFlags){return new CadEdit(Type.TEXT,new float[]{x,y},text==null?"":text,3f,false,rotationDegrees,styleName,familyHint,shx,height,widthFactor,oblique,generationFlags);}
    public static CadEdit insert(String blockName,float x,float y,float scale,float rotationDegrees){
        String name=normalizeBlockName(blockName);if(name.isEmpty()||!Float.isFinite(scale)||scale<=0f)return null;
        return new CadEdit(Type.INSERT,new float[]{x,y},name,3f,false,rotationDegrees,"STANDARD","sans",false,scale,1f,0f,0);
    }
    public float insertScale(){return type==Type.INSERT&&textHeight>0f?textHeight:1f;}
    public boolean hasTextStyle(){return type==Type.TEXT&&textHeight>0f;}

    public CadEdit copy(){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public CadEdit withLayerOverride(String layer){return new CadEdit(type,Arrays.copyOf(xy,xy.length),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layer);}
    public CadEdit translated(float dx,float dy){float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){out[i]+=dx;out[i+1]+=dy;}return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public CadEdit rotated(float degrees,float pivotX,float pivotY){double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);float[] out=xy.clone();for(int i=0;i+1<out.length;i+=2){double x=out[i]-pivotX,y=out[i+1]-pivotY;out[i]=(float)(pivotX+x*co-y*si);out[i+1]=(float)(pivotY+x*si+y*co);}float textRotation=(type==Type.TEXT||type==Type.INSERT)?rotationDegrees+degrees:rotationDegrees;return new CadEdit(type,out,text,strokeWidth,closed,textRotation,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public CadEdit scaled(float factor,float pivotX,float pivotY){
        if(!Float.isFinite(factor)||factor<=0f)throw new IllegalArgumentException("scale");
        float[] out=xy.clone();
        for(int i=0;i+1<out.length;i+=2){out[i]=pivotX+(out[i]-pivotX)*factor;out[i+1]=pivotY+(out[i+1]-pivotY)*factor;}
        float scaledTextHeight=(type==Type.TEXT||type==Type.INSERT)&&textHeight>0f?textHeight*factor:textHeight;
        float scaledStroke=Math.max(1f,strokeWidth*factor);
        return new CadEdit(type,out,text,scaledStroke,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,scaledTextHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
    }
    public CadEdit mirrored(boolean verticalAxis,float pivotX,float pivotY){
        float[] out=xy.clone();
        for(int i=0;i+1<out.length;i+=2){
            if(verticalAxis)out[i]=2f*pivotX-out[i];
            else out[i+1]=2f*pivotY-out[i+1];
        }
        float textRotation=rotationDegrees;
        if(type==Type.TEXT||type==Type.INSERT)textRotation=verticalAxis?180f-rotationDegrees:-rotationDegrees;
        return new CadEdit(type,out,text,strokeWidth,closed,textRotation,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
    }
    public CadEdit offset(float distance){
        if(!Float.isFinite(distance)||Math.abs(distance)<1e-6f)return null;
        switch(type){
            case LINE:{
                if(xy.length<4)return null;float dx=xy[2]-xy[0],dy=xy[3]-xy[1];float len=(float)Math.hypot(dx,dy);if(len<1e-6f)return null;
                float nx=-dy/len,ny=dx/len,ox=nx*distance,oy=ny*distance;
                return new CadEdit(type,new float[]{xy[0]+ox,xy[1]+oy,xy[2]+ox,xy[3]+oy},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
            }
            case CIRCLE:{
                if(xy.length<4)return null;float dx=xy[2]-xy[0],dy=xy[3]-xy[1];float r=(float)Math.hypot(dx,dy);float nr=r+distance;if(r<1e-6f||nr<=1e-6f)return null;
                float k=nr/r;
                return new CadEdit(type,new float[]{xy[0],xy[1],xy[0]+dx*k,xy[1]+dy*k},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
            }
            case ARC:{
                // Keep the source arc's center and angular sweep. Offset is
                // concentric: positive expands radius; negative contracts it.
                // Rebuild all three points, never flatten an arc to a chord.
                if(xy.length<8)return null;
                double cx=xy[6],cy=xy[7];
                double radius=Math.hypot((double)xy[0]-cx,(double)xy[1]-cy);
                double newRadius=radius+distance;
                if(!(radius>1e-6d)||!(newRadius>1e-6d)||
                        !Double.isFinite(radius)||!Double.isFinite(newRadius))
                    return null;
                double ratio=newRadius/radius;
                float[] out=xy.clone();
                for(int i=0;i<6;i+=2){
                    double x=cx+((double)xy[i]-cx)*ratio;
                    double y=cy+((double)xy[i+1]-cy)*ratio;
                    if(!Double.isFinite(x)||!Double.isFinite(y)||
                            Math.abs(x)>1e9d||Math.abs(y)>1e9d)return null;
                    out[i]=(float)x;out[i+1]=(float)y;
                }
                return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,
                        textStyleName,textFamilyHint,textShx,textHeight,
                        textWidthFactor,textOblique,textGenerationFlags,layerOverride);
            }
            case POLYLINE:{
                float[] out=offsetPolyline(xy,closed,distance);
                if(out==null)return null;
                return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,
                        textStyleName,textFamilyHint,textShx,textHeight,
                        textWidthFactor,textOblique,textGenerationFlags,layerOverride);
            }
            case RECTANGLE:{
                if(xy.length<4)return null;float l=Math.min(xy[0],xy[2])-distance,r=Math.max(xy[0],xy[2])+distance,t=Math.min(xy[1],xy[3])-distance,b=Math.max(xy[1],xy[3])+distance;
                if(r-l<=1e-6f||b-t<=1e-6f)return null;
                float x1=xy[0]<=xy[2]?l:r,x2=xy[0]<=xy[2]?r:l,y1=xy[1]<=xy[3]?t:b,y2=xy[1]<=xy[3]?b:t;
                return new CadEdit(type,new float[]{x1,y1,x2,y2},text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
            }
            default:return null;
        }
    }

    /**
     * Conservative straight-segment POLYLINE offset.
     * Positive distance offsets to the left of traversal; negative to right.
     * Returns null rather than inventing intersections when an offset would
     * reverse edges, self-cross, collapse a corner, or exceed the miter bound.
     * Closed contours are limited to strictly convex simple polygons.
     * Bulge arcs and complex concave contour offset require a curve solver.
     */
    private static float[] offsetPolyline(float[] points,boolean closed,double distance){
        if(points==null||points.length<4||(points.length&1)!=0||!Double.isFinite(distance)||
                Math.abs(distance)<1e-6)return null;
        int n=points.length/2;
        if(n>2048||closed&&n<3)return null; // keep interactive O(n²) safety audit bounded
        int segs=closed?n:n-1;
        double[] ux=new double[segs],uy=new double[segs],nx=new double[segs],ny=new double[segs];
        for(int i=0;i<n;i++){
            if(!Float.isFinite(points[2*i])||!Float.isFinite(points[2*i+1])||
                    Math.abs((double)points[2*i])>1e9||Math.abs((double)points[2*i+1])>1e9)return null;
        }
        for(int i=0;i<segs;i++){
            int j=(i+1)%n;
            double dx=(double)points[2*j]-points[2*i],dy=(double)points[2*j+1]-points[2*i+1];
            double len=Math.hypot(dx,dy);
            if(!(len>1e-6)||!Double.isFinite(len))return null;
            ux[i]=dx/len;uy[i]=dy/len;nx[i]=-uy[i];ny[i]=ux[i];
        }
        if(polylineSelfCrosses(points,closed))return null;
        if(closed){
            double turnSum=0d,turnSign=0d;
            for(int i=0;i<n;i++){
                int prev=(i+n-1)%n,next=i;
                double cross=ux[prev]*uy[next]-uy[prev]*ux[next];
                double dot=ux[prev]*ux[next]+uy[prev]*uy[next];
                if(Math.abs(cross)<1e-8)return null; // strictly convex, no collinear turns
                if(turnSign==0d)turnSign=Math.signum(cross);
                else if(cross*turnSign<=0d)return null;
                turnSum+=Math.atan2(cross,dot);
            }
            if(Math.abs(Math.abs(turnSum)-2*Math.PI)>1e-5)return null;
        }
        float[] result=new float[points.length];
        for(int i=0;i<n;i++){
            double x=points[2*i],y=points[2*i+1],tx,ty;
            if(!closed&&i==0){
                tx=x+nx[0]*distance;ty=y+ny[0]*distance;
            }else if(!closed&&i==n-1){
                tx=x+nx[segs-1]*distance;ty=y+ny[segs-1]*distance;
            }else{
                int p=(i+segs-1)%segs,q=i%segs;
                double cross=ux[p]*uy[q]-uy[p]*ux[q];
                double dot=ux[p]*ux[q]+uy[p]*uy[q];
                if(Math.abs(cross)<1e-8){
                    if(dot<=0d)return null; // 180-degree reversal
                    tx=x+nx[p]*distance;ty=y+ny[p]*distance;
                }else{
                    double deltaX=(nx[q]-nx[p])*distance;
                    double deltaY=(ny[q]-ny[p])*distance;
                    double along=(deltaX*uy[q]-deltaY*ux[q])/cross;
                    tx=x+nx[p]*distance+ux[p]*along;
                    ty=y+ny[p]*distance+uy[p]*along;
                    if(Math.hypot(tx-x,ty-y)>Math.abs(distance)*8d)return null;
                }
            }
            if(!Double.isFinite(tx)||!Double.isFinite(ty)||Math.abs(tx)>1e9||Math.abs(ty)>1e9)return null;
            result[2*i]=(float)tx;result[2*i+1]=(float)ty;
        }
        // Prevent collapsed/reversed edges or a translated offset loop.
        boolean changed=false;
        for(int i=0;i<n;i++)if(result[2*i]!=points[2*i]||result[2*i+1]!=points[2*i+1])changed=true;
        if(!changed||polylineSelfCrosses(result,closed))return null;
        for(int i=0;i<segs;i++){
            int j=(i+1)%n;
            double vx=(double)result[2*j]-result[2*i],vy=(double)result[2*j+1]-result[2*i+1];
            if(Math.hypot(vx,vy)<=1e-6||vx*ux[i]+vy*uy[i]<=1e-6)return null;
        }
        return result;
    }

    private static boolean polylineSelfCrosses(float[] xy,boolean closed){
        int n=xy.length/2,segs=closed?n:n-1;
        for(int i=0;i<segs;i++){
            int a=(i+1)%n;
            for(int j=i+1;j<segs;j++){
                if(j==i+1||closed&&i==0&&j==segs-1)continue;
                int b=(j+1)%n;
                if(segmentsTouch(xy[2*i],xy[2*i+1],xy[2*a],xy[2*a+1],
                        xy[2*j],xy[2*j+1],xy[2*b],xy[2*b+1]))return true;
            }
        }
        return false;
    }

    private static boolean segmentsTouch(double ax,double ay,double bx,double by,
                                         double cx,double cy,double dx,double dy){
        if(Math.max(ax,bx)<Math.min(cx,dx)||Math.max(cx,dx)<Math.min(ax,bx)||
                Math.max(ay,by)<Math.min(cy,dy)||Math.max(cy,dy)<Math.min(ay,by))return false;
        double abx=bx-ax,aby=by-ay,cdx=dx-cx,cdy=dy-cy;
        double c1=abx*(cy-ay)-aby*(cx-ax),c2=abx*(dy-ay)-aby*(dx-ax);
        double c3=cdx*(ay-cy)-cdy*(ax-cx),c4=cdx*(by-cy)-cdy*(bx-cx);
        if(((c1>0&&c2<0)||(c1<0&&c2>0))&&
           ((c3>0&&c4<0)||(c3<0&&c4>0)))return true;
        return (c1==0&&cx>=Math.min(ax,bx)&&cx<=Math.max(ax,bx)&&cy>=Math.min(ay,by)&&cy<=Math.max(ay,by))||
               (c2==0&&dx>=Math.min(ax,bx)&&dx<=Math.max(ax,bx)&&dy>=Math.min(ay,by)&&dy<=Math.max(ay,by))||
               (c3==0&&ax>=Math.min(cx,dx)&&ax<=Math.max(cx,dx)&&ay>=Math.min(cy,dy)&&ay<=Math.max(cy,dy))||
               (c4==0&&bx>=Math.min(cx,dx)&&bx<=Math.max(cx,dx)&&by>=Math.min(cy,dy)&&by<=Math.max(cy,dy));
    }

    public CadEdit withTextHeight(float height){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,height,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public CadEdit withTextStyle(String styleName,String familyHint,boolean shx,float height){return new CadEdit(type,xy.clone(),text,strokeWidth,closed,rotationDegrees,styleName,familyHint,shx,height,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public CadEdit withClosed(boolean value){return new CadEdit(type,xy.clone(),text,strokeWidth,value,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);}
    public int stretchVertexCount(){
        if(type==Type.LINE&&xy.length>=4)return 2;
        if(type==Type.POLYLINE&&xy.length>=4)return xy.length/2;
        if(type==Type.RECTANGLE&&xy.length>=4)return 4;
        return 0;
    }
    public float stretchVertexX(int index){
        if(type==Type.RECTANGLE&&xy.length>=4){switch(index){case 0:return xy[0];case 1:return xy[2];case 2:return xy[2];case 3:return xy[0];default:return Float.NaN;}}
        int p=index*2;return p>=0&&p<xy.length?xy[p]:Float.NaN;
    }
    public float stretchVertexY(int index){
        if(type==Type.RECTANGLE&&xy.length>=4){switch(index){case 0:return xy[1];case 1:return xy[1];case 2:return xy[3];case 3:return xy[3];default:return Float.NaN;}}
        int p=index*2+1;return p>=1&&p<xy.length?xy[p]:Float.NaN;
    }
    public CadEdit stretchedVertex(int index,float x,float y){
        if(!Float.isFinite(x)||!Float.isFinite(y))return null;
        if(type==Type.LINE||type==Type.POLYLINE){
            int p=index*2;if(p<0||p+1>=xy.length)return null;float[] out=xy.clone();out[p]=x;out[p+1]=y;
            return new CadEdit(type,out,text,strokeWidth,closed,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
        }
        if(type==Type.RECTANGLE&&xy.length>=4){
            float[] out=xy.clone();
            switch(index){
                case 0:out[0]=x;out[1]=y;break;
                case 1:out[2]=x;out[1]=y;break;
                case 2:out[2]=x;out[3]=y;break;
                case 3:out[0]=x;out[3]=y;break;
                default:return null;
            }
            return new CadEdit(type,out,text,strokeWidth,true,rotationDegrees,textStyleName,textFamilyHint,textShx,textHeight,textWidthFactor,textOblique,textGenerationFlags,layerOverride);
        }
        return null;
    }

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
        case INSERT:return xy.length>=2?(float)Math.hypot(x-xy[0],y-xy[1]):Float.POSITIVE_INFINITY;
        case XLINE:{if(xy.length<4)return Float.POSITIVE_INFINITY;float dx=xy[2]-xy[0],dy=xy[3]-xy[1],len=(float)Math.hypot(dx,dy);if(len<1e-6f)return Float.POSITIVE_INFINITY;return Math.abs((x-xy[0])*dy-(y-xy[1])*dx)/len;}
        case POLYLINE:{if(xy.length<4)return Float.POSITIVE_INFINITY;float best=Float.POSITIVE_INFINITY;for(int i=2;i+1<xy.length;i+=2)best=Math.min(best,segmentDistance(x,y,xy[i-2],xy[i-1],xy[i],xy[i+1]));if(closed&&xy.length>=6)best=Math.min(best,segmentDistance(x,y,xy[xy.length-2],xy[xy.length-1],xy[0],xy[1]));return best;}
        case HATCH:{if(xy.length<6)return Float.POSITIVE_INFINITY;float best=Float.POSITIVE_INFINITY;for(int i=2;i+1<xy.length;i+=2)best=Math.min(best,segmentDistance(x,y,xy[i-2],xy[i-1],xy[i],xy[i+1]));best=Math.min(best,segmentDistance(x,y,xy[xy.length-2],xy[xy.length-1],xy[0],xy[1]));return best;}
        case TEXT:return (float)Math.hypot(x-xy[0],y-xy[1]);default:return Float.POSITIVE_INFINITY;}}
    private static float angle(float x,float y){float a=(float)Math.toDegrees(Math.atan2(y,x));return a<0f?a+360f:a;}
    private static float ccw(float from,float to){float d=to-from;while(d<0f)d+=360f;while(d>=360f)d-=360f;return d;}
    private static boolean onArc(float start,float mid,float end,float point){
        float se=ccw(start,end),sm=ccw(start,mid);
        if(sm<=se+1e-4f)return ccw(start,point)<=se+1e-4f;
        return ccw(point,start)<=ccw(end,start)+1e-4f;
    }
    private static float segmentDistance(float px,float py,float ax,float ay,float bx,float by){float dx=bx-ax,dy=by-ay;float len=dx*dx+dy*dy;if(len<=1e-12f)return (float)Math.hypot(px-ax,py-ay);float t=((px-ax)*dx+(py-ay)*dy)/len;t=Math.max(0f,Math.min(1f,t));return (float)Math.hypot(px-(ax+t*dx),py-(ay+t*dy));}
    private static String normalizeBlockName(String name){return name==null?"":name.trim().replaceAll("[^A-Za-z0-9_\\-]","_");}
    private static float normalize(float degrees){if(!Float.isFinite(degrees))return 0f;float v=degrees%360f;if(v<=-180f)v+=360f;if(v>180f)v-=360f;return v;}
}
