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
        testLineAndPolyline();
        testInsertedBlockLine();
    }

    private static void testLineAndPolyline() throws Exception {
        File file=File.createTempFile("mesh3d-lines-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"
                +"0\nLINE\n10\n1\n20\n2\n30\n3\n11\n4\n21\n6\n31\n8\n"
                +"0\nPOLYLINE\n70\n9\n"
                +"0\nVERTEX\n10\n0\n20\n0\n30\n0\n"
                +"0\nVERTEX\n10\n5\n20\n0\n30\n2\n"
                +"0\nVERTEX\n10\n5\n20\n5\n30\n4\n"
                +"0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.triangles.length!=0)throw new AssertionError("line-only model must not invent surfaces");
            if(mesh.xyz.length!=15)throw new AssertionError("LINE + 3D POLYLINE vertex count");
            if(mesh.edges.length!=8)throw new AssertionError("LINE + closed 3D POLYLINE edges");
            if(mesh.xyz[2]!=3f||mesh.xyz[5]!=8f||mesh.xyz[11]!=2f||mesh.xyz[14]!=4f)
                throw new AssertionError("3D line/polyline Z coordinates");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testInsertedBlockLine() throws Exception {
        File file=File.createTempFile("mesh3d-block-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"
                +"0\nBLOCK\n2\nPIPE3D\n10\n0\n20\n0\n30\n0\n"
                +"0\nLINE\n10\n0\n20\n0\n30\n1\n11\n2\n21\n0\n31\n3\n"
                +"0\nENDBLK\n0\nENDSEC\n"
                +"0\nSECTION\n2\nENTITIES\n"
                +"0\nINSERT\n2\nPIPE3D\n10\n10\n20\n20\n30\n0\n50\n90\n"
                +"0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=6||mesh.edges.length!=2)throw new AssertionError("inserted block line geometry");
            if(Math.abs(mesh.xyz[0]-10f)>1e-5||Math.abs(mesh.xyz[1]-20f)>1e-5||mesh.xyz[2]!=1f)
                throw new AssertionError("insert transform start");
            if(Math.abs(mesh.xyz[3]-10f)>1e-5||Math.abs(mesh.xyz[4]-22f)>1e-5||mesh.xyz[5]!=3f)
                throw new AssertionError("insert transform end");
            if(mesh.sourceLines[0]!=-1||mesh.sourceLines[1]!=-1)throw new AssertionError("inserted geometry must be read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static String vertex(int x,int y,int z){return "0\nVERTEX\n70\n192\n10\n"+x+"\n20\n"+y+"\n30\n"+z+"\n";}
}
