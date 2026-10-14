import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** DXF parser -> indexed mesh -> real triangle area, not invented volume. */
public final class Cad3dSurfaceDxfIntegrationTest {
    private static void need(boolean condition,String why){
        if(!condition)throw new AssertionError(why);
    }
    private static Cad3dSurfaceMetrics.Result measure(String source)throws Exception{
        File drawing=File.createTempFile("musacad-3d-area-", ".dxf");
        try{
            Files.writeString(drawing.toPath(),source,StandardCharsets.US_ASCII);
            Dxf3dMesh mesh=Dxf3dMesh.read(drawing);
            need(!mesh.solidProxy&&mesh.unsupportedSolids==0,"simple DXF face is not ACIS");
            return Cad3dSurfaceMetrics.measure(mesh.xyz,mesh.triangles);
        }finally{Files.deleteIfExists(drawing.toPath());}
    }
    public static void main(String[] args)throws Exception{
        String start="0\nSECTION\n2\nENTITIES\n";
        String end="0\nENDSEC\n0\nEOF\n";
        Cad3dSurfaceMetrics.Result triangle=measure(start+
            "0\n3DFACE\n10\n0\n20\n0\n30\n0\n"+
            "11\n3\n21\n0\n31\n0\n"+
            "12\n0\n22\n4\n32\n0\n"+end);
        need(triangle.triangleCount==1,"one DXF 3DFACE is one triangle");
        need(Math.abs(triangle.area-6.0)<1e-9,"real DXF right triangle area=6");
        Cad3dSurfaceMetrics.Result lineOnly=measure(start+
            "0\nLINE\n10\n0\n20\n0\n30\n0\n"+
            "11\n3\n21\n4\n31\n5\n"+end);
        need(lineOnly.triangleCount==0&&lineOnly.area==0,
            "a 3D LINE must not be mistaken for a surface");
        System.out.println("Native DXF triangle surface area end-to-end PASS");
    }
}
