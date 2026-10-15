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
        test3dFaceInvisibleEdgesAndSafeRoundtrip();
        testPolygonMesh();
        testInsertedBlockLine();
        testAxialZBlockHeightAndNestedScaling();
        testNegativeZMinsert();
        testTiltedOcsBlockPlacement();
        testNestedTiltedMinsert();
        testInvalidTiltNormalDoesNotLeakGeometry();
        testMissingAndExternalBlocksFlagIncomplete3d();
        testRejectedOverflowedHeightTransform();
        testFlat2dPlanMetadata();
        testAcisSatSolidMesh();
        testAcisSolidBoundsProxy();
        testMissing3dTagDoesNotWritePartialFile();
    }

    private static void test3dFaceInvisibleEdgesAndSafeRoundtrip() throws Exception {
        File file=File.createTempFile("mesh3d-face-visibility-", ".dxf");
        File saved=File.createTempFile("mesh3d-face-saved-", ".dxf");
        try {
            // Four corners are triangulated for shading, but only the CAD
            // face outline is displayed. Bits 1 and 4 hide edge 1-2 and 3-4.
            String quad="0\nSECTION\n2\nENTITIES\n"+
                "0\n3DFACE\n70\n5\n"+
                "10\n0\n20\n0\n30\n0\n"+
                "11\n4\n21\n0\n31\n0\n"+
                "12\n4\n22\n3\n32\n0\n"+
                "13\n0\n23\n3\n33\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),quad,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.triangles.length!=6||mesh.edges.length!=4)
                throw new AssertionError("quad must keep two shaded faces and two visible border edges");
            if(mesh.edges[0]!=1||mesh.edges[1]!=2||
               mesh.edges[2]!=3||mesh.edges[3]!=0)
                throw new AssertionError("3DFACE group-70 invisible-edge mask incorrectly decoded");
            float[] opened=mesh.xyz.clone();
            mesh.xyz[8]=7f;
            ByteArrayOutputStream written=new ByteArrayOutputStream();
            Dxf3dEditor.write(file,written,mesh,opened);
            String exported=written.toString(StandardCharsets.UTF_8);
            if(!exported.contains("70\n5\n"))
                throw new AssertionError("save must preserve 3DFACE hidden-edge mask");
            Files.write(saved.toPath(),written.toByteArray());
            Dxf3dMesh reopened=Dxf3dMesh.read(saved);
            if(reopened.xyz[8]!=7f||reopened.triangles.length!=6||
               !java.util.Arrays.equals(mesh.edges,reopened.edges))
                throw new AssertionError("3DFACE saved XY/Z and visible outline must round-trip");

            // No fourth corner means 4=3; bit 8 hides the 3->1 edge.
            String triangle="0\nSECTION\n2\nENTITIES\n"+
                "0\n3DFACE\n70\n8\n"+
                "10\n0\n20\n0\n30\n0\n"+
                "11\n2\n21\n0\n31\n0\n"+
                "12\n0\n22\n2\n32\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),triangle,StandardCharsets.UTF_8);
            Dxf3dMesh tri=Dxf3dMesh.read(file);
            if(tri.triangles.length!=3||tri.edges.length!=4||
               tri.edges[0]!=0||tri.edges[1]!=1||
               tri.edges[2]!=1||tri.edges[3]!=2)
                throw new AssertionError("triangular 3DFACE edge 4->1 must honor visibility flag");

            // Even with all four visible outline edges, the artificial
            // triangulation diagonal must never appear as a wireframe edge.
            Files.writeString(file.toPath(),quad.replace("70\n5\n","70\n0\n"),StandardCharsets.UTF_8);
            Dxf3dMesh outline=Dxf3dMesh.read(file);
            if(outline.triangles.length!=6||outline.edges.length!=8)
                throw new AssertionError("visible quad must have 4 perimeter edges, not 5");
            for(int i=0;i+1<outline.edges.length;i+=2)
                if((outline.edges[i]==0&&outline.edges[i+1]==2)||
                   (outline.edges[i]==2&&outline.edges[i+1]==0))
                    throw new AssertionError("wireframe exposed synthetic diagonal");

            Files.writeString(file.toPath(),quad.replace("70\n5\n","70\n16\n"),StandardCharsets.UTF_8);
            try {
                Dxf3dMesh.read(file);
                throw new AssertionError("invalid 3DFACE edge mask should fail closed");
            } catch(java.io.IOException expected) {}
        } finally {
            Files.deleteIfExists(file.toPath());
            Files.deleteIfExists(saved.toPath());
        }
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

    private static void testPolygonMesh() throws Exception {
        File file=File.createTempFile("mesh3d-polygon-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"
                +"0\nPOLYLINE\n70\n16\n71\n2\n72\n2\n"
                +meshVertex(0,0,0)+meshVertex(4,0,0)+meshVertex(0,3,1)+meshVertex(4,3,2)
                +"0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=12)throw new AssertionError("polygon mesh vertex count");
            if(mesh.triangles.length!=6)throw new AssertionError("polygon mesh must create two triangles");
            if(mesh.edges.length!=10)throw new AssertionError("polygon mesh edge count");
            if(mesh.xyz[8]!=1f||mesh.xyz[11]!=2f)throw new AssertionError("polygon mesh Z coordinates");
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

    private static void assertNear(float actual,double expected,String scenario){
        if(!Float.isFinite(actual)||Math.abs(actual-expected)>0.0001)
            throw new AssertionError(scenario+": "+actual+" != "+expected);
    }

    private static void testAxialZBlockHeightAndNestedScaling() throws Exception {
        File file=File.createTempFile("mesh3d-zscale-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nLEVEL\n10\n0\n20\n0\n30\n2\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n3\n11\n2\n21\n0\n31\n5\n"+
                "0\nENDBLK\n"+
                "0\nBLOCK\n2\nPARENT\n10\n0\n20\n0\n30\n0\n"+
                "0\nINSERT\n2\nLEVEL\n10\n0\n20\n0\n30\n10\n43\n2\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nPARENT\n10\n10\n20\n20\n30\n100\n43\n3\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=6||result.edges.length!=2)
                throw new AssertionError("nested block vertex count");
            assertNear(result.xyz[2],136,"nested block base Z and Z-scale");
            assertNear(result.xyz[5],148,"nested block second Z");
            assertNear(result.xyz[0],10,"nested block X");
            if(result.sourceLines[0]!=-1||result.sourceLines[1]!=-1)
                throw new AssertionError("expanded INSERT must remain read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testNegativeZMinsert() throws Exception {
        File file=File.createTempFile("mesh3d-negative-z-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nDUP\n10\n0\n20\n0\n30\n0\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n1\n11\n1\n21\n0\n31\n2\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nDUP\n10\n10\n20\n20\n30\n10\n"+
                "43\n2\n70\n2\n71\n1\n44\n5\n230\n-1\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=12||result.edges.length!=4)
                throw new AssertionError("MINSERT must emit two 3D lines");
            assertNear(result.xyz[0],-10,"negative-Z X mirroring");
            assertNear(result.xyz[2],-12,"negative-Z first Z");
            assertNear(result.xyz[5],-14,"negative-Z second Z");
            assertNear(result.xyz[6],-15,"negative-Z X array spacing");
            assertNear(result.xyz[8],-12,"negative-Z repeated Z");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testRejectedOverflowedHeightTransform() throws Exception {
        File file=File.createTempFile("mesh3d-overflow-z-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nBAD\n30\n1e308\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n1\n11\n1\n21\n0\n31\n2\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nBAD\n30\n-1e308\n43\n1e308\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n1\n11\n1\n21\n0\n31\n2\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=6||result.edges.length!=2)
                throw new AssertionError("overflowed INSERT must not emit invalid vertices");
            if(result.skippedBlockReferences!=1)
                throw new AssertionError("overflowed 3D block must be counted as incomplete");
        }finally{Files.deleteIfExists(file.toPath());}
    }


    private static void testTiltedOcsBlockPlacement() throws Exception {
        File file=File.createTempFile("mesh3d-tilted-ocs-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nAXIS\n10\n0\n20\n0\n30\n0\n"+
                "0\nLINE\n10\n1\n20\n2\n30\n3\n11\n4\n21\n5\n31\n6\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nAXIS\n10\n10\n20\n20\n30\n7\n"+
                "210\n0\n220\n1\n230\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=6||result.edges.length!=2)
                throw new AssertionError("tilted INSERT lost block geometry");
            assertNear(result.xyz[0],-11,"tilted point X");
            assertNear(result.xyz[1],10,"tilted point Y");
            assertNear(result.xyz[2],22,"tilted point Z");
            assertNear(result.xyz[3],-14,"tilted endpoint X");
            assertNear(result.xyz[4],13,"tilted endpoint Y");
            assertNear(result.xyz[5],25,"tilted endpoint Z");
            if(result.sourceLines[0]!=-1)
                throw new AssertionError("tilted INSERT cannot be edited as raw coordinates");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testNestedTiltedMinsert() throws Exception {
        File file=File.createTempFile("mesh3d-tilted-array-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nPART\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n0\n11\n2\n21\n0\n31\n1\n"+
                "0\nENDBLK\n"+
                "0\nBLOCK\n2\nASSEMBLY\n"+
                "0\nINSERT\n2\nPART\n10\n1\n20\n2\n30\n3\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nASSEMBLY\n10\n10\n20\n20\n30\n7\n"+
                "210\n0\n220\n1\n230\n0\n70\n2\n71\n1\n44\n5\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=12||result.edges.length!=4)
                throw new AssertionError("tilted nested MINSERT array count");
            if(result.skippedBlockReferences!=0)
                throw new AssertionError("valid nested 3D array must not be marked incomplete");
            double[] expected={-11,10,22, -13,11,22, -16,10,22, -18,11,22};
            for(int i=0;i<expected.length;i++)
                assertNear(result.xyz[i],expected[i],"nested OCS coordinate "+i);
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testInvalidTiltNormalDoesNotLeakGeometry() throws Exception {
        File file=File.createTempFile("mesh3d-tilted-invalid-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nBAD\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n0\n11\n2\n21\n0\n31\n0\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nBAD\n10\n10\n20\n20\n"+
                "210\n0\n220\n0\n230\n0\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=6||result.edges.length!=2)
                throw new AssertionError("invalid extrusion must not invent vertices");
            if(result.skippedBlockReferences!=1)
                throw new AssertionError("invalid OCS block must be disclosed as incomplete");
        }finally{Files.deleteIfExists(file.toPath());}
    }


    private static void testMissingAndExternalBlocksFlagIncomplete3d() throws Exception {
        File file=File.createTempFile("mesh3d-partial-blocks-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nEXTERNAL\n70\n4\n1\nprivate-path.dwg\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n"+
                "0\nENDBLK\n0\nENDSEC\n"+
                "0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nUNKNOWN\n10\n0\n20\n0\n"+
                "0\nINSERT\n2\nEXTERNAL\n10\n0\n20\n0\n"+
                "0\nLINE\n10\n0\n20\n0\n30\n0\n11\n2\n21\n0\n31\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh result=Dxf3dMesh.read(file);
            if(result.xyz.length!=6||result.edges.length!=2)
                throw new AssertionError("valid independent geometry must still open");
            if(result.skippedBlockReferences!=2)
                throw new AssertionError("missing XREF and unknown block must count as partial");
            if(result.solidProxy)
                throw new AssertionError("unresolved XREF must not fabricate a 3D solid");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testFlat2dPlanMetadata() throws Exception {
        File file=File.createTempFile("mesh3d-flat-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"
                +"0\nLINE\n10\n0\n20\n0\n30\n0\n11\n10\n21\n5\n31\n0\n"
                +"0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(!mesh.flatPlan)throw new AssertionError("flat 2D plan must be detected");
            if(mesh.minZ!=0f||mesh.maxZ!=0f)throw new AssertionError("flat 2D Z range");
            if(mesh.solidProxy)throw new AssertionError("ordinary 2D plan must not be marked as solid proxy");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testAcisSatSolidMesh() throws Exception {
        File file=File.createTempFile("mesh3d-acis-", ".dxf");
        try{
            String[] records={
                "-0 point $-1 0 0 0 #","-1 point $-1 10 0 0 #","-2 point $-1 0 10 0 #",
                "-10 vertex $-1 $-1 $0 #","-11 vertex $-1 $-1 $1 #","-12 vertex $-1 $-1 $2 #",
                "-20 straight-curve $-1 0 0 0 1 0 0 I I #","-21 straight-curve $-1 10 0 0 -1 1 0 I I #","-22 straight-curve $-1 0 10 0 0 -1 0 I I #",
                "-30 edge $-1 $10 0 $11 10 $40 $20 forward 7 unknown #",
                "-31 edge $-1 $11 0 $12 10 $41 $21 forward 7 unknown #",
                "-32 edge $-1 $12 0 $10 10 $42 $22 forward 7 unknown #",
                "-40 coedge $-1 $41 $42 $-1 $30 forward $50 $-1 #",
                "-41 coedge $-1 $42 $40 $-1 $31 forward $50 $-1 #",
                "-42 coedge $-1 $40 $41 $-1 $32 forward $50 $-1 #",
                "-50 loop $-1 $-1 $40 $60 #",
                "-70 plane-surface $-1 0 0 0 0 0 1 1 0 0 forward_v I I I I #",
                "-60 face $-1 $-1 $50 $-1 $-1 $70 forward single #"
            };
            StringBuilder dxf=new StringBuilder("0\nSECTION\n2\nENTITIES\n0\n3DSOLID\n70\n1\n");
            for(int i=0;i<records.length;i++)dxf.append(i==0?"1\n":"3\n").append(records[i]).append("\n");
            dxf.append("0\nENDSEC\n0\nEOF\n");
            Files.writeString(file.toPath(),dxf.toString(),StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.solidProxy)throw new AssertionError("valid ACIS SAT must not fall back to bounds proxy");
            if(mesh.acisSolids!=1)throw new AssertionError("ACIS solid must be marked read-only");
            if(mesh.unsupportedSolids!=0)throw new AssertionError("planar ACIS solid must resolve completely");
            if(mesh.triangles.length!=3||mesh.xyz.length!=9)
                throw new AssertionError("ACIS triangle must become real tessellated geometry");
            for(int line:mesh.sourceLines)if(line!=-1)throw new AssertionError("tessellated ACIS vertices are read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testAcisSolidBoundsProxy() throws Exception {
        File file=File.createTempFile("mesh3d-solid-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nHEADER\n"
                +"9\n$EXTMIN\n10\n1\n20\n2\n30\n3\n"
                +"9\n$EXTMAX\n10\n11\n20\n22\n30\n33\n"
                +"0\nENDSEC\n"
                +"0\nSECTION\n2\nENTITIES\n"
                +"0\n3DSOLID\n70\n1\n1\nACIS_PLACEHOLDER\n"
                +"0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(!mesh.solidProxy)throw new AssertionError("ACIS solid must use bounds proxy when no tessellation is available");
            if(mesh.acisSolids!=1)throw new AssertionError("fallback ACIS solid count");
            if(mesh.unsupportedSolids!=1)throw new AssertionError("ACIS solid count");
            if(mesh.xyz.length!=24||mesh.triangles.length!=0||mesh.edges.length!=24)
                throw new AssertionError("solid proxy must be an 8-corner, 12-edge wire box");
            if(mesh.minZ!=3f||mesh.maxZ!=33f||mesh.flatPlan)throw new AssertionError("solid proxy Z bounds");
            for(int v:mesh.sourceLines)if(v!=-1)throw new AssertionError("solid proxy geometry must be read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testMissing3dTagDoesNotWritePartialFile() throws Exception {
        File file=File.createTempFile("mesh3d-save-guard-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"
                +"0\n3DFACE\n10\n0\n20\n0\n30\n1\n"
                +"11\n1\n21\n0\n31\n1\n"
                +"12\n0\n22\n1\n32\n1\n"
                +"0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            float[] original=mesh.xyz.clone();
            mesh.xyz[2]=9f; // edit source vertex-0 Z coordinate (DXF group 30)
            Files.writeString(file.toPath(),dxf.replace("30\n1\n",""),StandardCharsets.UTF_8);
            ByteArrayOutputStream target=new ByteArrayOutputStream();
            boolean blocked=false;
            try{Dxf3dEditor.write(file,target,mesh,original);}
            catch(java.io.IOException e){blocked=true;}
            if(!blocked)throw new AssertionError("missing source coordinate must block 3D save");
            if(target.size()!=0)throw new AssertionError("preflight failure wrote an incomplete 3D DXF file");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static String vertex(int x,int y,int z){return "0\nVERTEX\n70\n192\n10\n"+x+"\n20\n"+y+"\n30\n"+z+"\n";}
    private static String meshVertex(int x,int y,int z){return "0\nVERTEX\n70\n64\n10\n"+x+"\n20\n"+y+"\n30\n"+z+"\n";}
}
