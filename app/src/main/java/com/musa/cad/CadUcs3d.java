package com.musa.cad;

import java.io.IOException;

/**
 * Orthonormal arbitrary 3D UCS transform. All axes are right-handed and
 * invertible. Coordinates remain in drawing units without hidden scaling.
 */
public final class CadUcs3d {
    public final double[] origin,xAxis,yAxis,zAxis;

    private CadUcs3d(double[] origin,double[] x,double[] y,double[] z){
        this.origin=origin;this.xAxis=x;this.yAxis=y;this.zAxis=z;
    }
    private static double norm(double[] a){
        return Math.sqrt(dot(a,a));
    }
    private static double dot(double[]a,double[]b){
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
    }
    private static double[] cross(double[]a,double[]b){
        return new double[]{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    }
    private static double[] normalized(double[] a)throws IOException{
        double length=norm(a);
        if(!Double.isFinite(length)||length<1e-10)
            throw new IOException("UCS ekseni sıfır veya güvenilir biçimde tanımlanmamış");
        return new double[]{a[0]/length,a[1]/length,a[2]/length};
    }
    public static CadUcs3d of(double[] origin,double[] xDirection,double[] approximateY)throws IOException{
        if(origin==null||xDirection==null||approximateY==null
            ||origin.length!=3||xDirection.length!=3||approximateY.length!=3)
            throw new IOException("UCS için orijin + iki 3B eksen vektörü gereklidir");
        for(double[] a:new double[][]{origin,xDirection,approximateY})
            for(double value:a)if(!Double.isFinite(value)||Math.abs(value)>1e9)
                throw new IOException("Geçersiz UCS koordinatı");
        double[] x=normalized(xDirection);
        // Reject almost-parallel axes rather than creating unstable geometry.
        double[] z=normalized(cross(x,approximateY));
        double[] y=normalized(cross(z,x));
        return new CadUcs3d(origin.clone(),x,y,z);
    }
    public static CadUcs3d world()throws IOException{
        return of(new double[]{0,0,0},new double[]{1,0,0},new double[]{0,1,0});
    }
    public double[] toWorld(double x,double y,double z){
        return new double[]{
            origin[0]+xAxis[0]*x+yAxis[0]*y+zAxis[0]*z,
            origin[1]+xAxis[1]*x+yAxis[1]*y+zAxis[1]*z,
            origin[2]+xAxis[2]*x+yAxis[2]*y+zAxis[2]*z
        };
    }
    public double[] toLocal(double x,double y,double z){
        double[] delta={x-origin[0],y-origin[1],z-origin[2]};
        return new double[]{dot(delta,xAxis),dot(delta,yAxis),dot(delta,zAxis)};
    }
    public double[] normalToWorld(double nx,double ny,double nz){
        return new double[]{
            xAxis[0]*nx+yAxis[0]*ny+zAxis[0]*nz,
            xAxis[1]*nx+yAxis[1]*ny+zAxis[1]*nz,
            xAxis[2]*nx+yAxis[2]*ny+zAxis[2]*nz
        };
    }
}
