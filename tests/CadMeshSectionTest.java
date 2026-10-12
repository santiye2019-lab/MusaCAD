import com.musa.cad.CadMeshSection;
import com.musa.cad.CadSolidKernel;
import com.musa.cad.CadSolidCommand;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

public final class CadMeshSectionTest {
    private static void check(boolean b,String message){if(!b)throw new AssertionError(message);}
    public static void main(String[]args)throws Exception{
        CadSolidKernel.Mesh box=CadSolidCommand.execute("BOX","0,0,0,10,8,5");
        float[] slice=CadMeshSection.horizontal(box.xyz,box.triangles,2.5f);
        check(slice.length>0&&slice.length%6==0,"section must contain WCS XYZ line pairs");
        for(int i=0;i<slice.length;i+=3){
            check(Math.abs(slice[i+2]-2.5f)<1e-5,"section plane Z");
            check(slice[i]>=0&&slice[i]<=10&&slice[i+1]>=0&&slice[i+1]<=8,
                "SECTION line lies outside bounding model");
        }
        check(CadMeshSection.horizontal(box.xyz,box.triangles,20).length==0,
            "unintersected Z plane invented geometry");
        ByteArrayOutputStream target=new ByteArrayOutputStream();
        CadMeshSection.writeDxf(slice,target);
        String dxf=target.toString(StandardCharsets.US_ASCII);
        check(dxf.contains("0\nLINE\n")&&dxf.contains("31\n2.5\n")
            &&dxf.endsWith("0\nEOF\n"),"SECTION output should be valid 3D DXF LINE records");
        System.out.println("3D sectional plane intersections and genuine DXF LINE output passed");
    }
}
