package com.musa.cad;

/** Pure geometry helpers for classic DXF LEADER arrowheads. */
public final class DxfLeader {
    /** Returns tip,leftBase,rightBase packed as x/y pairs, or an empty array when geometry is degenerate. */
    public static double[] arrow(double tipX,double tipY,double nextX,double nextY,double requestedSize){
        double dx=nextX-tipX,dy=nextY-tipY,len=Math.hypot(dx,dy);if(!Double.isFinite(len)||len<1e-12)return new double[0];
        double size=saneSize(requestedSize,len);if(size<=0)return new double[0];
        double ux=dx/len,uy=dy/len,nx=-uy,ny=ux,half=size*.36;
        double bx=tipX+ux*size,by=tipY+uy*size;
        return new double[]{tipX,tipY,bx+nx*half,by+ny*half,bx-nx*half,by-ny*half};
    }

    /** Uses DIMSTYLE size when available; otherwise a conservative fraction of the first leader segment. */
    public static double saneSize(double styleSize,double firstSegmentLength){
        if(!Double.isFinite(firstSegmentLength)||firstSegmentLength<=1e-12)return 0d;
        double size=Double.isFinite(styleSize)&&styleSize>1e-12?styleSize:firstSegmentLength*.08;
        return Math.max(firstSegmentLength*.01,Math.min(firstSegmentLength*.30,size));
    }
    private DxfLeader(){}
}
