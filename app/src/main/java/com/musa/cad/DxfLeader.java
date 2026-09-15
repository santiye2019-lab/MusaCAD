package com.musa.cad;

/** Pure geometry helpers for classic DXF LEADER paths, arrowheads and hook lines. */
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
        return path(xs,ys,spline,Double.NaN,Double.NaN);
    }

    /**
     * Spline LEADER variant honoring AutoCAD's stored end-tangent direction (group 211/221).
     * NaN tangent values keep the automatic tangent used by the generic fit-point sampler.
     */
    public static double[] path(double[] xs,double[] ys,boolean spline,double endTx,double endTy){
        if(xs==null||ys==null||xs.length!=ys.length||xs.length<2)return new double[0];
        for(int i=0;i<xs.length;i++)if(!Double.isFinite(xs[i])||!Double.isFinite(ys[i]))return new double[0];
        if(spline){double[] sampled=DxfCurves.sampleFitSpline(xs,ys,false,Double.NaN,Double.NaN,endTx,endTy);if(sampled.length>=4)return sampled;}
        double[] packed=new double[xs.length*2];for(int i=0;i<xs.length;i++){packed[i*2]=xs[i];packed[i*2+1]=ys[i];}return packed;
    }

    /**
     * Returns the classic horizontal hook/landing segment as start/end x/y pairs.
     * Group 74 selects whether the hook follows or opposes the stored horizontal vector.
     */
    public static double[] hook(double x,double y,double horizontalX,double horizontalY,boolean sameDirection,double size){
        double len=Math.hypot(horizontalX,horizontalY);
        if(!finite(x,y,horizontalX,horizontalY,size)||len<1e-12||size<=1e-12)return new double[0];
        double sign=sameDirection?1d:-1d,ux=horizontalX/len*sign,uy=horizontalY/len*sign;
        return new double[]{x,y,x+ux*size,y+uy*size};
    }

    /** Uses DIMSTYLE size when available; otherwise a conservative fraction of the adjacent leader segment. */
    public static double saneSize(double styleSize,double firstSegmentLength){
        if(!Double.isFinite(firstSegmentLength)||firstSegmentLength<=1e-12)return 0d;
        double size=Double.isFinite(styleSize)&&styleSize>1e-12?styleSize:firstSegmentLength*.08;
        return Math.max(firstSegmentLength*.01,Math.min(firstSegmentLength*.30,size));
    }
    private static boolean finite(double... values){for(double v:values)if(!Double.isFinite(v))return false;return true;}
    private DxfLeader(){}
}
