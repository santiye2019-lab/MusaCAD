import com.musa.cad.DxfViewport;

public class DxfViewportTest {
    private static void close(double a,double b){if(Math.abs(a-b)>1e-6)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        DxfViewport.Spec v=DxfViewport.of(100,50,80,40,10,20,20,0,2,1,0,0,1);
        if(!v.supported())throw new AssertionError("top viewport");
        double[] p=DxfViewport.point(v,10,20);close(p[0],100);close(p[1],50);
        p=DxfViewport.point(v,30,20);close(p[0],140);close(p[1],50);
        close(v.left(),60);close(v.right(),140);close(v.bottom(),30);close(v.top(),70);
        if(!DxfViewport.contains(v,60,30,0)||!DxfViewport.contains(v,140,70,0))throw new AssertionError("viewport edges");
        if(DxfViewport.contains(v,140.1,50,0))throw new AssertionError("outside viewport");
        if(!DxfViewport.contains(v,140.1,50,.2))throw new AssertionError("viewport tolerance");

        DxfViewport.Spec rotated=DxfViewport.of(0,0,100,100,0,0,100,Math.PI/2,3,1,0,0,1);
        p=DxfViewport.point(rotated,10,0);close(p[0],0);close(p[1],-10);

        DxfViewport.Spec targeted=DxfViewport.of(25,30,100,80,0,0,40,0,4,1,0,0,1,5,7,0);
        p=DxfViewport.point(targeted,5,7);close(p[0],25);close(p[1],30);
        p=DxfViewport.point(targeted,15,7);close(p[0],45);close(p[1],30);

        DxfViewport.Spec oblique=DxfViewport.of(0,0,100,100,0,0,100,0,5,1,1,1,1,0,0,0);
        if(!oblique.supported())throw new AssertionError("oblique viewport");
        p=DxfViewport.point(oblique,1,0);close(p[0],-1/Math.sqrt(2));close(p[1],-1/Math.sqrt(6));
        p=DxfViewport.point(oblique,0,1);close(p[0],1/Math.sqrt(2));close(p[1],-1/Math.sqrt(6));

        DxfViewport.Spec obliqueTarget=DxfViewport.of(10,20,100,100,0,0,50,0,6,1,1,1,1,3,4,5);
        double uz=2/Math.sqrt(6);
        p=DxfViewport.point(obliqueTarget,3,4);close(p[0],10);close(p[1],20-2*5*uz);

        if(DxfViewport.of(0,0,10,10,0,0,10,0,1,1,0,0,1).supported())throw new AssertionError("overall paper viewport");
        if(DxfViewport.of(0,0,10,10,0,0,10,0,2,0,0,0,1).supported())throw new AssertionError("off viewport");
        if(DxfViewport.of(0,0,10,10,0,0,10,0,2,1,1,0,0).supported())throw new AssertionError("pure side view");
        if(DxfViewport.of(0,0,10,10,0,0,10,0,2,1,0,0,0).supported())throw new AssertionError("zero direction");
        System.out.println("24 DXF viewport geometry and clipping cases passed");
    }
}
