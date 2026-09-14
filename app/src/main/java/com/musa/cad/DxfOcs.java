package com.musa.cad;

/** AutoCAD arbitrary-axis OCS -> WCS projection helpers used by the 2D renderer. */
public final class DxfOcs {
    private static final class Basis {
        final double axx,axy,axz,ayx,ayy,ayz,nx,ny,nz;
        Basis(double axx,double axy,double axz,double ayx,double ayy,double ayz,double nx,double ny,double nz){
            this.axx=axx;this.axy=axy;this.axz=axz;
            this.ayx=ayx;this.ayy=ayy;this.ayz=ayz;
            this.nx=nx;this.ny=ny;this.nz=nz;
        }
    }

    /**
     * Returns a 2D affine [a,b,c,d,x,y] that projects OCS x/y coordinates at a
     * constant OCS elevation onto the WCS XY plane.
     */
    public static double[] plane2d(double ex,double ey,double ez,double elevation){
        if(!finite(ex,ey,ez,elevation))throw new IllegalArgumentException("Non-finite OCS plane");
        Basis q=basis(ex,ey,ez);
        double tx=q.nx*elevation,ty=q.ny*elevation;
        if(!finite(q.axx,q.axy,q.ayx,q.ayy,tx,ty))throw new IllegalArgumentException("Invalid OCS plane projection");
        return new double[]{q.axx,q.axy,q.ayx,q.ayy,tx,ty};
    }

    /** Returns Android/DxfBlocks-style 2D affine [a,b,c,d,x,y] for INSERT/MINSERT. */
    public static double[] insert2d(double bx,double by,double bz,
                                    double sx,double sy,double sz,double degrees,
                                    double ix,double iy,double iz,
                                    double ex,double ey,double ez,
                                    double arrayX,double arrayY){
        if(!finite(bx,by,bz,sx,sy,sz,degrees,ix,iy,iz,ex,ey,ez,arrayX,arrayY))
            throw new IllegalArgumentException("Non-finite OCS transform");
        Basis q=basis(ex,ey,ez);

        double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);
        double r00=co*sx,r01=-si*sy,r10=si*sx,r11=co*sy;
        // Project the block XY basis through OCS into WCS XY.
        double a=q.axx*r00+q.ayx*r10;
        double c=q.axx*r01+q.ayx*r11;
        double b=q.axy*r00+q.ayy*r10;
        double d=q.axy*r01+q.ayy*r11;

        // MINSERT row/column spacing follows INSERT rotation, but not block scale.
        double ox=co*arrayX-si*arrayY;
        double oy=si*arrayX+co*arrayY;
        double ocsX=ix+ox,ocsY=iy+oy;
        double wx=q.axx*ocsX+q.ayx*ocsY+q.nx*iz;
        double wy=q.axy*ocsX+q.ayy*ocsY+q.ny*iz;
        // Parser is 2D, so member Z is treated as zero. Respect a nonzero block-base Z.
        wx-=q.nx*bz*sz;
        wy-=q.ny*bz*sz;
        double tx=wx-a*bx-c*by;
        double ty=wy-b*bx-d*by;
        if(!finite(a,b,c,d,tx,ty))throw new IllegalArgumentException("Invalid OCS projection");
        return new double[]{a,b,c,d,tx,ty};
    }

    private static Basis basis(double ex,double ey,double ez){
        double nl=Math.sqrt(ex*ex+ey*ey+ez*ez);
        if(!Double.isFinite(nl)||nl<1e-12)throw new IllegalArgumentException("Zero extrusion vector");
        double nx=ex/nl,ny=ey/nl,nz=ez/nl;

        // Autodesk arbitrary-axis algorithm.
        double axx,axy,axz;
        if(Math.abs(nx)<1d/64d&&Math.abs(ny)<1d/64d){
            // Wy x N
            axx=nz;axy=0;axz=-nx;
        }else{
            // Wz x N
            axx=-ny;axy=nx;axz=0;
        }
        double al=Math.sqrt(axx*axx+axy*axy+axz*axz);
        if(!Double.isFinite(al)||al<1e-12)throw new IllegalArgumentException("Invalid OCS X axis");
        axx/=al;axy/=al;axz/=al;

        // Ay = N x Ax
        double ayx=ny*axz-nz*axy;
        double ayy=nz*axx-nx*axz;
        double ayz=nx*axy-ny*axx;
        double yl=Math.sqrt(ayx*ayx+ayy*ayy+ayz*ayz);
        if(!Double.isFinite(yl)||yl<1e-12)throw new IllegalArgumentException("Invalid OCS Y axis");
        ayx/=yl;ayy/=yl;ayz/=yl;
        return new Basis(axx,axy,axz,ayx,ayy,ayz,nx,ny,nz);
    }

    private static boolean finite(double... values){
        for(double v:values)if(!Double.isFinite(v))return false;
        return true;
    }
    private DxfOcs(){}
}
