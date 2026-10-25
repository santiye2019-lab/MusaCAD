package com.musa.cad;

/** Host-only regression for Android culling. This tests view optimization,
 * NOT original DWG decode or shared C11 CAD semantics. */
public final class NativeArcCullingBoundsTest {
    private static void require(boolean b,String detail){
        if(!b)throw new AssertionError(detail);
    }
    private static void oracle(float cx,float cy,float ux,float uy,
                               float vx,float vy,float start,float sweep){
        float[] b={-7,-8,-9,-10};
        require(NativeSceneViewportBounds.arcCullingBounds(
                cx,cy,ux,uy,vx,vy,start,sweep,b),"ARC bounds rejected");
        require(b[0]<=b[2]&&b[1]<=b[3]&&
            Float.isFinite(b[0])&&Float.isFinite(b[1])&&
            Float.isFinite(b[2])&&Float.isFinite(b[3]),"bad AABB");
        /* Independent sampling oracle for positive/negative direction,
         * arbitrary ellipse basis, and angular wrap. */
        for(int i=0;i<=1440;i++){
            double a=start+(double)sweep*i/1440.0;
            double x=cx+ux*Math.cos(a)+vx*Math.sin(a);
            double y=cy+uy*Math.cos(a)+vy*Math.sin(a);
            require(x>=b[0]-1e-6&&x<=b[2]+1e-6&&
                    y>=b[1]-1e-6&&y<=b[3]+1e-6,
                    "sampled arc clipped at "+i);
        }
    }
    public static void main(String[] args) {
        final float pi=(float)Math.PI;
        final float tau=(float)(2*Math.PI);
        // First-quadrant sweep spans only 25% of the circle box.
        float[] quarter=new float[4];
        require(NativeSceneViewportBounds.arcCullingBounds(0,0,4,0,0,4,
                  0,pi/2f,quarter),"quarter not bounded");
        require(quarter[0]>-0.01f&&quarter[1]>-0.01f&&
                quarter[2]>=4f&&quarter[3]>=4f,
                "small arc enlarged to whole circle");
        oracle(0,0,4,0,0,4,0,pi/2f);
        oracle(0,0,1,0,0,1,
               (float)Math.toRadians(350),(float)Math.toRadians(20));
        oracle(0,0,1,0,0,1,
               (float)Math.toRadians(10),(float)Math.toRadians(-20));
        oracle(12,-5,3,2,-1,4,pi/4f,pi/2f);
        oracle(12,-5,3,2,-1,4,pi/4f,-pi/2f);
        oracle(12,-5,3,2,-1,4,pi/4f,tau);
        oracle(12,-5,3,2,-1,4,pi/4f,-tau);
        oracle(1_000_000f,-1_000_000f,3,2,-1,4,0,pi/3);
        // Deterministic diverse angles and ellipse orientations.
        for(int i=1;i<=100;i++){
            float start=(float)(i*.091-3.7);
            float sweep=(float)((i%2==0?1:-1)*(0.05+i*.044));
            oracle(i-50,i/2f,2.5f+(i%7),-3f,i%5-.5f,2f,start,sweep);
        }
        float[] unchanged={42,43,44,45};
        require(!NativeSceneViewportBounds.arcCullingBounds(
            0,0,4,0,0,4,0,0,unchanged),"zero-sweep accepted");
        require(!NativeSceneViewportBounds.arcCullingBounds(
            Float.NaN,0,4,0,0,4,0,1,unchanged),"NaN accepted");
        require(!NativeSceneViewportBounds.arcCullingBounds(
            0,0,4,0,0,4,0,1,null),"null accepted");
        require(unchanged[0]==42&&unchanged[1]==43&&
                unchanged[2]==44&&unchanged[3]==45,
                "fail-atomic culling array overwritten");
        // A multi-turn/invalid geometry never narrows the culling box.
        float[] whole=new float[4];
        require(NativeSceneViewportBounds.arcCullingBounds(
            0,0,2,0,0,2,0,8*tau,whole),"multi-turn fallback missing");
        require(whole[0]<-1.99&&whole[2]>1.99&&
                whole[1]<-1.99&&whole[3]>1.99,
                "unsupported sweep clipped by narrow box");
        System.out.println("PASS native v6 ARC tight UI culling: "
            +"signed/wrapped sweeps, ellipses, full-circle, fail-atomic,"
            +" 100 varied source-space sweeps");
    }
}
