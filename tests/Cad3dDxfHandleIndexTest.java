import com.musa.cad.Cad3dDxfHandleIndex;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.attribute.FileTime;

/** No Android device or original-DWG parity is asserted by this JVM test. */
public final class Cad3dDxfHandleIndexTest {
    private static void check(boolean ok) {
        if(!ok)throw new AssertionError();
    }
    private static void rejected(String dxf) throws Exception {
        File f=File.createTempFile("musacad-bad-handle-", ".dxf");
        try {
            Files.writeString(f.toPath(),dxf,StandardCharsets.UTF_8);
            try {
                Cad3dDxfHandleIndex.read(f);
                throw new AssertionError("Malformed/duplicate handle accepted");
            }catch(IOException expected) {}
        }finally {Files.deleteIfExists(f.toPath());}
    }
    private static String line(String handle,int x) {
        return "0\nLINE\n"+(handle==null?"":"5\n"+handle+"\n")+
            "10\n"+x+"\n20\n0\n30\n0\n11\n"+(x+2)+"\n21\n0\n31\n0\n";
    }
    private static String face(String handle) {
        return "0\n3DFACE\n"+(handle==null?"":"5\n"+handle+"\n")+
            "10\n0\n20\n0\n30\n2\n11\n2\n21\n0\n31\n2\n"+
            "12\n2\n22\n2\n32\n2\n13\n0\n23\n2\n33\n2\n";
    }
    private static String drawing(String body) {
        return "0\nSECTION\n2\nENTITIES\n"+body+"0\nENDSEC\n0\nEOF\n";
    }
    public static void main(String[] args) throws Exception {
        File f=File.createTempFile("musacad-handle-provenance-", ".dxf");
        try {
            String source=drawing(line("00a1",0)+face("B2")+line(null,4));
            Files.writeString(f.toPath(),source,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(f);
            try(com.musa.cad.DxfStream stream=new com.musa.cad.DxfStream(f,StandardCharsets.UTF_8)) {
                com.musa.cad.DxfBlocks.Record record;
                while((record=stream.next())!=null)
                    System.out.println("DXF handle fixture record: "+record.type+
                        " source="+record.sourceStart+" handle="+record.text(5,"<none>"));
            }
            Cad3dDxfHandleIndex index=Cad3dDxfHandleIndex.read(f);
            check(index.indexedRecordCount()==2);
            check(mesh.xyz.length/3==8);
            check(mesh.triangles.length/3==2);
            Cad3dDxfHandleIndex.Candidate line=index.forVertex(mesh,0);
            check(line!=null&&"A1".equals(line.dxfHandle)&&
                    "LINE".equals(line.entityType));
            check(index.forVertex(mesh,1)==line);
            check(index.forEdge(mesh,0)==line);
            Cad3dDxfHandleIndex.Candidate face=index.forTriangle(mesh,0);
            check(face!=null&&"B2".equals(face.dxfHandle)&&
                    "3DFACE".equals(face.entityType));
            check(index.forTriangle(mesh,1)==face);
            // Unsigned record positions and triangle indices are NOT handles.
            check(index.forVertex(mesh,6)==null);
            check(index.forVertex(mesh,-1)==null);
            check(index.forVertex(mesh,8)==null);
            check(index.forTriangle(mesh,2)==null);
            check(index.forEdge(mesh,-1)==null);
            index.requireUnchanged(f);

            // Same file length and timestamp are NOT sufficient for reuse.
            FileTime timestamp=Files.getLastModifiedTime(f.toPath());
            Files.writeString(f.toPath(),source.replace("00a1","00c3"),
                    StandardCharsets.UTF_8);
            Files.setLastModifiedTime(f.toPath(),timestamp);
            boolean changed=false;
            try {index.requireUnchanged(f);} catch(IOException expected) {changed=true;}
            check(changed);
        } finally {Files.deleteIfExists(f.toPath());}

        rejected(drawing(line("A1",0)+line("0a1",4)));
        rejected(drawing(line("0",0)));
        rejected(drawing(line("NOHANDLE",0)));
        rejected(drawing(line("ABCDEF12345678901",0)));
        rejected(drawing("0\nLINE\n5\nA1\n5\nB2\n"+
                "10\n0\n20\n0\n11\n2\n21\n0\n"));

        // Nested INSERT expands a child LINE into world coordinates, but
        // sourceLines=-1; the block definition's handle is not a unique
        // original DWG instance ID and MUST NOT be exposed as one.
        File nested=File.createTempFile("musacad-handle-block-", ".dxf");
        try {
            String s="0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n2\nB\n10\n0\n20\n0\n"+
                line("AB",0)+"0\nENDBLK\n0\nENDSEC\n"+
                "0\nSECTION\n2\nENTITIES\n0\nINSERT\n5\nCC\n"+
                "2\nB\n10\n10\n20\n0\n0\nENDSEC\n0\nEOF\n";
            Files.writeString(nested.toPath(),s,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(nested);
            Cad3dDxfHandleIndex index=Cad3dDxfHandleIndex.read(nested);
            check(mesh.xyz.length==6&&index.indexedRecordCount()==0);
            check(index.forVertex(mesh,0)==null);
            check(index.forEdge(mesh,0)==null);
        }finally {Files.deleteIfExists(nested.toPath());}
        System.out.println("MusaCAD DXF group-5 read-only source provenance PASS");
    }
}
