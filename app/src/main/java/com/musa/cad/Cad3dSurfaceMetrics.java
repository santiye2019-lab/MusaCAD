package com.musa.cad;

/**
 * Read-only triangle surface measurements in the drawing's native units.
 *
 * This is NOT a watertight-volume or ACIS B-Rep surface evaluation. The area
 * sums only triangles that are actually present in the decoded CAD mesh;
 * callers must disclose proxy/unsupported geometry and avoid claiming that
 * incomplete tessellation gives a full quantity takeoff.
 *
 * Runs off the Android UI thread and uses O(1) extra memory.
 */
public final class Cad3dSurfaceMetrics {
    private Cad3dSurfaceMetrics(){}

    public static final class Result {
        public final double area;
        public final int triangleCount,degenerateCount;
        private Result(double area,int triangleCount,int degenerateCount){
            this.area=area;this.triangleCount=triangleCount;
            this.degenerateCount=degenerateCount;
        }
    }

    public static Result measure(float[] xyz,int[] triangles){
        if(xyz==null||triangles==null||xyz.length%3!=0||
           triangles.length%3!=0)
            throw new IllegalArgumentException("invalid CAD triangle arrays");
        final int vertices=xyz.length/3;
        double sum=0,correction=0;
        int degenerate=0;
        for(int i=0;i<triangles.length;i+=3){
            int a=triangles[i],b=triangles[i+1],c=triangles[i+2];
            if(a<0||b<0||c<0||a>=vertices||b>=vertices||c>=vertices)
                throw new IllegalArgumentException("CAD triangle index out of range");
            int ax=a*3,bx=b*3,cx=c*3;
            for(int k=0;k<3;k++)
                if(!Float.isFinite(xyz[ax+k])||
                   !Float.isFinite(xyz[bx+k])||
                   !Float.isFinite(xyz[cx+k]))
                    throw new IllegalArgumentException("non-finite CAD triangle coordinate");
            // Subtract in double precision even for large real-world CAD
            // coordinates. Subtracting as float can overflow prematurely.
            double u0=(double)xyz[bx]-xyz[ax];
            double u1=(double)xyz[bx+1]-xyz[ax+1];
            double u2=(double)xyz[bx+2]-xyz[ax+2];
            double v0=(double)xyz[cx]-xyz[ax];
            double v1=(double)xyz[cx+1]-xyz[ax+1];
            double v2=(double)xyz[cx+2]-xyz[ax+2];
            double cx0=u1*v2-u2*v1;
            double cx1=u2*v0-u0*v2;
            double cx2=u0*v1-u1*v0;
            double face=0.5*Math.hypot(Math.hypot(cx0,cx1),cx2);
            if(!Double.isFinite(face))
                throw new IllegalArgumentException("CAD triangle area overflow");
            if(face==0)degenerate++;
            // Neumaier compensated summation prevents thousands of tiny
            // tessellated faces disappearing next to larger faces.
            double next=sum+face;
            if(Math.abs(sum)>=Math.abs(face))correction+=(sum-next)+face;
            else correction+=(face-next)+sum;
            sum=next;
        }
        double total=sum+correction;
        if(!Double.isFinite(total))
            throw new IllegalArgumentException("CAD surface area overflow");
        return new Result(total,triangles.length/3,degenerate);
    }
}
