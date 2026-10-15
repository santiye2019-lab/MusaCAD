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
        testLwPolylineBulgeAndSparseVertices();
        testLwPolylineClosedArcAndNegativeBulge();
        testTiltedLwPolylineInRotatedBlock();
        testMalformedLwPolylineFailClosed();
        testCircularAndArcOutlines();
        testEllipticalArcsAndFullEllipse();
        testQuadraticAndRationalSpline();
        testFitOnlySplineInterpolationAndInsert();
        testMalformedFitOnlySplines();
        testNonuniformAndRepeatedSplineKnots();
        testVerifiedPeriodicRationalSpline();
        testFalsePeriodicSplineSeamsRejected();
        testClosedSplineAndRotatedBlock();
        testUnsupportedAndMalformedSplinesFailClosed();
        testTiltedEllipseWithinRotatedBlock();
        testMalformedEllipseFailClosed();
        testTiltedCircularOcsAndNestedInsert();
        testInvalidCurvesFailClosed();
        test3dFaceInvisibleEdgesAndSafeRoundtrip();
        testSolidTraceReversedLastCornerAndPreservedDxfRoundtrip();
        testPolyfaceSignedInvisibleEdgesAndQuadWireframe();
        testPolygonMesh();
        testPolygonMeshWireGridOpenClosedAndShortSource();
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

    private static void testPolyfaceSignedInvisibleEdgesAndQuadWireframe() throws Exception {
        File file=File.createTempFile("mesh3d-polyface-signed-", ".dxf");
        File saved=File.createTempFile("mesh3d-polyface-saved-", ".dxf");
        try {
            String start="0\nSECTION\n2\nENTITIES\n"+
                "0\nPOLYLINE\n70\n64\n"+
                vertex(0,0,0)+vertex(4,0,0)+vertex(4,3,0)+vertex(0,3,0);
            String end="0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
            // A signed -3 hides only edge 3->4. A quad has TWO shaded
            // triangles but only three visible perimeter edges.
            String face="0\nVERTEX\n70\n128\n71\n1\n72\n2\n73\n-3\n74\n4\n";
            Files.writeString(file.toPath(),start+face+end,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=12||mesh.triangles.length!=6||mesh.edges.length!=6)
                throw new AssertionError("polyface quad shading must not leak hidden or diagonal edges");
            if(!java.util.Arrays.equals(mesh.edges,new int[]{0,1,1,2,3,0}))
                throw new AssertionError("polyface negative vertex hides START->NEXT edge");

            // Unrelated editable coordinate changes must preserve the face
            // signed index in DXF and reproduce the same visible wireframe.
            float[] source=mesh.xyz.clone();
            mesh.xyz[11]=2f;
            ByteArrayOutputStream output=new ByteArrayOutputStream();
            Dxf3dEditor.write(file,output,mesh,source);
            String exported=output.toString(StandardCharsets.UTF_8);
            if(!exported.contains("73\n-3\n"))
                throw new AssertionError("polyface hidden face edge sign must round-trip");
            Files.write(saved.toPath(),output.toByteArray());
            Dxf3dMesh again=Dxf3dMesh.read(saved);
            if(again.xyz[11]!=2f||again.triangles.length!=6||
               !java.util.Arrays.equals(again.edges,mesh.edges))
                throw new AssertionError("saved polyface visible edges changed");

            // Make all four source perimeter edges visible. The generated
            // triangulation diagonal 1->3 still must NOT be shown.
            Files.writeString(file.toPath(),
                    start+face.replace("73\n-3\n","73\n3\n")+end,
                    StandardCharsets.UTF_8);
            mesh=Dxf3dMesh.read(file);
            if(mesh.triangles.length!=6||mesh.edges.length!=8||
               !java.util.Arrays.equals(mesh.edges,new int[]{0,1,1,2,2,3,3,0}))
                throw new AssertionError("polyface quad must have four visible borders only");

            // In triangular polyfaces the sign of the FIRST vertex hides
            // edge 1->2, including last->first wrap semantics.
            String triangle="0\nVERTEX\n70\n128\n71\n-1\n72\n2\n73\n3\n";
            Files.writeString(file.toPath(),start+triangle+end,StandardCharsets.UTF_8);
            mesh=Dxf3dMesh.read(file);
            if(mesh.triangles.length!=3||
               !java.util.Arrays.equals(mesh.edges,new int[]{1,2,2,0}))
                throw new AssertionError("polyface signed first vertex edge not honored");

            // Fail closed on invalid face indexing rather than invent a
            // different surface or silently dropping invalid geometry.
            String[] invalid={
                face.replace("73\n-3\n","73\n-3.5\n"),
                face.replace("73\n-3\n","73\n0\n"),
                face.replace("74\n4\n","74\n9\n"),
                face.replace("74\n4\n","74\n2\n"),
                face.replace("73\n-3\n",""),
                face.replace("73\n-3\n","73\nNaN\n")
            };
            for(int k=0;k<invalid.length;k++){
                Files.writeString(file.toPath(),start+invalid[k]+end,
                                  StandardCharsets.UTF_8);
                boolean rejected=false;
                try{Dxf3dMesh.read(file);}
                catch(java.io.IOException expected){rejected=true;}
                if(!rejected)throw new AssertionError(
                        "malformed polyface index accepted: case "+k);
            }
        }finally{
            Files.deleteIfExists(file.toPath());
            Files.deleteIfExists(saved.toPath());
        }
    }

    private static void testSolidTraceReversedLastCornerAndPreservedDxfRoundtrip()
            throws Exception {
        File file=File.createTempFile("mesh3d-solid-trace-order-", ".dxf");
        File saved=File.createTempFile("mesh3d-solid-trace-saved-", ".dxf");
        try{
            String head="0\nSECTION\n2\nENTITIES\n";
            String end="0\nENDSEC\n0\nEOF\n";
            // Raw DXF SOLID/TRACE source order: 0=(0,0), 1=(4,0),
            // 2=(0,3), 3=(4,3). The physical perimeter is 0,1,3,2.
            String reversed="10\n0\n20\n0\n30\n0\n"+
                "11\n4\n21\n0\n31\n0\n"+
                "12\n0\n22\n3\n32\n0\n"+
                "13\n4\n23\n3\n33\n0\n";
            int[] expectedTriangles={0,1,3,0,3,2};
            int[] expectedEdges={0,1,1,3,3,2,2,0};
            for(String type:new String[]{"SOLID","TRACE"}){
                String dxf=head+"0\n"+type+"\n"+reversed+end;
                Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
                Dxf3dMesh mesh=Dxf3dMesh.read(file);
                if(mesh.xyz.length!=12||
                   !java.util.Arrays.equals(mesh.triangles,expectedTriangles)||
                   !java.util.Arrays.equals(mesh.edges,expectedEdges))
                    throw new AssertionError(type+" reversed fourth/third order or wireframe wrong");
                float[] opened=mesh.xyz.clone();
                mesh.xyz[8]=2f; // source DXF 12/22/32 third corner
                ByteArrayOutputStream output=new ByteArrayOutputStream();
                Dxf3dEditor.write(file,output,mesh,opened);
                Files.write(saved.toPath(),output.toByteArray());
                String written=output.toString(StandardCharsets.UTF_8);
                if(!written.contains("12\n0\n22\n3\n32\n2\n"))
                    throw new AssertionError(type+" third DXF source corner was not preserved");
                Dxf3dMesh reopened=Dxf3dMesh.read(saved);
                if(reopened.xyz[8]!=2f||
                   !java.util.Arrays.equals(reopened.triangles,expectedTriangles)||
                   !java.util.Arrays.equals(reopened.edges,expectedEdges))
                    throw new AssertionError(type+" save/reopen changed original corner order");

                // No fourth corner: true triangle (0,1,2).
                Files.writeString(file.toPath(),
                        head+"0\n"+type+"\n"+reversed.substring(0,reversed.indexOf("13\n"))+end,
                        StandardCharsets.UTF_8);
                mesh=Dxf3dMesh.read(file);
                if(mesh.triangles.length!=3||mesh.edges.length!=6)
                    throw new AssertionError(type+" 3-corner face must remain triangle");

                // Autodesk DXF also repeats the third vertex as the fourth
                // for some triangular SOLID/TRACE records. No zero-area
                // extra triangulation or duplicated perimeter edge.
                Files.writeString(file.toPath(),
                        head+"0\n"+type+"\n"+
                        reversed.replace("13\n4\n23\n3\n33\n0\n",
                                         "13\n0\n23\n3\n33\n0\n")+end,
                        StandardCharsets.UTF_8);
                mesh=Dxf3dMesh.read(file);
                if(mesh.triangles.length!=3||mesh.edges.length!=6)
                    throw new AssertionError(type+" duplicated corner must not create a quad");
            }

            // Unlike SOLID/TRACE, 3DFACE stores its last two vertices in
            // ordinary perimeter order, and this MUST NOT be swapped.
            String ordinary="10\n0\n20\n0\n30\n0\n"+
                "11\n4\n21\n0\n31\n0\n"+
                "12\n4\n22\n3\n32\n0\n"+
                "13\n0\n23\n3\n33\n0\n";
            Files.writeString(file.toPath(),
                    head+"0\n3DFACE\n"+ordinary+end,StandardCharsets.UTF_8);
            Dxf3dMesh face=Dxf3dMesh.read(file);
            if(!java.util.Arrays.equals(face.triangles,new int[]{0,1,2,0,2,3})||
               !java.util.Arrays.equals(face.edges,new int[]{0,1,1,2,2,3,3,0}))
                throw new AssertionError("3DFACE must keep original 0,1,2,3 vertex order");
        }finally{
            Files.deleteIfExists(file.toPath());
            Files.deleteIfExists(saved.toPath());
        }
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

    private static void assertCurveNear(float actual,double expected,String why){
        if(!Float.isFinite(actual)||Math.abs(actual-expected)>0.0002)
            throw new AssertionError(why+": "+actual+" != "+expected);
    }

    private static void testFitOnlySplineInterpolationAndInsert() throws Exception {
        File file=File.createTempFile("mesh3d-fit-spline-", ".dxf");
        try{
            String linear="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n0\n71\n1\n72\n0\n73\n0\n74\n3\n"+
                "11\n0\n21\n0\n31\n2\n"+
                "11\n3\n21\n0\n31\n2\n"+
                "11\n3\n21\n4\n31\n2\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),linear,StandardCharsets.UTF_8);
            Dxf3dMesh line=Dxf3dMesh.read(file);
            if(line.approximateCurves!=1||line.xyz.length!=9||
               line.edges.length!=4||line.triangles.length!=0)
                throw new AssertionError("linear fit-only SPLINE must join all fit points");
            assertCurveNear(line.xyz[0],0,"linear fit first X");
            assertCurveNear(line.xyz[3],3,"linear fit middle X");
            assertCurveNear(line.xyz[4],0,"linear fit middle Y");
            assertCurveNear(line.xyz[6],3,"linear fit last X");
            assertCurveNear(line.xyz[7],4,"linear fit last Y");
            String cubic="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nFIT_PIPE\n10\n0\n20\n0\n30\n0\n"+
                "0\nSPLINE\n70\n8\n71\n3\n72\n0\n73\n0\n74\n3\n"+
                "11\n0\n21\n0\n31\n1\n"+
                "11\n4\n21\n4\n31\n1\n"+
                "11\n8\n21\n0\n31\n1\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nFIT_PIPE\n10\n10\n20\n20\n30\n30\n50\n90\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),cubic,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=1||mesh.xyz.length!=25*3||
               mesh.edges.length!=24*2||mesh.triangles.length!=0)
                throw new AssertionError("cubic fit-only SPLINE must sample 2 chord-length segments");
            assertCurveNear(mesh.xyz[0],10,"rotated fit start X");
            assertCurveNear(mesh.xyz[1],20,"rotated fit start Y");
            assertCurveNear(mesh.xyz[2],31,"rotated fit start Z");
            assertCurveNear(mesh.xyz[12*3],6,"rotated fit middle X");
            assertCurveNear(mesh.xyz[12*3+1],24,"rotated fit middle Y");
            assertCurveNear(mesh.xyz[24*3],10,"rotated fit end X");
            assertCurveNear(mesh.xyz[24*3+1],28,"rotated fit end Y");
            float[] original=mesh.xyz.clone();
            for(int i=0;i<mesh.sourceLines.length;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("fit-only preview vertices must not be editable");
            mesh.xyz[0]+=1;
            boolean rejected=false;
            try{Dxf3dEditor.write(file,new ByteArrayOutputStream(),mesh,original);}
            catch(java.io.IOException expected){rejected=true;}
            if(!rejected)throw new AssertionError("fit-only SPLINE sample edit must be rejected");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testMalformedFitOnlySplines() throws Exception {
        File file=File.createTempFile("mesh3d-bad-fit-spline-", ".dxf");
        try{
            String fit="11\n0\n21\n0\n11\n2\n21\n2\n11\n4\n21\n0\n";
            String[] invalid={
                "71\n3\n74\n2\n"+fit,          // incorrect fit count
                "71\n3\n74\n3\n11\n0\n21\n0\n11\n2\n11\n4\n21\n0\n",
                "71\n3\n74\n3\n11\n0\n21\n0\n11\n0\n21\n0\n11\n4\n21\n0\n",
                "71\n2\n74\n3\n"+fit,          // degree 2 undefined by preview
                "70\n1\n71\n3\n74\n3\n"+fit,   // closed fit-only unsupported
                "70\n2\n71\n3\n74\n3\n"+fit,   // periodic fit-only unsupported
                "71\n3\n74\n3\n12\n1\n22\n1\n"+fit, // tangent requires exact solver
                "71\n3\n74\n3\n"+fit+"11\n6\n21\n0\n",
                "71\n3\n74\n3\n"+fit+"40\n0\n",
                "71\n3\n74\n3\n"+fit+"10\n0\n20\n0\n",
                "71\n3\n74\n3\n11\nNaN\n21\n0\n11\n2\n21\n0\n11\n4\n21\n0\n"
            };
            for(String groups:invalid){
                Files.writeString(file.toPath(),
                    "0\nSECTION\n2\nENTITIES\n0\nSPLINE\n"+
                    groups+"0\nENDSEC\n0\nEOF\n",StandardCharsets.UTF_8);
                boolean rejected=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){rejected=true;}
                if(!rejected)throw new AssertionError("malformed fit-only SPLINE must fail closed");
            }
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testQuadraticAndRationalSpline() throws Exception {
        File file=File.createTempFile("mesh3d-spline-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n8\n71\n2\n72\n6\n73\n3\n"+
                "40\n0\n40\n0\n40\n0\n40\n1\n40\n1\n40\n1\n"+
                "10\n0\n20\n0\n30\n1\n"+
                "10\n2\n20\n2\n30\n3\n"+
                "10\n4\n20\n0\n30\n5\n"+
                "0\nSPLINE\n70\n12\n71\n2\n72\n6\n73\n3\n"+
                "40\n0\n40\n0\n40\n0\n40\n1\n40\n1\n40\n1\n"+
                "41\n1\n41\n0.7071067811865476\n41\n1\n"+
                "10\n1\n20\n0\n10\n1\n20\n1\n10\n0\n20\n1\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=2||mesh.triangles.length!=0||
               mesh.xyz.length!=50*3||mesh.edges.length!=48*2)
                throw new AssertionError("two quadratic SPLINE curves must sample 24 segments each");
            assertCurveNear(mesh.xyz[0],0,"B-spline first X");
            assertCurveNear(mesh.xyz[2],1,"B-spline first Z");
            assertCurveNear(mesh.xyz[12*3],2,"B-spline midpoint X");
            assertCurveNear(mesh.xyz[12*3+1],1,"B-spline midpoint Y");
            assertCurveNear(mesh.xyz[12*3+2],3,"B-spline midpoint Z");
            assertCurveNear(mesh.xyz[24*3],4,"B-spline end X");
            assertCurveNear(mesh.xyz[24*3+2],5,"B-spline end Z");
            assertCurveNear(mesh.xyz[25*3],1,"rational start X");
            assertCurveNear(mesh.xyz[37*3],Math.sqrt(0.5),"rational quarter circle midpoint X");
            assertCurveNear(mesh.xyz[37*3+1],Math.sqrt(0.5),"rational midpoint Y");
            assertCurveNear(mesh.xyz[49*3],0,"rational end X");
            assertCurveNear(mesh.xyz[49*3+1],1,"rational end Y");
            for(int i=0;i<mesh.xyz.length/3;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("SPLINE sample must remain read-only");
            float[] before=mesh.xyz.clone();
            mesh.xyz[0]+=1;
            try{
                Dxf3dEditor.write(file,new ByteArrayOutputStream(),mesh,before);
                throw new AssertionError("tessellated SPLINE must not write as DXF control point");
            }catch(java.io.IOException expected){}
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testVerifiedPeriodicRationalSpline() throws Exception {
        File file=File.createTempFile("mesh3d-periodic-spline-", ".dxf");
        try{
            // Periodic degree-2 nonclamped knot vector with cyclic controls
            // A,B,C,A,B and repeated rational weights on matching controls.
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n6\n71\n2\n72\n8\n73\n5\n"+
                "40\n0\n40\n1\n40\n2\n40\n3\n"+
                "40\n4\n40\n5\n40\n6\n40\n7\n"+
                "41\n1\n41\n0.5\n41\n1\n41\n1\n41\n0.5\n"+
                "10\n0\n20\n0\n30\n4\n"+
                "10\n2\n20\n0\n30\n4\n"+
                "10\n1\n20\n2\n30\n4\n"+
                "10\n0\n20\n0\n30\n4\n"+
                "10\n2\n20\n0\n30\n4\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=1||mesh.triangles.length!=0||
                mesh.xyz.length!=48*3||mesh.edges.length!=48*2)
                throw new AssertionError("periodic spline must close with 48 bounded wire segments");
            assertCurveNear(mesh.xyz[0],2d/3d,"rational periodic seam X");
            assertCurveNear(mesh.xyz[1],0,"rational periodic seam Y");
            assertCurveNear(mesh.xyz[2],4,"rational periodic elevation");
            assertCurveNear(mesh.xyz[16*3],4d/3d,"rational interior knot X");
            assertCurveNear(mesh.xyz[16*3+1],4d/3d,"rational interior knot Y");
            assertCurveNear(mesh.xyz[32*3],0.5,"rational second interior knot X");
            assertCurveNear(mesh.xyz[32*3+1],1,"rational second interior knot Y");
            if(mesh.edges[mesh.edges.length-2]!=47||
                mesh.edges[mesh.edges.length-1]!=0)
                throw new AssertionError("periodic spline must reconnect to the first point");
            for(int i=0;i<mesh.sourceLines.length;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("periodic samples are not source DXF edit vertices");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testFalsePeriodicSplineSeamsRejected() throws Exception {
        File file=File.createTempFile("mesh3d-false-periodic-", ".dxf");
        try{
            String prefix="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n2\n71\n2\n72\n8\n73\n5\n"+
                "40\n0\n40\n1\n40\n2\n40\n3\n"+
                "40\n4\n40\n5\n40\n6\n40\n7\n";
            String firstThree="10\n0\n20\n0\n10\n2\n20\n0\n10\n1\n20\n2\n";
            String[] invalidSuffix={
                // Periodic flag with controls that do NOT meet at the seam.
                "10\n4\n20\n0\n10\n2\n20\n0\n",
                // Endpoints meet geometrically but their directions oppose:
                // no invented zero-order seam/cusp for degree >= 2.
                "10\n2\n20\n0\n10\n0\n20\n0\n"
            };
            for(String suffix:invalidSuffix){
                Files.writeString(file.toPath(),prefix+firstThree+suffix+
                    "0\nENDSEC\n0\nEOF\n",StandardCharsets.UTF_8);
                boolean refused=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
                if(!refused)
                    throw new AssertionError("false periodic seam must not silently wrap");
            }
            // Translating all invalid controls to far-away map coordinates
            // must NOT dilute the tangent mismatch tolerance.
            String translated=prefix+
                "10\n100000000\n20\n100000000\n"+
                "10\n100000002\n20\n100000000\n"+
                "10\n100000001\n20\n100000002\n"+
                "10\n100000002\n20\n100000000\n"+
                "10\n100000000\n20\n100000000\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),translated,StandardCharsets.UTF_8);
            boolean refused=false;
            try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
            if(!refused)
                throw new AssertionError("translated false periodic seam must be rejected");

            // C2+ continuity is not yet proven for degree 3+ periodic curves;
            // rejecting these is safer than showing a false smooth seam.
            String cubic=prefix.replace("71\n2\n72\n8\n73\n5\n",
                "71\n3\n72\n9\n73\n5\n").replace("40\n7\n","40\n7\n40\n8\n");
            Files.writeString(file.toPath(),cubic+firstThree+
                "10\n0\n20\n0\n10\n2\n20\n0\n"+
                "0\nENDSEC\n0\nEOF\n",StandardCharsets.UTF_8);
            refused=false;
            try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
            if(!refused)
                throw new AssertionError("unverified degree-3 periodic spline must reject");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testNonuniformAndRepeatedSplineKnots() throws Exception {
        File file=File.createTempFile("mesh3d-knot-corners-", ".dxf");
        try{
            // A knot at 0.001 would be missed by naive 1/36 sampling.
            // Each distinct knot must be sampled exactly, preserving corners.
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n0\n71\n1\n72\n6\n73\n4\n"+
                "40\n0\n40\n0\n40\n0.001\n40\n0.75\n40\n1\n40\n1\n"+
                "10\n0\n20\n0\n10\n0\n20\n10\n"+
                "10\n10\n20\n10\n10\n10\n20\n0\n"+
                "0\nSPLINE\n70\n0\n71\n2\n72\n8\n73\n5\n"+
                "40\n0\n40\n0\n40\n0\n40\n0.03\n40\n0.03\n"+
                "40\n1\n40\n1\n40\n1\n"+
                "10\n0\n20\n0\n10\n1\n20\n0\n10\n2\n20\n0\n"+
                "10\n2\n20\n2\n10\n3\n20\n2\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=2||mesh.triangles.length!=0||
                mesh.xyz.length!=(37+49)*3||mesh.edges.length!=(36+48)*2)
                throw new AssertionError("SPLINE knot-aware sampling vertex budget changed");
            // degree-1 sharp turns occur exactly on nonuniform knots.
            assertCurveNear(mesh.xyz[12*3],0,"first tightly spaced knot X");
            assertCurveNear(mesh.xyz[12*3+1],10,"first tightly spaced knot Y");
            assertCurveNear(mesh.xyz[24*3],10,"second nonuniform knot X");
            assertCurveNear(mesh.xyz[24*3+1],10,"second nonuniform knot Y");
            // Repeated degree-2 interior knot (C0) is also explicitly
            // sampled and does not get smoothed over in the wireframe.
            int kink=37+24;
            assertCurveNear(mesh.xyz[kink*3],2,"repeated knot X");
            assertCurveNear(mesh.xyz[kink*3+1],0,"repeated knot Y");
            if(mesh.sourceLines[kink]!=-1)
                throw new AssertionError("knot-aware tessellation is not directly editable");

            // Multiplicity degree+1 inside a spline is discontinuous and
            // requires separate subpaths, which this reader cannot represent.
            // Reject it rather than silently draw an invented connecting edge.
            String discontinuous="0\nSECTION\n2\nENTITIES\n"+
                "0\nSPLINE\n70\n0\n71\n1\n72\n6\n73\n4\n"+
                "40\n0\n40\n0\n40\n0.5\n40\n0.5\n40\n1\n40\n1\n"+
                "10\n0\n20\n0\n10\n1\n20\n0\n"+
                "10\n5\n20\n0\n10\n6\n20\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),discontinuous,StandardCharsets.UTF_8);
            boolean rejected=false;
            try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){rejected=true;}
            if(!rejected)throw new AssertionError("discontinuous SPLINE must fail closed");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testClosedSplineAndRotatedBlock() throws Exception {
        File file=File.createTempFile("mesh3d-closed-spline-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nSPLINE_PIPE\n10\n0\n20\n0\n30\n0\n"+
                "0\nSPLINE\n70\n1\n71\n1\n72\n6\n73\n4\n"+
                "40\n0\n40\n0\n40\n1\n40\n2\n40\n3\n40\n3\n"+
                "10\n0\n20\n0\n30\n2\n"+
                "10\n2\n20\n0\n30\n2\n"+
                "10\n2\n20\n2\n30\n2\n"+
                "10\n0\n20\n0\n30\n2\n"+
                "0\nENDBLK\n0\nENDSEC\n"+
                "0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nSPLINE_PIPE\n10\n10\n20\n20\n30\n30\n50\n90\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=1||mesh.triangles.length!=0||
               mesh.xyz.length!=36*3||mesh.edges.length!=36*2)
                throw new AssertionError("closed SPLINE must reconnect without duplicate sample");
            assertCurveNear(mesh.xyz[0],10,"rotated closed spline first X");
            assertCurveNear(mesh.xyz[1],20,"rotated closed spline first Y");
            assertCurveNear(mesh.xyz[2],32,"rotated closed spline first Z");
            if(mesh.edges[mesh.edges.length-2]!=35||mesh.edges[mesh.edges.length-1]!=0)
                throw new AssertionError("closed B-spline must close on first point");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testUnsupportedAndMalformedSplinesFailClosed() throws Exception {
        File file=File.createTempFile("mesh3d-bad-spline-", ".dxf");
        try{
            String valid="71\n2\n72\n6\n73\n3\n"+
                "40\n0\n40\n0\n40\n0\n40\n1\n40\n1\n40\n1\n"+
                "10\n0\n20\n0\n10\n1\n20\n1\n10\n2\n20\n0\n";
            String[] invalid={
                valid.replace("71\n2\n","71\n8\n"),
                valid.replace("72\n6\n","72\n5\n"),
                valid.replace("40\n0\n40\n0\n40\n0\n40\n1\n",
                              "40\n0\n40\n0\n40\n1\n40\n0\n"),
                valid.replace("10\n1\n20\n1\n","10\n1\n"),
                valid.replace("40\n1\n40\n1\n40\n1\n","40\n1\n40\n1\n40\nNaN\n"),
                valid+"41\n1\n41\n0\n41\n1\n",
                valid+"41\n1\n41\n1\n",
                "71\n2\n72\n0\n73\n0\n11\n1\n21\n1\n",
                "70\n2\n"+valid,
                "70\n1\n"+valid
            };
            for(String groups:invalid){
                Files.writeString(file.toPath(),"0\nSECTION\n2\nENTITIES\n"+
                    "0\nSPLINE\n"+groups+"0\nENDSEC\n0\nEOF\n",
                    StandardCharsets.UTF_8);
                boolean rejected=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){rejected=true;}
                if(!rejected)throw new AssertionError("unsupported or malformed SPLINE must fail closed");
            }
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testEllipticalArcsAndFullEllipse() throws Exception {
        File file=File.createTempFile("mesh3d-ellipse-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nELLIPSE\n10\n10\n20\n20\n30\n30\n"+
                "11\n3\n21\n0\n31\n0\n40\n0.5\n"+
                "0\nELLIPSE\n10\n0\n20\n0\n30\n4\n"+
                "11\n4\n21\n0\n31\n0\n40\n0.25\n"+
                "41\n1.5707963267948966\n42\n3.141592653589793\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=2||mesh.triangles.length!=0||
                mesh.xyz.length!=61*3||mesh.edges.length!=60*2)
                throw new AssertionError("ELLIPSE full/partial path must close only full ellipse");
            assertCurveNear(mesh.xyz[0],13,"ellipse first X");
            assertCurveNear(mesh.xyz[1],20,"ellipse first Y");
            assertCurveNear(mesh.xyz[2],30,"ellipse first Z");
            assertCurveNear(mesh.xyz[12*3],10,"ellipse quarter X");
            assertCurveNear(mesh.xyz[12*3+1],21.5,"ellipse quarter Y");
            if(mesh.edges[94]!=47||mesh.edges[95]!=0)
                throw new AssertionError("full ELLIPSE must close without duplicate endpoint");
            assertCurveNear(mesh.xyz[48*3],0,"partial ellipse first X");
            assertCurveNear(mesh.xyz[48*3+1],1,"partial ellipse first Y");
            assertCurveNear(mesh.xyz[60*3],-4,"partial ellipse last X");
            assertCurveNear(mesh.xyz[60*3+1],0,"partial ellipse last Y");
            for(int i=0;i<mesh.xyz.length/3;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("sampled ellipse vertices must be read-only");
            float[] original=mesh.xyz.clone();
            mesh.xyz[0]+=1;
            try{
                Dxf3dEditor.write(file,new ByteArrayOutputStream(),mesh,original);
                throw new AssertionError("sampled ellipse is not a raw DXF vertex");
            }catch(java.io.IOException expected){}
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testTiltedEllipseWithinRotatedBlock() throws Exception {
        File file=File.createTempFile("mesh3d-ellipse-block-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nELLIPTIC_PIPE\n10\n0\n20\n0\n30\n0\n"+
                "0\nELLIPSE\n10\n1\n20\n2\n30\n3\n"+
                "11\n0\n21\n0\n31\n4\n40\n0.5\n"+
                "210\n0\n220\n1\n230\n0\n"+
                "0\nENDBLK\n0\nENDSEC\n"+
                "0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nELLIPTIC_PIPE\n10\n10\n20\n20\n30\n30\n50\n90\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=48*3||mesh.edges.length!=48*2||mesh.approximateCurves!=1)
                throw new AssertionError("tilted ELLIPSE inside INSERT missing");
            // Source point at (1,2,7); 90 degree INSERT -> (8,21,37).
            assertCurveNear(mesh.xyz[0],8,"insert ellipse first X");
            assertCurveNear(mesh.xyz[1],21,"insert ellipse first Y");
            assertCurveNear(mesh.xyz[2],37,"insert ellipse first Z");
            // Normal (0,1,0), major (0,0,4), minor (2,0,0):
            // quarter local (3,2,3) => (8,23,33).
            assertCurveNear(mesh.xyz[12*3],8,"insert ellipse quarter X");
            assertCurveNear(mesh.xyz[12*3+1],23,"insert ellipse quarter Y");
            assertCurveNear(mesh.xyz[12*3+2],33,"insert ellipse quarter Z");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testMalformedEllipseFailClosed() throws Exception {
        File file=File.createTempFile("mesh3d-bad-ellipse-", ".dxf");
        try{
            String[] invalid={
                "11\n0\n21\n0\n31\n0\n40\n0.5\n",
                "11\n1\n21\n0\n31\n0\n40\n0\n",
                "11\n1\n21\n0\n31\n0\n40\n1.1\n",
                "11\n1\n21\n0\n31\n0\n40\n0.3\n210\n0\n220\n0\n230\n0\n",
                "11\n1\n21\n0\n31\n1\n40\n0.3\n",
                "11\n1\n21\n0\n31\n0\n40\n0.3\n41\n0\n42\n0\n",
                "11\n1\n21\n0\n31\n0\n40\n0.3\n41\n0\n42\n8\n",
                "11\nNaN\n21\n0\n31\n0\n40\n0.3\n"
            };
            for(String values:invalid){
                Files.writeString(file.toPath(),
                    "0\nSECTION\n2\nENTITIES\n0\nELLIPSE\n"+
                    "10\n1\n20\n2\n30\n3\n"+values+"0\nENDSEC\n0\nEOF\n",
                    StandardCharsets.UTF_8);
                boolean refused=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
                if(!refused)throw new AssertionError("invalid ELLIPSE must fail closed");
            }
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testCircularAndArcOutlines() throws Exception {
        File file=File.createTempFile("mesh3d-circle-arc-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nCIRCLE\n10\n10\n20\n20\n30\n30\n40\n2\n"+
                "0\nARC\n10\n0\n20\n0\n30\n5\n40\n10\n50\n350\n51\n10\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=2||mesh.triangles.length!=0||
               mesh.xyz.length!=52*3||mesh.edges.length!=51*2)
                throw new AssertionError("CIRCLE/ARC must expand to bounded wire-only vertices/edges");
            assertCurveNear(mesh.xyz[0],12,"circle first X");
            assertCurveNear(mesh.xyz[1],20,"circle first Y");
            assertCurveNear(mesh.xyz[2],30,"circle first Z");
            assertCurveNear(mesh.xyz[12*3],10,"circle quarter X");
            assertCurveNear(mesh.xyz[12*3+1],22,"circle quarter Y");
            if(mesh.edges[94]!=47||mesh.edges[95]!=0)
                throw new AssertionError("circle must close exactly, not add a degenerate last vertex");
            assertCurveNear(mesh.xyz[48*3],10*Math.cos(Math.toRadians(350)),"wrapped arc first X");
            assertCurveNear(mesh.xyz[48*3+1],10*Math.sin(Math.toRadians(350)),"wrapped arc first Y");
            assertCurveNear(mesh.xyz[51*3],10*Math.cos(Math.toRadians(10)),"wrapped arc last X");
            assertCurveNear(mesh.xyz[51*3+1],10*Math.sin(Math.toRadians(10)),"wrapped arc last Y");
            if(mesh.edges[mesh.edges.length-2]!=50||mesh.edges[mesh.edges.length-1]!=51)
                throw new AssertionError("ARC must remain open");
            for(int i=0;i<mesh.sourceLines.length;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("sampled curve point must not be edited as a source DXF XYZ");

            float[] original=mesh.xyz.clone();
            mesh.xyz[0]+=1f;
            boolean refused=false;
            try{
                Dxf3dEditor.write(file,new ByteArrayOutputStream(),mesh,original);
            }catch(java.io.IOException expected){refused=true;}
            if(!refused)throw new AssertionError("sampled arc/circle XYZ must be read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testTiltedCircularOcsAndNestedInsert() throws Exception {
        File file=File.createTempFile("mesh3d-circle-ocs-", ".dxf");
        try{
            // Normal (0,1,0) has OCS X=(-1,0,0), Y=(0,0,1).
            String tilted="0\nSECTION\n2\nENTITIES\n"+
                "0\nCIRCLE\n10\n4\n20\n5\n30\n6\n40\n2\n"+
                "210\n0\n220\n1\n230\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),tilted,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=1||mesh.edges.length!=96)
                throw new AssertionError("tilted circle path not generated");
            assertCurveNear(mesh.xyz[0],-6,"tilted OCS X");
            assertCurveNear(mesh.xyz[1],6,"tilted OCS Y");
            assertCurveNear(mesh.xyz[2],5,"tilted OCS Z");
            assertCurveNear(mesh.xyz[12*3],-4,"tilted circle quarter X");
            assertCurveNear(mesh.xyz[12*3+1],6,"tilted circle quarter Y");
            assertCurveNear(mesh.xyz[12*3+2],7,"tilted circle quarter Z");

            String nested="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nPIPE_CURVE\n10\n0\n20\n0\n30\n0\n"+
                "0\nCIRCLE\n10\n1\n20\n0\n30\n2\n40\n1\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nPIPE_CURVE\n10\n10\n20\n20\n30\n30\n50\n90\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),nested,StandardCharsets.UTF_8);
            Dxf3dMesh viaBlock=Dxf3dMesh.read(file);
            if(viaBlock.approximateCurves!=1||viaBlock.edges.length!=96)
                throw new AssertionError("inserted circle must expand to complete 3D path");
            assertCurveNear(viaBlock.xyz[0],10,"insert rotated first X");
            assertCurveNear(viaBlock.xyz[1],22,"insert rotated first Y");
            assertCurveNear(viaBlock.xyz[2],32,"insert rotated first Z");
            assertCurveNear(viaBlock.xyz[12*3],9,"insert rotated quarter X");
            assertCurveNear(viaBlock.xyz[12*3+1],21,"insert rotated quarter Y");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testInvalidCurvesFailClosed() throws Exception {
        File file=File.createTempFile("mesh3d-invalid-curve-", ".dxf");
        try{
            String[] invalid={
                "0\nCIRCLE\n10\n0\n20\n0\n40\n0\n",
                "0\nCIRCLE\n10\n0\n20\n0\n40\n4\n210\n0\n220\n0\n230\n0\n",
                "0\nARC\n10\n0\n20\n0\n40\n4\n50\n90\n51\n90\n",
                "0\nCIRCLE\n10\n0\n20\n0\n40\nNaN\n",
                "0\nCIRCLE\n10\n0\n20\n0\n40\n1e308\n"
            };
            for(String entity:invalid){
                Files.writeString(file.toPath(),
                    "0\nSECTION\n2\nENTITIES\n"+entity+"0\nENDSEC\n0\nEOF\n",
                    StandardCharsets.UTF_8);
                boolean refused=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
                if(!refused)throw new AssertionError("invalid CIRCLE/ARC must fail closed");
            }
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testLwPolylineBulgeAndSparseVertices() throws Exception {
        File file=File.createTempFile("mesh3d-lw-bulge-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nENTITIES\n"+
                "0\nLWPOLYLINE\n90\n3\n70\n0\n38\n7\n"+
                "10\n0\n20\n0\n42\n1\n"+
                "10\n4\n20\n0\n"+
                "10\n4\n20\n3\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.approximateCurves!=1||mesh.triangles.length!=0||
               mesh.xyz.length!=26*3||mesh.edges.length!=25*2)
                throw new AssertionError("LWPOLYLINE semicircle + line count");
            assertCurveNear(mesh.xyz[0],0,"LWP first X");
            assertCurveNear(mesh.xyz[2],7,"LWP elevation");
            assertCurveNear(mesh.xyz[12*3],2,"LWP semicircle X");
            assertCurveNear(mesh.xyz[12*3+1],-2,"LWP positive bulge turns below XY chord");
            assertCurveNear(mesh.xyz[24*3],4,"LWP end of bulge X");
            assertCurveNear(mesh.xyz[25*3+1],3,"LWP straight end Y");
            for(int i=0;i<mesh.sourceLines.length;i++)
                if(mesh.sourceLines[i]!=-1||mesh.coordinateSlots[i]!=-1)
                    throw new AssertionError("LWP visual points must not be directly editable");
            float[] opened=mesh.xyz.clone();
            mesh.xyz[12*3+1]+=1;
            boolean refused=false;
            try{Dxf3dEditor.write(file,new ByteArrayOutputStream(),mesh,opened);}
            catch(java.io.IOException expected){refused=true;}
            if(!refused)throw new AssertionError("LWP tessellation must be read-only");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testLwPolylineClosedArcAndNegativeBulge() throws Exception {
        File file=File.createTempFile("mesh3d-lw-closed-", ".dxf");
        try{
            String negative="0\nSECTION\n2\nENTITIES\n"+
                "0\nLWPOLYLINE\n90\n2\n"+
                "10\n0\n20\n0\n42\n-1\n10\n4\n20\n0\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),negative,StandardCharsets.UTF_8);
            Dxf3dMesh mirrored=Dxf3dMesh.read(file);
            if(mirrored.xyz.length!=25*3||mirrored.edges.length!=24*2||
               mirrored.approximateCurves!=1)
                throw new AssertionError("negative bulge must draw a semicircular open arc");
            assertCurveNear(mirrored.xyz[12*3],2,"negative bulge midpoint X");
            assertCurveNear(mirrored.xyz[12*3+1],2,"negative bulge midpoint Y");

            String closed="0\nSECTION\n2\nENTITIES\n"+
                "0\nLWPOLYLINE\n90\n3\n70\n1\n"+
                "10\n0\n20\n0\n10\n4\n20\n0\n"+
                "10\n4\n20\n3\n42\n0.4142135623730950\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),closed,StandardCharsets.UTF_8);
            Dxf3dMesh shape=Dxf3dMesh.read(file);
            if(shape.approximateCurves!=1||shape.triangles.length!=0||
               shape.xyz.length!=14*3||shape.edges.length!=14*2)
                throw new AssertionError("closed LWPOLYLINE must wrap last bulge to first without duplicate vertex");
            if(shape.edges[shape.edges.length-1]!=0)
                throw new AssertionError("closed bulge must reconnect original first vertex");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testTiltedLwPolylineInRotatedBlock() throws Exception {
        File file=File.createTempFile("mesh3d-lw-block-", ".dxf");
        try{
            String dxf="0\nSECTION\n2\nBLOCKS\n"+
                "0\nBLOCK\n2\nLWPIPE\n10\n0\n20\n0\n30\n0\n"+
                "0\nLWPOLYLINE\n90\n2\n38\n5\n"+
                "210\n0\n220\n1\n230\n0\n"+
                "10\n1\n20\n2\n10\n3\n20\n2\n"+
                "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"+
                "0\nINSERT\n2\nLWPIPE\n10\n10\n20\n20\n30\n30\n50\n90\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
            Dxf3dMesh mesh=Dxf3dMesh.read(file);
            if(mesh.xyz.length!=6||mesh.edges.length!=2||mesh.approximateCurves!=0)
                throw new AssertionError("straight LWP in tilted OCS and INSERT must render");
            assertCurveNear(mesh.xyz[0],5,"OCS + rotated INSERT start X");
            assertCurveNear(mesh.xyz[1],19,"OCS + rotated INSERT start Y");
            assertCurveNear(mesh.xyz[2],32,"OCS + rotated INSERT start Z");
            assertCurveNear(mesh.xyz[3],5,"OCS + rotated INSERT end X");
            assertCurveNear(mesh.xyz[4],17,"OCS + rotated INSERT end Y");
            assertCurveNear(mesh.xyz[5],32,"OCS + rotated INSERT end Z");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testMalformedLwPolylineFailClosed() throws Exception {
        File file=File.createTempFile("mesh3d-lw-malformed-", ".dxf");
        try{
            String[] records={
                "90\n3\n10\n0\n20\n0\n10\n4\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n10\n4\n",
                "90\n2\n10\n0\n42\n1\n20\n0\n10\n4\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n42\n1\n10\n0\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n42\n1\n42\n-1\n10\n4\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n42\nNaN\n10\n4\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n10\n4\n20\n0\n210\n0\n220\n0\n230\n0\n",
                "90\n2\n10\n0\n20\n0\n10\n1e308\n20\n0\n",
                "90\n2\n10\n0\n20\n0\n42\n1e9\n10\n4\n20\n0\n"
            };
            for(String groups:records){
                Files.writeString(file.toPath(),
                    "0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n"+
                    groups+"0\nENDSEC\n0\nEOF\n",StandardCharsets.UTF_8);
                boolean refused=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){refused=true;}
                if(!refused)throw new AssertionError("malformed LWPOLYLINE must be rejected");
            }
        }finally{Files.deleteIfExists(file.toPath());}
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
            if(mesh.edges.length!=8||!java.util.Arrays.equals(mesh.edges,new int[]{0,1,2,3,0,2,1,3}))
                throw new AssertionError("polygon mesh must render only four authored grid edges, not diagonal");
            if(mesh.xyz[8]!=1f||mesh.xyz[11]!=2f)throw new AssertionError("polygon mesh Z coordinates");
        }finally{Files.deleteIfExists(file.toPath());}
    }

    private static void testPolygonMeshWireGridOpenClosedAndShortSource() throws Exception {
        File file=File.createTempFile("mesh3d-mesh-grid-", ".dxf");
        try{
            String vertices=
                meshVertex(0,0,0)+meshVertex(2,0,0)+meshVertex(4,0,0)+
                meshVertex(0,2,0)+meshVertex(2,2,1)+meshVertex(4,2,0)+
                meshVertex(0,4,0)+meshVertex(2,4,0)+meshVertex(4,4,0);
            String[] flags={"16","17","48","49"};
            int[] gridEdgeCounts={12,15,15,18};
            for(int i=0;i<flags.length;i++){
                String dxf="0\nSECTION\n2\nENTITIES\n"+
                    "0\nPOLYLINE\n70\n"+flags[i]+"\n71\n3\n72\n3\n"+
                    vertices+"0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
                Files.writeString(file.toPath(),dxf,StandardCharsets.UTF_8);
                Dxf3dMesh mesh=Dxf3dMesh.read(file);
                int expectedCells=(i==3?9:i==1||i==2?6:4);
                if(mesh.xyz.length!=27||mesh.triangles.length!=expectedCells*6||
                   mesh.edges.length!=gridEdgeCounts[i]*2)
                    throw new AssertionError("polygon mesh open/closed grid mismatch case "+i);
                for(int k=0;k<mesh.edges.length;k+=2){
                    int a=mesh.edges[k],b=mesh.edges[k+1];
                    int rowA=a/3,rowB=b/3,colA=a%3,colB=b%3;
                    boolean neighborInRow=rowA==rowB&&
                        (Math.abs(colA-colB)==1||(i==1||i==3)&&Math.abs(colA-colB)==2);
                    boolean neighborInColumn=colA==colB&&
                        (Math.abs(rowA-rowB)==1||(i==2||i==3)&&Math.abs(rowA-rowB)==2);
                    if(!neighborInRow&&!neighborInColumn)
                        throw new AssertionError("synthetic polygon mesh diagonal leaked to wireframe");
                }
            }
            String base="0\nSECTION\n2\nENTITIES\n0\nPOLYLINE\n70\n16\n71\n3\n72\n3\n";
            String closing="0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
            String fewer=meshVertex(0,0,0)+meshVertex(2,0,0)+
                meshVertex(4,0,0)+meshVertex(0,2,0)+
                meshVertex(2,2,1)+meshVertex(4,2,0)+
                meshVertex(0,4,0)+meshVertex(2,4,0);
            String[] malformed={base+fewer+closing,base+vertices+meshVertex(6,6,6)+closing};
            for(int i=0;i<malformed.length;i++){
                Files.writeString(file.toPath(),malformed[i],StandardCharsets.UTF_8);
                boolean rejected=false;
                try{Dxf3dMesh.read(file);}catch(java.io.IOException expected){rejected=true;}
                if(!rejected)throw new AssertionError("polygon mesh missing/extra grid rows accepted");
            }
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
