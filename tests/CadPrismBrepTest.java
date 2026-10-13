import com.musa.cad.CadPrismBrep;
import java.io.StringWriter;

/** Deterministic, pure-Java regression for the narrow polygonal B-Rep primitive. */
public class CadPrismBrepTest {
    private static void eq(double actual,double expected){
        if(Math.abs(actual-expected)>1e-7)throw new AssertionError("Expected "+expected+", got "+actual);
    }
    private static void fail(Runnable r){
        try {r.run();}catch(IllegalArgumentException expected){return;}
        throw new AssertionError("Expected fail-closed geometry validation");
    }
    private static int count(String s,String needle){
        int n=0,p=0;while((p=s.indexOf(needle,p))>=0){n++;p+=needle.length();}return n;
    }
    public static void main(String[]args)throws Exception{
        double[] square={0,0,2,0,2,2,0,2};
        CadPrismBrep.Solid s=CadPrismBrep.extrude(square,1,3);
        eq(s.volume,12);
        if(!s.isWatertight()||s.ringVertices!=4||s.xyz.length!=24||
                s.triangles.length!=36)throw new AssertionError("Square prism manifold");
        CadPrismBrep.Solid negative=CadPrismBrep.extrude(square,4,-3);
        eq(negative.volume,12);
        if(!negative.isWatertight())throw new AssertionError("Negative extrusion manifold");
        CadPrismBrep.Solid cw=CadPrismBrep.extrude(new double[]{0,2,2,2,2,0,0,0},1,3);
        eq(cw.volume,12);
        if(!cw.isWatertight())throw new AssertionError("Reverse ring manifold");
        CadPrismBrep.Solid translated=CadPrismBrep.extrude(
                new double[]{100000,100000,100002,100000,100002,100002,100000,100002},
                100000,3);
        eq(translated.volume,12);
        double[] concave={0,0,3,0,3,1,1,1,1,3,0,3};
        CadPrismBrep.Solid l=CadPrismBrep.extrude(concave,0,2);
        eq(l.volume,10);
        if(!l.isWatertight()||l.triangles.length!=60)
            throw new AssertionError("Concave polygon extrusion failed");
        fail(()->CadPrismBrep.extrude(new double[]{0,0,2,2,0,2,2,0},0,2));
        fail(()->CadPrismBrep.extrude(new double[]{0,0,1,0,1,0,0,1},0,2));
        fail(()->CadPrismBrep.extrude(square,0,0));
        fail(()->CadPrismBrep.extrude(square,0,Double.NaN));
        fail(()->CadPrismBrep.extrude(new double[]{0,0,1,0,Double.POSITIVE_INFINITY,2},0,2));
        StringWriter writer=new StringWriter();s.writeDxf(writer,"MUSACAD_3D");
        String dxf=writer.toString();
        if(!dxf.contains("AC1015")||!dxf.endsWith("0\nEOF\n")||
                count(dxf,"0\n3DFACE\n")!=12)
            throw new AssertionError("3DFACE DXF export contract");
        try{s.writeDxf(new StringWriter(),"BAD\n0\nLINE");throw new AssertionError("DXF injection accepted");}
        catch(IllegalArgumentException expected){}
        System.out.println("CadPrismBrep watertight extrusion, concavity, invalid input and DXF tests passed.");
    }
}
