import com.musa.cad.Cad3dLegacyCameraBridge;

public final class Cad3dLegacyCameraBridgeTest {
    private static void check(boolean value){if(!value)throw new AssertionError();}
    private static void close(double a,double b,double allowed){
        check(Double.isFinite(a)&&Math.abs(a-b)<=allowed);
    }
    private static double[] nativeMatrixProjection(double[] m,double x,double y,double z,
            int width,int height){
        double[] p={x,y,z,1},h=new double[4];
        for(int row=0;row<4;row++)for(int col=0;col<4;col++)h[row]+=m[col*4+row]*p[col];
        check(h[3]>0);
        return new double[]{width*(1+h[0]/h[3])*.5d,height*(1-h[1]/h[3])*.5d};
    }
    private static double[] canvasProjection(float x,float y,float z,float cx,float cy,
            float cz,float radius,float yaw,float pitch,float zoom,int width,int height){
        float co=(float)Math.cos(yaw),si=(float)Math.sin(yaw);
        float cp=(float)Math.cos(pitch),sp=(float)Math.sin(pitch);
        float scale=Math.min(width,height)*zoom/(radius*2.6f);
        float dx=x-cx,dy=y-cy,dz=z-cz;
        float right=co*dx-si*dy,away=si*dx+co*dy;
        float up=cp*dz-sp*away,depth=sp*dz+cp*away;
        float perspective=Math.max(.45f,Math.min(2.5f,1f/(1f+depth/(radius*2.5f))));
        return new double[]{width*.5f+right*scale*perspective,
                            height*.5f-up*scale*perspective};
    }
    private static void camera(float cx,float cy,float cz,float radius,
            float yaw,float pitch,float zoom,int width,int height){
        Cad3dLegacyCameraBridge.Snapshot snap=Cad3dLegacyCameraBridge.capture(
                cx,cy,cz,radius,yaw,pitch,zoom,width,height);
        double[] m=snap.worldToClipColumnMajor();
        m[0]=100000; // caller cannot mutate stored projection
        check(snap.worldToClipColumnMajor()[0]!=100000);
        float[][] points={{cx,cy,cz},{cx+.2f*radius,cy+.1f*radius,cz+.2f*radius},
            {cx-.3f*radius,cy+.2f*radius,cz-.1f*radius}};
        for(float[] p:points){
            check(snap.matchesCanvasAt(p[0],p[1],p[2]));
            double[] expected=canvasProjection(p[0],p[1],p[2],cx,cy,cz,
                    radius,yaw,pitch,zoom,width,height);
            double[] projected=nativeMatrixProjection(snap.worldToClipColumnMajor(),
                    p[0],p[1],p[2],width,height);
            close(projected[0],expected[0],.05);
            close(projected[1],expected[1],.05);
        }
        check(snap.matchesCanvasMesh(new float[]{cx,cy,cz,cx+.1f*radius,cy,cz}));
    }
    private static void rejects(Runnable r){
        try{r.run();throw new AssertionError("Expected camera rejection");}
        catch(IllegalArgumentException expected){}
    }
    public static void main(String[] args){
        camera(0,0,0,50,.65f,.32f,1f,1080,2240);
        camera(10000,-20000,300,180,0,0,2f,1920,1080);
        camera(-200,-400,0,100,0,-(float)Math.PI/2, .25f,1200,800);
        camera(0,0,0,50,(float)Math.PI/2,0,25f,800,1600);
        Cad3dLegacyCameraBridge.Snapshot s=Cad3dLegacyCameraBridge.capture(
                0,0,0,10,0,0,1,800,600);
        check(!s.matchesCanvasAt(0,-17,0)); // clamps near model limit
        check(!s.matchesCanvasMesh(new float[]{0,0,0,0,-17,0}));
        check(!s.matchesCanvasMesh(new float[]{0,0}));
        check(!s.matchesCanvasAt(Double.NaN,0,0));
        rejects(()->Cad3dLegacyCameraBridge.capture(0,0,0,0,0,0,1,100,100));
        rejects(()->Cad3dLegacyCameraBridge.capture(0,0,0,1,0,0,1,0,100));
        rejects(()->Cad3dLegacyCameraBridge.capture(0,0,0,1,Float.NaN,0,1,100,100));
        System.out.println("Camera-to-core world-to-clip projection regression PASS");
    }
}
