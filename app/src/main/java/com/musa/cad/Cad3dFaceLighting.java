package com.musa.cad;

/**
 * Conservative mesh face Lambert shading using 64-bit geometry arithmetic.
 *
 * This is the viewport *display* normal, not a CAD B-Rep face normal: it
 * cannot fix a missing surface, an invalid solid, or user-defined materials.
 * Avoids float cross-product overflow on large 3D drawing coordinates.
 */
public final class Cad3dFaceLighting {
    private Cad3dFaceLighting(){}

    /**
     * Returns 0..1 diffuse brightness from a face's unoriented normal;
     * -1 is a degenerate or invalid triangle that must not be drawn as a face.
     *
     * The legacy viewport uses an absolute dot product so front/back surfaces
     * remain similarly lit; preserve this visual style, only fix numerics.
     */
    public static double brightness(float[] xyz,int a,int b,int c){
        if(xyz==null||xyz.length%3!=0||a<0||b<0||c<0||
           a>=xyz.length/3||b>=xyz.length/3||c>=xyz.length/3)
            return -1;
        int i=a*3,j=b*3,k=c*3;
        for(int axis=0;axis<3;axis++){
            if(!Float.isFinite(xyz[i+axis])||
               !Float.isFinite(xyz[j+axis])||
               !Float.isFinite(xyz[k+axis]))
                return -1;
        }
        double ux=(double)xyz[j]-xyz[i];
        double uy=(double)xyz[j+1]-xyz[i+1];
        double uz=(double)xyz[j+2]-xyz[i+2];
        double vx=(double)xyz[k]-xyz[i];
        double vy=(double)xyz[k+1]-xyz[i+1];
        double vz=(double)xyz[k+2]-xyz[i+2];
        double nx=uy*vz-uz*vy;
        double ny=uz*vx-ux*vz;
        double nz=ux*vy-uy*vx;
        // Inputs are finite float32 coordinates: double-normal squared
        // magnitudes remain well inside double range. One sqrt is cheaper
        // than two Math.hypot calls for every shaded triangle every frame.
        double length=Math.sqrt(nx*nx+ny*ny+nz*nz);
        if(!Double.isFinite(length)||length<=0d)
            return -1d;
        double light=Math.abs((0.35d*nx-0.4d*ny+0.8d*nz)/length);
        return Double.isFinite(light)?Math.min(1d,Math.max(0d,light)):-1d;
    }
}
