import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import com.musa.cad.Dxf3dEditor;

public final class Dxf3dMeshTest {
    public static void main(String[] args) throws Exception {
        File file=File.createTempFile("mesh3d-", ".dxf");
        try {
            String dxf="0\nSECTION\n2\nENTITIES\n0\nPOLYLINE\n70\n64\n"
                + vertex(0,0,0)+vertex(3,0,0)+vertex(0,4,0)
                +"0\nVERTEX\n70\n128\n71\n1\n72\n2\n73\n3\n0\nSEQEND\n"
                +"0\n3DFACE\n10\n0\n20\n0\n30\n5\n11\n3\n21\n0\n31\n5\n12\n0\n22\n4\n32\n5\n"
                +"0\nENDSEC\n0\nEOF\n";
            byte[] prefix=dxf.replace("0\nENDSEC\n0\nEOF\n","").replace("\n","\r\n").getBytes(StandardCharsets.UTF_8);
            ByteArrayOutputStream input=new ByteArrayOutputStream();input.write(prefix);
            input.write("0\r\nTEXT\r\n1\r\n".getBytes(StandardCharsets.US_ASCII));input.write(0xdd);input.write("\r\n0\r\nENDSEC\r\n0\r\nEOF\r\n".getBytes(StandardCharsets.US_ASCII));
            Files.write(file.toPath(),input.toByteArray());
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=18||mesh.triangles.length!=6||mesh.edges.length!=12)throw new AssertionError("mesh geometry");
            if(mesh.xyz[8]!=0||mesh.xyz[11]!=5||mesh.xyz[14]!=5||mesh.radius<=0)throw new AssertionError("Z coordinates");
            double dx=mesh.xyz[3]-mesh.xyz[0],dy=mesh.xyz[4]-mesh.xyz[1],dz=mesh.xyz[5]-mesh.xyz[2];
            if(Math.abs(Math.sqrt(dx*dx+dy*dy+dz*dz)-3)>1e-6)throw new AssertionError("distance");
            float[] original=mesh.xyz.clone();mesh.xyz[2]=7;mesh.xyz[14]=8;
            ByteArrayOutputStream target=new ByteArrayOutputStream();Dxf3dEditor.write(file,target,mesh,original);
            byte[] saved=target.toByteArray();boolean preserved=false;
            for(int i=0;i<saved.length;i++){if(saved[i]==(byte)0xdd)preserved=true;if(saved[i]=='\n'&&(i==0||saved[i-1]!='\r'))throw new AssertionError("line ending changed");}
            if(!preserved)throw new AssertionError("drawing text encoding changed");
            Files.write(file.toPath(),target.toByteArray());Dxf3dMesh edited=Dxf3dMesh.read(file);
            if(edited.xyz[2]!=7||edited.xyz[14]!=8||edited.xyz[11]!=5)throw new AssertionError("saved coordinates");
        } finally {Files.deleteIfExists(file.toPath());}
    }
    private static String vertex(int x,int y,int z){return "0\nVERTEX\n70\n192\n10\n"+x+"\n20\n"+y+"\n30\n"+z+"\n";}
}
