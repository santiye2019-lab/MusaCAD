package com.musa.cad;

/**
 * Pure Java screen-space axis picking and screen-drag -> CAD drawing-unit math.
 * Camera axes agree with Mesh3dView's orthographic tangent at a mesh vertex.
 */
public final class CadGizmoMath {
    private static final double MIN_VISIBLE_AXIS=0.12;
    private CadGizmoMath(){}
    /* On actual Android the shared native CAD core is mandatory. Existing
     * pure-Java equations remain a host-only regression oracle during the
     * tracked UCS/Gizmo migration; they are NOT an Android fallback. */
    private static final boolean ANDROID_RUNTIME=isAndroidRuntime();
    private static boolean isAndroidRuntime(){
        String vm=System.getProperty("java.vm.name","");
        return "Dalvik".equalsIgnoreCase(vm)||"ART".equalsIgnoreCase(vm);
    }
    static {
        if(ANDROID_RUNTIME)System.loadLibrary("musacad_dwg");
    }
    private static native double[] axisDirectionNative(double x,double y,double z,
                                                        double yaw,double pitch);
    private static native int pickNative(double x,double y,double[][] directions,
                                         double handlePixels,double tolerancePixels);
    private static native double drawingDistanceNative(double dxPixels,double dyPixels,
                    double axisX,double axisY,double pixelsPerUnit);

    /** Screen x increases right; screen y increases downward. */
    public static double[] axisDirection(CadUcsFrame frame,int axis,double yaw,double pitch){
        if(frame==null||axis<0||axis>2||!Double.isFinite(yaw)||!Double.isFinite(pitch))
            throw new IllegalArgumentException("Invalid gizmo axis/camera");
        double[] d=frame.move(axis,1d);
        if(ANDROID_RUNTIME)return axisDirectionNative(d[0],d[1],d[2],yaw,pitch);
        double co=Math.cos(yaw),si=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);
        double away=si*d[0]+co*d[1];
        return new double[]{co*d[0]-si*d[1],sp*away-cp*d[2]};
    }

    /** Return -1 if no arrowhead is near the touch point. */
    public static int pick(double localX,double localY,double[][] directions,
                           double handlePixels,double tolerancePixels){
        if(directions==null||directions.length!=3||!Double.isFinite(localX)||
           !Double.isFinite(localY)||!Double.isFinite(handlePixels)||
           !Double.isFinite(tolerancePixels)||handlePixels<=0||tolerancePixels<=0)
            return -1;
        if(ANDROID_RUNTIME)return pickNative(localX,localY,directions,handlePixels,tolerancePixels);
        int selected=-1;
        double best=tolerancePixels*tolerancePixels;
        for(int i=0;i<3;i++){
            double[] v=directions[i];
            if(v==null||v.length!=2)continue;
            double len=Math.hypot(v[0],v[1]);
            if(!Double.isFinite(len)||len<MIN_VISIBLE_AXIS)continue;
            double endX=v[0]/len*handlePixels,endY=v[1]/len*handlePixels;
            double d2=(localX-endX)*(localX-endX)+(localY-endY)*(localY-endY);
            if(d2<=best){best=d2;selected=i;}
        }
        return selected;
    }

    /**
     * Project finger motion onto the chosen axis, including foreshortening.
     * A near-view-parallel axis yields NaN, never an arbitrary model displacement.
     */
    public static double drawingDistance(double dxPixels,double dyPixels,
                                         double axisX,double axisY,double pixelsPerUnit){
        if(ANDROID_RUNTIME)return drawingDistanceNative(dxPixels,dyPixels,axisX,axisY,pixelsPerUnit);
        double norm=axisX*axisX+axisY*axisY;
        if(!Double.isFinite(dxPixels)||!Double.isFinite(dyPixels)||
           !Double.isFinite(norm)||norm<MIN_VISIBLE_AXIS*MIN_VISIBLE_AXIS||
           !Double.isFinite(pixelsPerUnit)||pixelsPerUnit<=0)
            return Double.NaN;
        double result=(dxPixels*axisX+dyPixels*axisY)/(pixelsPerUnit*norm);
        return Double.isFinite(result)?result:Double.NaN;
    }
}
