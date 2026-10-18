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
    /* Native geometry is mandatory on Android; Java below is a JVM-only
     * differential reference, NOT an Android backup CAD engine. */
    private static final boolean ANDROID_RUNTIME=isAndroidRuntime();
    private static boolean isAndroidRuntime(){
        String vm=System.getProperty("java.vm.name","");
        return "Dalvik".equalsIgnoreCase(vm)||"ART".equalsIgnoreCase(vm);
    }
    static {
        if(ANDROID_RUNTIME)System.loadLibrary("musacad_dwg");
    }
    private static native double[] measureNative(float[] xyz,int[] triangles);

    public static final class Result {
        public final double area;
        public final int triangleCount,degenerateCount;
        // Triangle-area-weighted surface centroid; NOT the solid's mass center.
        // Zero-area and empty meshes expose NaN instead of fabricated XYZ.
        public final double centroidX,centroidY,centroidZ;
        private Result(double area,int triangleCount,int degenerateCount,
                       double centroidX,double centroidY,double centroidZ){
            this.area=area;this.triangleCount=triangleCount;
            this.degenerateCount=degenerateCount;
            this.centroidX=centroidX;this.centroidY=centroidY;this.centroidZ=centroidZ;
        }
    }

    public static Result measure(float[] xyz,int[] triangles){
        if(xyz==null||triangles==null||xyz.length%3!=0||
           triangles.length%3!=0)
            throw new IllegalArgumentException("invalid CAD triangle arrays");
        if(ANDROID_RUNTIME){
            double[] data=measureNative(xyz,triangles);
            if(data==null||data.length!=6||
               !Double.isFinite(data[0])||data[0]<0||
               data[4]!=triangles.length/3||data[5]<0||
               data[5]>data[4]||data[5]!=Math.rint(data[5])||
               (data[0]==0d?!(Double.isNaN(data[1])&&Double.isNaN(data[2])&&
                   Double.isNaN(data[3])):
                   !(Double.isFinite(data[1])&&Double.isFinite(data[2])&&
                   Double.isFinite(data[3]))))
                throw new IllegalArgumentException("invalid native CAD surface data");
            return new Result(data[0],(int)data[4],(int)data[5],
                    data[1],data[2],data[3]);
        }
        final int vertices=xyz.length/3;
        // Translate moment calculations near the mesh, so large CAD WCS
        // coordinates do not dominate much smaller model dimensions.
        int first=triangles.length>0?triangles[0]:-1;
        if(first>=vertices||first< -1)
            throw new IllegalArgumentException("CAD reference vertex out of range");
        if(first<0&&triangles.length>0)
            throw new IllegalArgumentException("CAD reference vertex out of range");
        double ox=first>=0?xyz[3*first]:0d;
        double oy=first>=0?xyz[3*first+1]:0d;
        double oz=first>=0?xyz[3*first+2]:0d;
        if(!Double.isFinite(ox)||!Double.isFinite(oy)||!Double.isFinite(oz))
            throw new IllegalArgumentException("invalid CAD surface reference");
        double sum=0,correction=0;
        double momentX=0,momentY=0,momentZ=0;
        double compX=0,compY=0,compZ=0;
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
            if(face>0d){
                double localX=(((double)xyz[ax]-ox)+((double)xyz[bx]-ox)+((double)xyz[cx]-ox))/3d;
                double localY=(((double)xyz[ax+1]-oy)+((double)xyz[bx+1]-oy)+((double)xyz[cx+1]-oy))/3d;
                double localZ=(((double)xyz[ax+2]-oz)+((double)xyz[bx+2]-oz)+((double)xyz[cx+2]-oz))/3d;
                double fx=face*localX,fy=face*localY,fz=face*localZ;
                if(!Double.isFinite(fx)||!Double.isFinite(fy)||!Double.isFinite(fz))
                    throw new IllegalArgumentException("CAD surface centroid moment overflow");
                double nextX=momentX+fx,nextY=momentY+fy,nextZ=momentZ+fz;
                compX+=(Math.abs(momentX)>=Math.abs(fx))?(momentX-nextX)+fx:(fx-nextX)+momentX;
                compY+=(Math.abs(momentY)>=Math.abs(fy))?(momentY-nextY)+fy:(fy-nextY)+momentY;
                compZ+=(Math.abs(momentZ)>=Math.abs(fz))?(momentZ-nextZ)+fz:(fz-nextZ)+momentZ;
                momentX=nextX;momentY=nextY;momentZ=nextZ;
            }
        }
        double total=sum+correction;
        if(!Double.isFinite(total))
            throw new IllegalArgumentException("CAD surface area overflow");
        if(total==0d)return new Result(0d,triangles.length/3,degenerate,
                Double.NaN,Double.NaN,Double.NaN);
        double centerX=ox+(momentX+compX)/total;
        double centerY=oy+(momentY+compY)/total;
        double centerZ=oz+(momentZ+compZ)/total;
        if(!Double.isFinite(centerX)||!Double.isFinite(centerY)||!Double.isFinite(centerZ))
            throw new IllegalArgumentException("CAD surface centroid overflow");
        return new Result(total,triangles.length/3,degenerate,centerX,centerY,centerZ);
    }
}
