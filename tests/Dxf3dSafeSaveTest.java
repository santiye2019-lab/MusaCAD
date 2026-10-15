import com.musa.cad.Dxf3dMesh;
import com.musa.cad.Dxf3dSafeSave;
import com.musa.cad.CadSaveSourceIntegrity;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

public final class Dxf3dSafeSaveTest {
    private static void yes(boolean value,String message){if(!value)throw new AssertionError(message);}
    public static void main(String[] args)throws Exception{
        File source=File.createTempFile("musacad-3d-safe-source-", ".dxf");
        File cache=Files.createTempDirectory("musacad-3d-safe-").toFile();
        String dxf="0\nSECTION\n2\nENTITIES\n"
            +"0\n3DFACE\n10\n0\n20\n0\n30\n1\n11\n3\n21\n0\n31\n1\n"
            +"12\n0\n22\n4\n32\n1\n999\nNOTE-A\n0\nENDSEC\n0\nEOF\n";
        try{
            Files.writeString(source.toPath(),dxf,StandardCharsets.UTF_8);
            byte[] originalBytes=Files.readAllBytes(source.toPath());
            CadSaveSourceIntegrity openedSource=CadSaveSourceIntegrity.capture(source);
            Dxf3dMesh mesh=Dxf3dMesh.read(source);
            openedSource.requireUnchanged(source);
            float[] before=mesh.xyz.clone();
            mesh.xyz[2]=9f;
            Dxf3dSafeSave.VerifiedStage staged=Dxf3dSafeSave.stage(source,mesh,before,cache,openedSource);
            try{
                yes(Arrays.equals(originalBytes,Files.readAllBytes(source.toPath())),
                    "export must not mutate source");
                Dxf3dMesh reopen=Dxf3dMesh.read(staged.file());
                yes(reopen.xyz[2]==9f,"edited 3D Z must survive export roundtrip");
                ByteArrayOutputStream destination=new ByteArrayOutputStream();
                long copied=Dxf3dSafeSave.publish(staged,()->destination);
                yes(copied==staged.file().length()&&
                    Arrays.equals(destination.toByteArray(),Files.readAllBytes(staged.file().toPath())),
                    "publish must copy the validated staged file");
                // Even after correct save/reopen, a staged file may be
                // swapped or modified before the Android SAF picker returns.
                // Verify the immutable stage hash BEFORE opener.open() can
                // truncate an existing destination.
                byte[] altered=Files.readAllBytes(staged.file().toPath());
                altered[altered.length-10]^=0x01;
                Files.write(staged.file().toPath(),altered);
                final boolean[] opened={false};
                boolean tamperingRejected=false;
                try{
                    Dxf3dSafeSave.publish(staged,()->{
                        opened[0]=true;
                        return new ByteArrayOutputStream();
                    });
                }catch(IOException expected){tamperingRejected=true;}
                yes(tamperingRejected&&!opened[0],
                    "tampered stage must be rejected BEFORE destination open");
            }finally{staged.delete();}

            // Source geometry changed before staging: same-sized bytes and
            // restored mtime would have passed a save-time SHA snapshot alone.
            // The new Z at the edited vertex could otherwise be masked by the
            // pending edit, allowing a false-success DXF round-trip.
            long originalStamp=source.lastModified();
            String stale=dxf.replace("30\n1\n11\n3\n","30\n2\n11\n3\n");
            yes(!stale.equals(dxf)&&stale.length()==dxf.length(),
                "stale source test must preserve source byte count");
            Files.writeString(source.toPath(),stale,StandardCharsets.UTF_8);
            source.setLastModified(originalStamp);
            boolean staleRejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache,openedSource);}
            catch(IOException expected){staleRejected=true;}
            yes(staleRejected,"changed source geometry must reject stale 3D edits");
            yes(cache.listFiles()!=null&&cache.listFiles().length==0,
                "stale source must not leave a temporary export");
            Files.write(source.toPath(),originalBytes);

            // Non-geometric metadata/XREF edits do not change mesh XYZ or
            // topology. The opening-session byte hash must still reject them.
            long metadataStamp=source.lastModified();
            String alteredMetadata=dxf.replace("NOTE-A","NOTE-B");
            yes(!alteredMetadata.equals(dxf)&&alteredMetadata.length()==dxf.length(),
                "non-geometric same-length fixture");
            Files.writeString(source.toPath(),alteredMetadata,StandardCharsets.UTF_8);
            source.setLastModified(metadataStamp);
            Dxf3dMesh metadataMesh=Dxf3dMesh.read(source);
            yes(Arrays.equals(metadataMesh.xyz,before)&&
                Arrays.equals(metadataMesh.triangles,mesh.triangles),
                "metadata-only edit must preserve visible XYZ and triangles");
            boolean metadataRejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache,openedSource);}
            catch(IOException expected){metadataRejected=true;}
            yes(metadataRejected,"changed unrendered source metadata must reject stale export");
            yes(cache.listFiles()!=null&&cache.listFiles().length==0,
                "metadata mismatch must not produce a staged DXF");
            Files.write(source.toPath(),originalBytes);

            // Missing edited DXF tag: stage must fail, never touching an existing destination.
            Files.writeString(source.toPath(),dxf.replace("30\n1\n",""),StandardCharsets.UTF_8);
            ByteArrayOutputStream existing=new ByteArrayOutputStream();
            existing.write("KEEP-ORIGINAL".getBytes(StandardCharsets.UTF_8));
            boolean rejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache,openedSource);}
            catch(IOException expected){rejected=true;}
            yes(rejected,"missing coordinate must reject 3D export before destination open");
            yes(existing.toString(StandardCharsets.UTF_8).equals("KEEP-ORIGINAL"),
                "rejected staged export must not overwrite destination");
            yes(cache.listFiles()!=null&&cache.listFiles().length==0,
                "failed staged export must delete temporary file");

            // Invalid coordinate also cannot leave an export file behind.
            Files.write(source.toPath(),originalBytes);
            mesh.xyz[2]=Float.NaN;
            rejected=false;
            try{Dxf3dSafeSave.stage(source,mesh,before,cache,openedSource);}
            catch(IOException expected){rejected=true;}
            yes(rejected&&cache.listFiles()!=null&&cache.listFiles().length==0,
                "non-finite geometry must not produce a staged or destination file");

            // An independent source LINE edit must not rewrite LWPOLYLINE
            // bulges, elevation or tessellated non-editable helper vertices.
            String withPolyline="0\nSECTION\n2\nENTITIES\n"+
                "0\nLWPOLYLINE\n90\n2\n38\n4\n"+
                "10\n0\n20\n0\n42\n1\n10\n4\n20\n0\n"+
                "0\nLINE\n10\n0\n20\n1\n30\n2\n11\n2\n21\n1\n31\n3\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(source.toPath(),withPolyline,StandardCharsets.UTF_8);
            CadSaveSourceIntegrity polylineOpened=CadSaveSourceIntegrity.capture(source);
            Dxf3dMesh mixed=Dxf3dMesh.read(source);
            yes(mixed.approximateCurves==1&&mixed.xyz.length==27*3,
                "mixed DXF must read bulge path and editable straight line");
            float[] mixOriginal=mixed.xyz.clone();
            int editableLineVertex=25;
            yes(mixed.sourceLines[editableLineVertex]>=0,
                "source LINE must remain editable after polyline tessellation");
            mixed.xyz[editableLineVertex*3+2]=8f;
            Dxf3dSafeSave.VerifiedStage mixedStaged=
                Dxf3dSafeSave.stage(source,mixed,mixOriginal,cache,polylineOpened);
            try{
                String output=Files.readString(mixedStaged.file().toPath(),StandardCharsets.UTF_8);
                yes(output.contains("42\n1\n")&&output.contains("38\n4\n"),
                    "safe save must keep original LWP bulge and elevation tags");
                Dxf3dMesh mixedAgain=Dxf3dMesh.read(mixedStaged.file());
                yes(mixedAgain.approximateCurves==1&&
                    Arrays.equals(mixedAgain.xyz,mixed.xyz)&&
                    Arrays.equals(mixedAgain.edges,mixed.edges),
                    "save/reopen must retain curve plus editable LINE geometry");
            }finally{mixedStaged.delete();}

            // A LINE coordinate edit must leave the original ELLIPSE axes
            // and rational SPLINE knots/weights byte-identical. The curved
            // tessellation is display-only and must reopen to the same XYZ.
            String ellSpline="0\nSECTION\n2\nENTITIES\n"+
                "0\nELLIPSE\n10\n1\n20\n2\n30\n3\n"+
                "11\n4\n21\n0\n31\n0\n40\n0.5\n"+
                "0\nSPLINE\n70\n4\n71\n2\n72\n6\n73\n3\n"+
                "40\n0\n40\n0\n40\n0\n40\n1\n40\n1\n40\n1\n"+
                "41\n1\n41\n0.7071067811865476\n41\n1\n"+
                "10\n1\n20\n0\n10\n1\n20\n1\n10\n0\n20\n1\n"+
                "0\nLINE\n10\n0\n20\n1\n30\n2\n11\n2\n21\n1\n31\n3\n"+
                "0\nENDSEC\n0\nEOF\n";
            Files.writeString(source.toPath(),ellSpline,StandardCharsets.UTF_8);
            CadSaveSourceIntegrity newSession=CadSaveSourceIntegrity.capture(source);
            Dxf3dMesh curved=Dxf3dMesh.read(source);
            yes(curved.approximateCurves==2&&curved.xyz.length==75*3,
                "ELLIPSE/SPLINE source geometry must be visible");
            float[] curvedOriginal=curved.xyz.clone();
            int sourceLine=73;
            yes(curved.sourceLines[sourceLine]>=0,
                "editable LINE must follow the read-only ELLIPSE/SPLINE samples");
            curved.xyz[sourceLine*3+2]=8f;
            Dxf3dSafeSave.VerifiedStage curvesSaved=
                Dxf3dSafeSave.stage(source,curved,curvedOriginal,cache,newSession);
            try{
                String out=Files.readString(curvesSaved.file().toPath(),StandardCharsets.UTF_8);
                yes(out.contains("41\n0.7071067811865476\n")&&
                    out.contains("11\n4\n21\n0\n31\n0\n40\n0.5\n"),
                    "safe save must retain rational spline weights and ellipse axes");
                Dxf3dMesh reopenedCurves=Dxf3dMesh.read(curvesSaved.file());
                yes(reopenedCurves.approximateCurves==2&&
                    Arrays.equals(curved.xyz,reopenedCurves.xyz)&&
                    Arrays.equals(curved.edges,reopenedCurves.edges)&&
                    Arrays.equals(curved.sourceLines,reopenedCurves.sourceLines),
                    "save/reopen must retain exact sampled curve geometry while editing independent LINE");
            }finally{curvesSaved.delete();}
        }finally{
            Files.deleteIfExists(source.toPath());
            File[] abandoned=cache.listFiles();
            if(abandoned!=null)for(File f:abandoned)Files.deleteIfExists(f.toPath());
            Files.deleteIfExists(cache.toPath());
        }
        System.out.println("3D DXF staged save: verified roundtrip, source intact, invalid output never published");
    }
}
