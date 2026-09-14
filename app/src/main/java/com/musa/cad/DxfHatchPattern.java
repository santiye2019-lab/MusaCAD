package com.musa.cad;

/** Pure geometry helpers for DXF HATCH pattern line families. */
public final class DxfHatchPattern {
    public static final class Line {
        public final double angleDeg,baseX,baseY,offsetX,offsetY;
        public final double[] dashes;
        public Line(double angleDeg,double baseX,double baseY,double offsetX,double offsetY,double[] dashes){
            this.angleDeg=angleDeg;this.baseX=baseX;this.baseY=baseY;
            this.offsetX=offsetX;this.offsetY=offsetY;
            this.dashes=dashes==null?new double[0]:dashes.clone();
        }
        public double dx(){return Math.cos(Math.toRadians(angleDeg));}
        public double dy(){return Math.sin(Math.toRadians(angleDeg));}
    }

    /** Inclusive k range for base + k*offset lines that can intersect the bbox. */
    public static int[] familyRange(Line line,double minX,double minY,double maxX,double maxY){
        double dx=line.dx(),dy=line.dy(),nx=-dy,ny=dx;
        double dn=line.offsetX*nx+line.offsetY*ny;
        if(!Double.isFinite(dn)||Math.abs(dn)<1e-12)return new int[]{0,0};
        double[] px={minX,maxX,maxX,minX},py={minY,minY,maxY,maxY};
        double min=Double.POSITIVE_INFINITY,max=Double.NEGATIVE_INFINITY;
        for(int i=0;i<4;i++){double q=px[i]*nx+py[i]*ny;min=Math.min(min,q);max=Math.max(max,q);}
        double base=line.baseX*nx+line.baseY*ny;
        double r1=(min-base)/dn,r2=(max-base)/dn;
        long lo=(long)Math.floor(Math.min(r1,r2))-1L,hi=(long)Math.ceil(Math.max(r1,r2))+1L;
        if(lo<Integer.MIN_VALUE)lo=Integer.MIN_VALUE;if(hi>Integer.MAX_VALUE)hi=Integer.MAX_VALUE;
        return new int[]{(int)lo,(int)hi};
    }

    public static double dashCycle(Line line){
        double sum=0;
        for(double d:line.dashes)if(Double.isFinite(d))sum+=Math.abs(d);
        return sum;
    }

    private DxfHatchPattern(){}
}
