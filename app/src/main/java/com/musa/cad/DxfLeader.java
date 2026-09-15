package com.musa.cad;

/** Pure geometry helpers for classic DXF LEADER paths and arrowheads. */
public final class DxfLeader {
    /** Returns tip,leftBase,rightBase packed as x/y pairs, or an empty array when geometry is degenerate. */
    public static double[] arrow(double tipX,double tipY,double nextX,double nextY,double requestedSize){
        double dx=nextX-tipX,dy=nextY-tipY,len=Math.hypot(dx,dy);if(!Double.isFinite(len)||len<1e-12)return new double[0];
        return arrowSized(tipX,tipY,nextX,nextY,saneSize(requestedSize,len));
    }

    /** Same arrow geometry, but the caller has already chosen/clamped the physical DXF size. */
    public static double[] arrowSized(double tipX,double tipY,double nextX,double nextY,double size){
        double dx=nextX-tipX,dy=nextY-tipY,len=Math.hypot(dx,dy);
        if(!Double.isFinite(len)||len<1e-12||!Double.isFinite(size)||size<=1e-12)return new double[0];
        double ux=dx/len,uy=dy/len,nx=-uy,ny=ux,half=size*.36;
        double bx=tipX+ux*size,by=tipY+uy*size;
        return new double[]{tipX,tipY,bx+nx*half,by+ny*half,bx-nx*half,by-ny*half};
    }

    /** Returns either the original straight LEADER vertices or a smooth spline path through them. */
    public static double[] path(double[] xs,double[] ys,boolean spline){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0];
        for(int i=0;i<xs.length;i++)if(!Double.isFinite(xs[i])||!Double.isFinite(ys[i]))return new double[0];
        if(spline){double[] sampled=DxfCurves.sampleFitSpline(xs,ys,false);if(sampled.length>=4)return sampled;}
        double[] packed=new double[xs.length*2];for(int i=0;i<xs.length;i++){packed[i*2]=xs[i];packed[i*2+1]=ys[i];}return packed;
    }

    /** Uses DIMSTYLE size when available; otherwise a conservative fraction of the first leader segment. */
    public static double saneSize(double styleSize,double firstSegmentLength){
        if(!Double.isFinite(firstSegmentLength)||firstSegmentLength<=1e-12)return 0d;
        double size=Double.isFinite(styleSize)&&styleSize>1e-12?styleSize:firstSegmentLength*.08;
        return Math.max(firstSegmentLength*.01,Math.min(firstSegmentLength*.30,size));
    }
    private DxfLeader(){}
}
