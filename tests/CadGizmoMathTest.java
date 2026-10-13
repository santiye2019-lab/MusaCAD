import com.musa.cad.CadUcsFrame;
import com.musa.cad.CadGizmoMath;

public final class CadGizmoMathTest {
    private static void near(double got,double want){
        if(!Double.isFinite(got)||Math.abs(got-want)>1e-8)
            throw new AssertionError(got+" != "+want);
    }
    private static void ok(boolean pass,String message){if(!pass)throw new AssertionError(message);}
    public static void main(String[] args){
        CadUcsFrame world=CadUcsFrame.world();
        double[] x=CadGizmoMath.axisDirection(world,0,0,0);
        double[] y=CadGizmoMath.axisDirection(world,1,0,0);
        double[] z=CadGizmoMath.axisDirection(world,2,0,0);
        near(x[0],1);near(x[1],0);
        near(y[0],0);near(y[1],0); // WCS Y parallel to camera direction at yaw=0.
        near(z[0],0);near(z[1],-1);
        double[][] dirs={x,y,z};
        ok(CadGizmoMath.pick(58,1,dirs,58,16)==0,"X arrow tip");
        ok(CadGizmoMath.pick(1,-58,dirs,58,16)==2,"Z arrow tip");
        ok(CadGizmoMath.pick(100,100,dirs,58,16)==-1,"outside 3 arrows");
        ok(CadGizmoMath.pick(0,0,dirs,58,16)==-1,"vertex itself not drag handle");
        ok(CadGizmoMath.pick(0,58,dirs,58,16)==-1,"hidden axis cannot be chosen");
        near(CadGizmoMath.drawingDistance(100,0,1,0,10),10);
        near(CadGizmoMath.drawingDistance(0,-25,0,-1,5),5);
        near(CadGizmoMath.drawingDistance(10,15,1,0,5),2);
        ok(Double.isNaN(CadGizmoMath.drawingDistance(100,0,0,0,10)),
            "nearly invisible axis must not move geometry");
        ok(Double.isNaN(CadGizmoMath.drawingDistance(100,0,0.05,0,10)),
            "foreshortened axis must be rejected");
        ok(Double.isNaN(CadGizmoMath.drawingDistance(100,0,1,0,0)),
            "invalid screen scale blocked");
        ok(Double.isNaN(CadGizmoMath.drawingDistance(Double.NaN,0,1,0,10)),
            "NaN pointer blocked");
        double[] tilted=CadGizmoMath.axisDirection(
            CadUcsFrame.front(),1,0,0);
        near(tilted[0],0);near(tilted[1],-1);
        double[] rotated=CadGizmoMath.axisDirection(world,0,Math.PI/2,0);
        near(rotated[0],0);near(rotated[1],0);
        System.out.println("UCS axis gizmo: camera projection, arrow hits, parallel-axis guard, distance passed");
    }
}
