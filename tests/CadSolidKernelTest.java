import com.musa.cad.CadSolidKernel;
import com.musa.cad.CadSolidCommand;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public final class CadSolidKernelTest {
    private static void check(boolean condition,String why){if(!condition)throw new AssertionError(why);}
    private static double volume(CadSolidKernel.Mesh m){
        double sum=0;
        for(int i=0;i<m.triangles.length;i+=3){
            int a=m.triangles[i]*3,b=m.triangles[i+1]*3,c=m.triangles[i+2]*3;
            double ax=m.xyz[a],ay=m.xyz[a+1],az=m.xyz[a+2];
            double bx=m.xyz[b],by=m.xyz[b+1],bz=m.xyz[b+2];
            double cx=m.xyz[c],cy=m.xyz[c+1],cz=m.xyz[c+2];
            sum+=(ax*(by*cz-bz*cy)+ay*(bz*cx-bx*cz)+az*(bx*cy-by*cx))/6d;
        }
        return sum;
    }
    private static void closed(CadSolidKernel.Mesh mesh){
        HashMap<Long,Integer> edges=new HashMap<>();
        for(int p=0;p<mesh.triangles.length;p+=3)
            for(int j=0;j<3;j++){
                int a=mesh.triangles[p+j],b=mesh.triangles[p+(j+1)%3];
                long key=((long)Math.min(a,b)<<32)|(Math.max(a,b)&0xffffffffL);
                edges.put(key,edges.getOrDefault(key,0)+1);
            }
        for(Map.Entry<Long,Integer> e:edges.entrySet())
            check(e.getValue()==2,"unclosed/nonmanifold triangle mesh edge "+e);
    }
    private static void near(double actual,double expected,String note){
        check(Math.abs(actual-expected)<1e-3, note+": "+actual+" expected "+expected);
    }
    public static void main(String[]args)throws Exception{
        CadSolidKernel.Mesh box=CadSolidCommand.execute("BOX","0,0,0,10,8,5");
        closed(box);near(volume(box),400d,"box 10x8x5 volume");
        CadSolidKernel.Mesh ext=CadSolidCommand.execute("EXTRUDE","0,0;10,0;10,8;0,8|5");
        closed(ext);near(volume(ext),400d,"extrusion cube");
        CadSolidKernel.Mesh concave=CadSolidCommand.execute("EXTRUDE","0,0;4,0;4,1;1,1;1,3;0,3|2");
        closed(concave);near(volume(concave),12d,"concave ear clipping volume");
        CadSolidKernel.Mesh loft=CadSolidCommand.execute("LOFT",
            "0,0;4,0;4,4;0,4|0|1,1;3,1;3,3;1,3|6");
        closed(loft);near(volume(loft),56d,"two-section LOFT truncated pyramid");
        CadSolidKernel.Mesh sweep=CadSolidCommand.execute("SWEEP",
            "0,0;4,0;4,4;0,4|0,0,0;2,1,5;4,2,10");
        closed(sweep);near(volume(sweep),160d,"translation SWEEP XY section volume");
        CadSolidKernel.Mesh shell=CadSolidCommand.execute("REVOLVE","1,0;2,0;2,5;1,5|64|360");
        closed(shell);check(volume(shell)>40d&&volume(shell)<50d,
            "revolved annular cylinder volume");
        CadSolidKernel.Mesh half=CadSolidCommand.execute("REVOLVE","1,0;2,0;2,5;1,5|32|180");
        closed(half);check(volume(half)>20d&&volume(half)<25d,
            "partial revolve must include end caps");
        CadSolidKernel.Box a=new CadSolidKernel.Box(0,0,0,4,4,4);
        CadSolidKernel.Box b=new CadSolidKernel.Box(2,2,2,6,6,6);
        CadSolidKernel.Mesh union=CadSolidKernel.boxBoolean(a,b,CadSolidKernel.BooleanOp.UNION);
        CadSolidKernel.Mesh intersection=CadSolidKernel.boxBoolean(a,b,CadSolidKernel.BooleanOp.INTERSECTION);
        CadSolidKernel.Mesh difference=CadSolidKernel.boxBoolean(a,b,CadSolidKernel.BooleanOp.DIFFERENCE);
        near(volume(union),120d,"box CSG union");
        near(volume(intersection),8d,"box CSG intersection");
        near(volume(difference),56d,"box CSG difference");
        closed(union);closed(intersection);closed(difference);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        CadSolidKernel.writeDxf(box,out);
        String dxf=out.toString(StandardCharsets.US_ASCII);
        check(dxf.contains("3DFACE")&&dxf.endsWith("0\nEOF\n"),
            "3DFACE DXF output missing or incomplete");
        check(dxf.split("0\n3DFACE\n",-1).length-1==box.faceCount(),
            "DXF face count does not match mesh");
        boolean blocked=false;
        try{CadSolidCommand.execute("EXTRUDE","0,0;4,4;0,4;4,0|5");}
        catch(java.io.IOException error){blocked=true;}
        check(blocked,"self intersecting profile incorrectly accepted");
        blocked=false;
        try{CadSolidCommand.execute("REVOLVE","0,0;2,0;2,2;0,2|32|360");}
        catch(java.io.IOException error){blocked=true;}
        check(blocked,"axis singularity must not be silently generated");
        System.out.println("MusaCAD real EXTRUDE/REVOLVE/BOX boolean 3D triangle geometry and DXF writer verified");
    }
}
