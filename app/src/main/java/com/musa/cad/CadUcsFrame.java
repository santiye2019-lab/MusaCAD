package com.musa.cad;

import java.util.Arrays;

/** Immutable right-handed user coordinate system for safe WCS <-> UCS conversions. */
public final class CadUcsFrame {
    private static final double EPS=1e-10,MAX_COORD=1e9;
    private final double[] origin,xAxis,yAxis,zAxis;

    private CadUcsFrame(double[] origin,double[] xAxis,double[] yAxis,double[] zAxis){
        this.origin=origin;this.xAxis=xAxis;this.yAxis=yAxis;this.zAxis=zAxis;
    }
    public static CadUcsFrame world(){return of(0,0,0,1,0,0,0,1,0);}
    public static CadUcsFrame front(){return of(0,0,0,1,0,0,0,0,1);}
    public static CadUcsFrame right(){return of(0,0,0,0,1,0,0,0,1);}
    public static CadUcsFrame of(double ox,double oy,double oz,double xx,double xy,double xz,
                                 double yx,double yy,double yz){
        double[] origin=vec(ox,oy,oz);
        for(double v:origin)if(Math.abs(v)>MAX_COORD)throw new IllegalArgumentException("UCS origin exceeds CAD bounds");
        double[] x=unit(vec(xx,xy,xz));
        double[] y=vec(yx,yy,yz);
        double project=dot(x,y);
        for(int i=0;i<3;i++)y[i]-=project*x[i];
        y=unit(y);
        double[] z=unit(cross(x,y));
        return new CadUcsFrame(origin,x,y,z);
    }
    public static CadUcsFrame parse(String values){
        if(values==null)throw new IllegalArgumentException("UCS values required");
        String[] split=values.trim().split("\\s*[,;]\\s*",-1);
        if(split.length!=9)throw new IllegalArgumentException("Origin, X axis and Y axis each require XYZ coordinates (9 values)");
        double[] nums=new double[9];
        try{for(int i=0;i<9;i++)nums[i]=Double.parseDouble(split[i]);}
        catch(NumberFormatException e){throw new IllegalArgumentException("Invalid UCS coordinate",e);}
        return of(nums[0],nums[1],nums[2],nums[3],nums[4],nums[5],nums[6],nums[7],nums[8]);
    }
    private static double[] vec(double x,double y,double z){
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z))
            throw new IllegalArgumentException("UCS coordinates must be finite");
        return new double[]{x,y,z};
    }
    private static double dot(double[] a,double[] b){
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
    }
    private static double[] cross(double[] a,double[] b){
        return new double[]{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    }
    private static double[] unit(double[] input){
        double length=Math.hypot(Math.hypot(input[0],input[1]),input[2]);
        if(!Double.isFinite(length)||length<EPS)
            throw new IllegalArgumentException("UCS axes cannot be zero or parallel");
        for(int i=0;i<3;i++)input[i]/=length;
        return input;
    }
    public double[] toWorld(double x,double y,double z){
        vec(x,y,z);
        return new double[]{
            origin[0]+x*xAxis[0]+y*yAxis[0]+z*zAxis[0],
            origin[1]+x*xAxis[1]+y*yAxis[1]+z*zAxis[1],
            origin[2]+x*xAxis[2]+y*yAxis[2]+z*zAxis[2]};
    }
    public double[] toLocal(double x,double y,double z){
        double[] d=vec(x-origin[0],y-origin[1],z-origin[2]);
        return new double[]{dot(d,xAxis),dot(d,yAxis),dot(d,zAxis)};
    }
    /** Displacement only: the UCS origin must not shift the geometry during an axis step. */
    public double[] move(int axis,double distance){
        if(axis<0||axis>2||!Double.isFinite(distance))
            throw new IllegalArgumentException("Invalid UCS axis step");
        double[] direction=axis==0?xAxis:axis==1?yAxis:zAxis;
        return new double[]{distance*direction[0],distance*direction[1],distance*direction[2]};
    }
    public double[] origin(){return Arrays.copyOf(origin,3);}
    public double[] xAxis(){return Arrays.copyOf(xAxis,3);}
    public double[] yAxis(){return Arrays.copyOf(yAxis,3);}
    public double[] zAxis(){return Arrays.copyOf(zAxis,3);}
}
