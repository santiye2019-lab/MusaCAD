package com.musa.cad;

/**
 * A read-only, physical-pixel camera adapter for the current Canvas 3D viewport.
 * Supplies the same column-major world-to-clip contract used by
 * musa_core_screen_scene_pick() on Android, Windows and iOS.
 *
 * The existing Canvas renderer clamps its perspective factor. The matrix is
 * exact only where the Canvas factor is not clamped; callers must NOT silently
 * use it for edited/out-of-bounds geometry or confuse a triangle with a DWG
 * entity handle. No native selection or UI behavior is enabled by this class.
 */
public final class Cad3dLegacyCameraBridge {
    private Cad3dLegacyCameraBridge() {}

    public static final class Snapshot {
        private final double[] worldToClip;
        public final int widthPx, heightPx;
        private final double centerX, centerY, centerZ, cosYaw, sinYaw,
            cosPitch, sinPitch, depthScale;

        private Snapshot(double[] matrix, int width, int height,
                double cx, double cy, double cz, double co, double si,
                double cp, double sp, double k) {
            worldToClip=matrix;
            widthPx=width;
            heightPx=height;
            centerX=cx;centerY=cy;centerZ=cz;
            cosYaw=co;sinYaw=si;cosPitch=cp;sinPitch=sp;depthScale=k;
        }

        /** Caller gets a copy; modifying it cannot corrupt the live camera. */
        public double[] worldToClipColumnMajor() { return worldToClip.clone(); }

        /**
         * Fail closed if a point enters the Canvas perspective-clamp region.
         * Check ALL relevant mesh vertices after editing before using native
         * selection; a valid matrix alone does not certify any geometry.
         */
        public boolean matchesCanvasAt(double x,double y,double z) {
            if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z))return false;
            double dx=x-centerX,dy=y-centerY,dz=z-centerZ;
            double away=sinYaw*dx+cosYaw*dy;
            double depth=sinPitch*dz+cosPitch*away;
            double denom=1d+depth/depthScale;
            if(!(denom>0d)||!Double.isFinite(denom))return false;
            double perspective=1d/denom;
            return Double.isFinite(perspective)&&perspective>=.45d&&perspective<=2.5d;
        }

        /** Scan outside the drawing/gesture thread after geometry edits. */
        public boolean matchesCanvasMesh(float[] xyz) {
            if(xyz==null||xyz.length%3!=0)return false;
            for(int i=0;i<xyz.length;i+=3)
                if(!matchesCanvasAt(xyz[i],xyz[i+1],xyz[i+2]))return false;
            return true;
        }
    }

    /**
     * Model origin and view angles are exactly the values used by Mesh3dView.
     * View width/height and touch coordinates are PHYSICAL pixels relative to
     * the View (top-left origin), not screen, dp, or parent-layout coordinates.
     * The C11 selection API must receive depth_zero_to_one=0 for this matrix.
     */
    public static Snapshot capture(float cx,float cy,float cz,float radius,
            float yaw,float pitch,float zoom,int widthPx,int heightPx) {
        if(!finite(cx)||!finite(cy)||!finite(cz)||!finite(radius)||
           !finite(yaw)||!finite(pitch)||!finite(zoom)||
           !(radius>0f)||!(zoom>0f)||widthPx<=0||heightPx<=0)
            throw new IllegalArgumentException("Invalid 3D camera or viewport");
        // Mesh3dView uses float trigonometric results and float scale.
        float co=(float)Math.cos(yaw),si=(float)Math.sin(yaw);
        float cp=(float)Math.cos(pitch),sp=(float)Math.sin(pitch);
        float scale=Math.min(widthPx,heightPx)*zoom/(radius*2.6f);
        float k=radius*2.5f;
        if(!finite(scale)||!(scale>0f)||!finite(k)||!(k>0f))
            throw new IllegalArgumentException("3D camera scale overflow");
        double[] right={co,-si,0};
        double[] up={-sp*si,-sp*co,cp};
        double[] depth={cp*si,cp*co,sp};
        double[] center={cx,cy,cz};
        double[] m=new double[16];
        // Homogeneous rows: x=a*right, y=b*up, z=2*depth/k,
        // w=1+depth/k. The -1/+1 Z unprojections are both finite,
        // so the shared C camera ray points through Canvas pixels.
        double[][] rows={right,up,depth,depth};
        double[] factors={2d*scale/widthPx,2d*scale/heightPx,2d/k,1d/k};
        for(int row=0;row<4;row++) {
            double dot=0;
            for(int axis=0;axis<3;axis++){
                m[axis*4+row]=rows[row][axis]*factors[row];
                dot+=rows[row][axis]*center[axis];
            }
            m[12+row]=(row==3?1d:0d)-dot*factors[row];
        }
        for(double value:m)if(!Double.isFinite(value))
            throw new IllegalArgumentException("3D camera matrix overflow");
        return new Snapshot(m,widthPx,heightPx,cx,cy,cz,co,si,cp,sp,k);
    }

    private static boolean finite(float value){return !Float.isNaN(value)&&!Float.isInfinite(value);}
}
