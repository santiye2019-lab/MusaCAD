package com.musa.cad;

/** Calculations promote coordinates before arithmetic; area uses a local origin. */
public final class MeasurementMath {
    public static double distance(float ax,float ay,float bx,float by){
        return Math.hypot((double)ax-bx,(double)ay-by);
    }
    public static double area(float[] xy){
        if(xy==null||xy.length%2!=0)throw new IllegalArgumentException("Coordinate pairs required");
        if(xy.length<6)return 0;
        double ox=xy[0],oy=xy[1],sum=0,correction=0;
        for(int i=2;i<xy.length-2;i+=2){
            double cross=((double)xy[i]-ox)*((double)xy[i+3]-oy)-((double)xy[i+2]-ox)*((double)xy[i+1]-oy);
            double term=cross-correction,next=sum+term;correction=(next-sum)-term;sum=next;
        }
        return Math.abs(sum)*.5;
    }
    private MeasurementMath(){}
}
