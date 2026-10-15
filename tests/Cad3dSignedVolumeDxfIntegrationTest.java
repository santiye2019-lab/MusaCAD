import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Real DXF POLYFACE decoder -> indexed triangles -> signed mesh volume. */
public final class Cad3dSignedVolumeDxfIntegrationTest {
    private static void need(boolean ok,String message){
        if(!ok)throw new AssertionError(message);
    }
    private static String vertex(int x,int y,int z){
        return "0\nVERTEX\n70\n192\n10\n"+x+"\n20\n"+y+"\n30\n"+z+"\n";
    }
    private static String face(int a,int b,int c){
        return "0\nVERTEX\n70\n128\n71\n"+a+
               "\n72\n"+b+"\n73\n"+c+"\n";
    }

    public static void main(String[] args)throws Exception{
        File input=File.createTempFile("musacad-polyface-volume-", ".dxf");
        try{
            String source="0\nSECTION\n2\nENTITIES\n0\nPOLYLINE\n70\n64\n"+
                vertex(0,0,0)+vertex(1,0,0)+vertex(0,1,0)+vertex(0,0,1)+
                face(1,3,2)+face(1,2,4)+face(1,4,3)+face(2,3,4)+
                "0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
            Files.writeString(input.toPath(),source,StandardCharsets.US_ASCII);
            Dxf3dMesh mesh=Dxf3dMesh.read(input);
            need(mesh.xyz.length==12&&mesh.triangles.length==12,
                    "actual DXF polyface must keep shared face indices");
            Cad3dMeshTopology.Result audit=
                    Cad3dMeshTopology.inspect(mesh.xyz,mesh.triangles);
            need(audit.checked&&audit.closedOrientedByIndex,
                    "real DXF polyface must pass indexed edge topology");
            Cad3dSignedVolume.Result result=Cad3dSignedVolume.measure(
                    mesh.xyz,mesh.triangles,audit);
            need(result.available&&Math.abs(result.signedVolume-1d/6d)<1e-12,
                    "real DXF tetra volume must be 1/6 drawing unit cubed");
        }finally{
            Files.deleteIfExists(input.toPath());
        }
        System.out.println("DXF polyface to signed 3D mesh volume PASS");
    }
}
