package com.musa.cad;

import java.util.ArrayList;

/** Geometry-independent helpers for DXF geometric polyline widths. */
public final class DxfPolylineWidth {
    public static double constant(double raw){return Double.isFinite(raw)?Math.abs(raw):0d;}

    /** Returns a constant width only when start/end widths describe the same geometric width. */
    public static double uniform(double start,double end){
        double a=constant(start),b=constant(end);
        if(a<=1e-12&&b<=1e-12)return 0d;
        if(a<=1e-12||b<=1e-12)return 0d;
        double tolerance=Math.max(1e-9,Math.max(a,b)*1e-6);
        return Math.abs(a-b)<=tolerance?(a+b)*.5:0d;
    }

    public static double device(double width,double axisScaleX,double axisScaleY){
        width=constant(width);if(width<=0)return 0;
        double sx=Math.abs(axisScaleX),sy=Math.abs(axisScaleY),scale=(sx+sy)*.5;
        if(!Double.isFinite(scale)||scale<=1e-12)return 0;
        return Math.max(.5,Math.min(10000d,width*scale));
    }

    /**
     * Builds filled outlines for DXF per-segment start/end widths. Each original segment
     * produces one strip polygon; small join triangles close the wedges between adjacent
     * segments. Bulge arcs are sampled with the same angular tolerance as the centerline
     * renderer. The result is geometry in drawing units, so INSERT/OCS/non-uniform affine
     * transforms deform the width together with the entity instead of approximating it as
     * a device-space stroke.
     */
    public static double[][] variable(double[] xs,double[] ys,double[] bulges,double[] startWidths,double[] endWidths,boolean closed){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0][];
        int n=xs.length,segments=closed?n:n-1;Segment[] built=new Segment[segments];ArrayList<double[]> out=new ArrayList<>();
        for(int i=0;i<segments;i++){
            int j=(i+1)%n;double sw=value(startWidths,i),ew=value(endWidths,i);
            Segment s=segment(xs[i],ys[i],xs[j],ys[j],value(bulges,i),sw,ew);built[i]=s;
            if(s!=null&&s.outline.length>=6)out.add(s.outline);
        }
        for(int i=1;i<segments;i++)addJoin(out,built[i-1],built[i],xs[i],ys[i]);
        if(closed&&segments>1)addJoin(out,built[segments-1],built[0],xs[0],ys[0]);
        return out.toArray(new double[out.size()][]);
    }

    public static boolean hasVariable(double[] startWidths,double[] endWidths){
        int n=Math.max(startWidths==null?0:startWidths.length,endWidths==null?0:endWidths.length);
        for(int i=0;i<n;i++)if(value(startWidths,i)>1e-12||value(endWidths,i)>1e-12)return true;
        return false;
    }

    private static final class Segment{
        final double[] outline;final double slx,sly,srx,sry,elx,ely,erx,ery;
        Segment(double[] outline,double slx,double sly,double srx,double sry,double elx,double ely,double erx,double ery){
            this.outline=outline;this.slx=slx;this.sly=sly;this.srx=srx;this.sry=sry;this.elx=elx;this.ely=ely;this.erx=erx;this.ery=ery;
        }
    }

    private static Segment segment(double x1,double y1,double x2,double y2,double bulge,double sw,double ew){
        if(!finite(x1,y1,x2,y2,bulge,sw,ew)||Math.hypot(x2-x1,y2-y1)<1e-12||Math.max(sw,ew)<=1e-12)return null;
        double[][] center=sampleSegment(x1,y1,x2,y2,bulge);if(center.length<2)return null;
        int m=center.length;double[] leftX=new double[m],leftY=new double[m],rightX=new double[m],rightY=new double[m];
        for(int k=0;k<m;k++){
            int a=k==0?0:k-1,b=k==m-1?m-1:k+1;double dx=center[b][0]-center[a][0],dy=center[b][1]-center[a][1],len=Math.hypot(dx,dy);
            if(len<1e-12){dx=x2-x1;dy=y2-y1;len=Math.hypot(dx,dy);}if(len<1e-12)continue;
            double nx=-dy/len,ny=dx/len,t=m==1?0d:k/(double)(m-1),half=(sw+(ew-sw)*t)*.5;
            leftX[k]=center[k][0]+nx*half;leftY[k]=center[k][1]+ny*half;rightX[k]=center[k][0]-nx*half;rightY[k]=center[k][1]-ny*half;
        }
        double[] outline=new double[m*4];int p=0;
        for(int k=0;k<m;k++){outline[p++]=leftX[k];outline[p++]=leftY[k];}
        for(int k=m-1;k>=0;k--){outline[p++]=rightX[k];outline[p++]=rightY[k];}
        return new Segment(outline,leftX[0],leftY[0],rightX[0],rightY[0],leftX[m-1],leftY[m-1],rightX[m-1],rightY[m-1]);
    }

    private static void addJoin(ArrayList<double[]> out,Segment prev,Segment next,double x,double y){
        if(prev==null||next==null||!finite(x,y))return;
        out.add(new double[]{prev.elx,prev.ely,x,y,next.slx,next.sly});
        out.add(new double[]{prev.erx,prev.ery,x,y,next.srx,next.sry});
    }

    private static double[][] sampleSegment(double x1,double y1,double x2,double y2,double bulge){
        double dx=x2-x1,dy=y2-y1,chord=Math.hypot(dx,dy);
        if(chord<1e-12||Math.abs(bulge)<1e-10)return new double[][]{{x1,y1},{x2,y2}};
        double theta=4d*Math.atan(bulge),h=chord*(1d-bulge*bulge)/(4d*bulge),mx=(x1+x2)*.5,my=(y1+y2)*.5;
        double cx=mx-dy/chord*h,cy=my+dx/chord*h,r=Math.hypot(x1-cx,y1-cy),start=Math.atan2(y1-cy,x1-cx);
        if(!Double.isFinite(r)||r<1e-12)return new double[][]{{x1,y1},{x2,y2}};
        int steps=Math.max(2,Math.min(96,(int)Math.ceil(Math.abs(theta)/(Math.PI/18d))));double[][] q=new double[steps+1][2];
        q[0][0]=x1;q[0][1]=y1;
        for(int s=1;s<=steps;s++){
            if(s==steps){q[s][0]=x2;q[s][1]=y2;}else{double a=start+theta*s/steps;q[s][0]=cx+r*Math.cos(a);q[s][1]=cy+r*Math.sin(a);}
        }
        return q;
    }

    private static double value(double[] values,int i){return values!=null&&i>=0&&i<values.length?constant(values[i]):0d;}
    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private DxfPolylineWidth(){}
}
