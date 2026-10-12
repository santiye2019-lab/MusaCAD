import com.musa.cad.CadUcs3d;
import com.musa.cad.CadSolidKernel;
import com.musa.cad.CadSolidCommand;

public final class CadUcs3dTest {
    private static void near(double a,double b,String note){
        if(Math.abs(a-b)>1e-7)throw new AssertionError(note+" "+a+" != "+b);
    }
    private static void failed(Throwing body,String note)throws Exception{
        boolean bad=false;try{body.run();}catch(java.io.IOException expected){bad=true;}
        if(!bad)throw new AssertionError(note);
    }
    private interface Throwing { void run()throws Exception; }
    public static void main(String[]args)throws Exception{
        CadUcs3d ucs=CadUcs3d.of(
            new double[]{10,20,30},new double[]{0,1,0},new double[]{-1,0,0});
        double[] p=ucs.toWorld(2,3,4);
        near(p[0],7,"world x");near(p[1],22,"world y");near(p[2],34,"world z");
        double[] q=ucs.toLocal(p[0],p[1],p[2]);
        near(q[0],2,"roundtrip x");near(q[1],3,"roundtrip y");near(q[2],4,"roundtrip z");
        double[] normal=ucs.normalToWorld(0,0,1);
        near(normal[0],0,"normal x");near(normal[1],0,"normal y");near(normal[2],1,"normal z");
        CadSolidKernel.Mesh model=CadSolidCommand.execute("BOX","0,0,0,2,3,4");
        CadSolidKernel.Mesh moved=CadSolidCommand.applyUcs(model,"10,20,30|0,1,0|-1,0,0");
        if(moved.faceCount()!=model.faceCount())throw new AssertionError("UCS lost triangles");
        double[] expected=ucs.toWorld(model.xyz[0],model.xyz[1],model.xyz[2]);
        for(int i=0;i<3;i++)near(moved.xyz[i],expected[i],"mesh UCS transform");
        failed(()->CadUcs3d.of(new double[]{0,0,0},
            new double[]{1,0,0},new double[]{2,0,0}),"parallel axes accepted");
        failed(()->CadSolidCommand.applyUcs(null,"0,0,0|1,0,0|0,1,0"),"missing model accepted");
        System.out.println("3D UCS forward/inverse, mesh transforms and invalid axis rejection passed");
    }
}
