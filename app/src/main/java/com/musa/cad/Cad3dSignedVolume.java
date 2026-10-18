package com.musa.cad;

/**
 * Signed algebraic volume enclosed by an indexed, consistently oriented
 * triangular mesh. This is NOT an ACIS/B-Rep solid kernel or an engineering
 * quantity certification: self-intersecting faces, coincident shells and
 * unverified CAD proxy objects can still make an indexed-closed mesh invalid.
 *
 * Accept ONLY an independently inspected closed/oriented indexed shell. The
 * result is signed (outward faces positive, inward faces negative); report
 * absolute magnitude as informational mesh volume in drawing units cubed.
 *
 * Runs on the file-loading worker. O(1) additional memory, O(face count).
 */
public final class Cad3dSignedVolume {
    private Cad3dSignedVolume(){}
    /* Android requires one portable native CAD engine. Java equations below
     * remain only as a non-Android JVM differential test reference. */
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
        public final boolean available;
        public final double signedVolume;
        public final int triangleCount;
        // Geometry centroid of the signed closed triangle shell, in WCS drawing units.
        // NaN when the shell is open, unverified or has zero net signed volume.
        public final double centroidX,centroidY,centroidZ;
        private Result(boolean available,double signedVolume,int triangleCount,
                       double centroidX,double centroidY,double centroidZ){
            this.available=available;this.signedVolume=signedVolume;
            this.triangleCount=triangleCount;
            this.centroidX=centroidX;this.centroidY=centroidY;this.centroidZ=centroidZ;
        }
        public double magnitude(){return available?Math.abs(signedVolume):0d;}
    }

    public static Result measure(float[] xyz,int[] triangles,
                                 Cad3dMeshTopology.Result topology){
        if(xyz==null||triangles==null||topology==null||
           xyz.length%3!=0||triangles.length%3!=0)
            throw new IllegalArgumentException("invalid CAD solid mesh input");
        final int faces=triangles.length/3;
        if(!topology.checked||!topology.closedOrientedByIndex||
           topology.triangles!=faces||faces==0)
            return new Result(false,0d,faces,Double.NaN,Double.NaN,Double.NaN);
        if(ANDROID_RUNTIME){
            /* C revalidates current vertices and faces; no stale approval. */
            double[] result=measureNative(xyz,triangles);
            if(result==null)
                return new Result(false,0d,faces,Double.NaN,Double.NaN,Double.NaN);
            if(result.length!=4||!Double.isFinite(result[0])||result[0]==0d||
               !Double.isFinite(result[1])||!Double.isFinite(result[2])||
               !Double.isFinite(result[3]))
                throw new IllegalArgumentException("invalid native CAD volume result");
            return new Result(true,result[0],faces,result[1],result[2],result[3]);
        }
        int vertices=xyz.length/3;
        int reference=triangles[0];
        if(reference<0||reference>=vertices)
            throw new IllegalArgumentException("invalid reference vertex");
        final int origin=reference*3;
        double ox=xyz[origin],oy=xyz[origin+1],oz=xyz[origin+2];
        if(!Double.isFinite(ox)||!Double.isFinite(oy)||!Double.isFinite(oz))
            throw new IllegalArgumentException("invalid reference point");

        // Translate near the mesh before the determinant: CAD world
        // coordinates may be millions of drawing units from (0,0,0).
        // Cross products about global zero lose accuracy catastrophically.
        double sum=0d,correction=0d;
        double mx=0d,my=0d,mz=0d,centroidCorrectionX=0d,centroidCorrectionY=0d,centroidCorrectionZ=0d;
        for(int i=0;i<triangles.length;i+=3){
            int ia=triangles[i],ib=triangles[i+1],ic=triangles[i+2];
            if(ia<0||ib<0||ic<0||ia>=vertices||ib>=vertices||ic>=vertices)
                throw new IllegalArgumentException("invalid CAD face index");
            int a=ia*3,b=ib*3,c=ic*3;
            double ax=(double)xyz[a]-ox,ay=(double)xyz[a+1]-oy,az=(double)xyz[a+2]-oz;
            double bx=(double)xyz[b]-ox,by=(double)xyz[b+1]-oy,bz=(double)xyz[b+2]-oz;
            double cx=(double)xyz[c]-ox,cy=(double)xyz[c+1]-oy,cz=(double)xyz[c+2]-oz;
            if(!Double.isFinite(ax)||!Double.isFinite(ay)||!Double.isFinite(az)||
               !Double.isFinite(bx)||!Double.isFinite(by)||!Double.isFinite(bz)||
               !Double.isFinite(cx)||!Double.isFinite(cy)||!Double.isFinite(cz))
                throw new IllegalArgumentException("invalid CAD XYZ coordinate");
            double determinant=ax*(by*cz-bz*cy)+
                               ay*(bz*cx-bx*cz)+
                               az*(bx*cy-by*cx);
            double tetra=determinant/6d;
            if(!Double.isFinite(tetra))
                throw new IllegalArgumentException("CAD signed volume overflow");
            // Neumaier compensation for numerical fidelity across facets.
            double next=sum+tetra;
            if(Math.abs(sum)>=Math.abs(tetra))correction+=(sum-next)+tetra;
            else correction+=(tetra-next)+sum;
            sum=next;
            // First moments of each signed tetrahedron, translated about
            // the opening vertex to retain precision at large CAD coordinates.
            double tx=tetra*(ax+bx+cx)/4d;
            double ty=tetra*(ay+by+cy)/4d;
            double tz=tetra*(az+bz+cz)/4d;
            if(!Double.isFinite(tx)||!Double.isFinite(ty)||!Double.isFinite(tz))
                throw new IllegalArgumentException("CAD centroid moment overflow");
            double nx=mx+tx,ny=my+ty,nz=mz+tz;
            centroidCorrectionX+=(Math.abs(mx)>=Math.abs(tx))?(mx-nx)+tx:(tx-nx)+mx;
            centroidCorrectionY+=(Math.abs(my)>=Math.abs(ty))?(my-ny)+ty:(ty-ny)+my;
            centroidCorrectionZ+=(Math.abs(mz)>=Math.abs(tz))?(mz-nz)+tz:(tz-nz)+mz;
            mx=nx;my=ny;mz=nz;
        }
        double signed=sum+correction;
        if(!Double.isFinite(signed))
            throw new IllegalArgumentException("CAD signed volume overflow");
        // Indexed shell with 0 algebraic volume cannot be certified as an
        // enclosing mesh (e.g. offsetting/orientation cancellations).
        if(signed==0d)return new Result(false,0d,faces,Double.NaN,Double.NaN,Double.NaN);
        double centerX=ox+(mx+centroidCorrectionX)/signed;
        double centerY=oy+(my+centroidCorrectionY)/signed;
        double centerZ=oz+(mz+centroidCorrectionZ)/signed;
        if(!Double.isFinite(centerX)||!Double.isFinite(centerY)||!Double.isFinite(centerZ))
            throw new IllegalArgumentException("CAD shell centroid overflow");
        return new Result(true,signed,faces,centerX,centerY,centerZ);
    }
}
