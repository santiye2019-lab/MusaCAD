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

    public static final class Result {
        public final boolean available;
        public final double signedVolume;
        public final int triangleCount;
        private Result(boolean available,double signedVolume,int triangleCount){
            this.available=available;this.signedVolume=signedVolume;
            this.triangleCount=triangleCount;
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
            return new Result(false,0d,faces);
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
        }
        double signed=sum+correction;
        if(!Double.isFinite(signed))
            throw new IllegalArgumentException("CAD signed volume overflow");
        // Indexed shell with 0 algebraic volume cannot be certified as an
        // enclosing mesh (e.g. offsetting/orientation cancellations).
        return new Result(signed!=0d,signed,faces);
    }
}
