package com.musa.cad;

/** AutoCAD arbitrary-axis OCS -> WCS projection helpers used by 2D INSERT rendering. */
public final class DxfOcs {
    /** Returns Android/DxfBlocks-style 2D affine [a,b,c,d,x,y]. */
    public static double[] insert2d(double bx,double by,double bz,
                                    double sx,double sy,double sz,double degrees,
                                    double ix,double iy,double iz,
                                    double ex,double ey,double ez,
                                    double arrayX,double arrayY){
        if(!finite(bx,by,bz,sx,sy,sz,degrees,ix,iy,iz,ex,ey,ez,arrayX,arrayY))
            throw new IllegalArgumentException("Non-finite OCS transform");
        double nl=Math.sqrt(ex*ex+ey*ey+ez*ez);
        if(nl<1e-12)throw new IllegalArgumentException("Zero extrusion vector");
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
        if(al<1e-12)throw new IllegalArgumentException("Invalid OCS X axis");
        axx/=al;axy/=al;axz/=al;
        // Ay = N x Ax
        double ayx=ny*axz-nz*axy;
        double ayy=nz*axx-nx*axz;
        double ayz=nx*axy-ny*axx;
        double yl=Math.sqrt(ayx*ayx+ayy*ayy+ayz*ayz);
        if(yl<1e-12)throw new IllegalArgumentException("Invalid OCS Y axis");
        ayx/=yl;ayy/=yl;ayz/=yl;

        double r=Math.toRadians(degrees),co=Math.cos(r),si=Math.sin(r);
        double r00=co*sx,r01=-si*sy,r10=si*sx,r11=co*sy;
        // Project the block XY basis through OCS into WCS XY.
        double a=axx*r00+ayx*r10;
        double c=axx*r01+ayx*r11;
        double b=axy*r00+ayy*r10;
        double d=axy*r01+ayy*r11;

        // MINSERT row/column spacing follows INSERT rotation, but not block scale.
        double ox=co*arrayX-si*arrayY;
        double oy=si*arrayX+co*arrayY;
        double ocsX=ix+ox,ocsY=iy+oy;
        double wx=axx*ocsX+ayx*ocsY+nx*iz;
        double wy=axy*ocsX+ayy*ocsY+ny*iz;
        // Parser is 2D, so member Z is treated as zero. Respect a nonzero block-base Z.
        wx-=nx*bz*sz;
        wy-=ny*bz*sz;
        double tx=wx-a*bx-c*by;
        double ty=wy-b*bx-d*by;
        if(!finite(a,b,c,d,tx,ty))throw new IllegalArgumentException("Invalid OCS projection");
        return new double[]{a,b,c,d,tx,ty};
    }

    private static boolean finite(double... values){
        for(double v:values)if(!Double.isFinite(v))return false;
        return true;
    }
    private DxfOcs(){}
}
