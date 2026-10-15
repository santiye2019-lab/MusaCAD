import com.musa.cad.CadRevolveShell;
import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Independent partial-angle REVOLVE end-cap and geometry regression. */
public final class CadRevolveSectorTest {
    private static void yes(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    private static void near(double value,double expected,double tolerance,String why){
        if(!Double.isFinite(value)||Math.abs(value-expected)>tolerance)
            throw new AssertionError(why+" expected "+expected+" actual "+value);
    }
    private static float[] floats(double[] xyz){
        float[] result=new float[xyz.length];
        for(int i=0;i<xyz.length;i++)result[i]=(float)xyz[i];
        return result;
    }
    private static void invalid(Runnable command,String label){
        try{command.run();}catch(IllegalArgumentException ok){return;}
        throw new AssertionError("Accepted invalid sector REVOLVE: "+label);
    }
    private static Cad3dSignedVolume.Result inspect(CadRevolveShell.Solid shell,double angle,int steps,String label){
        yes(shell.isWatertight(),label+" index watertightness");
        yes(shell.sweepDegrees==angle,label+" angle metadata");
        int n=shell.sectionVertices;
        yes(shell.xyz.length/3==n*(steps+1),label+" section seam vertices");
        yes(shell.triangles.length/3==2*n*steps+2*(n-2),
            label+" wall triangles and two triangulated end caps");
        float[] coords=floats(shell.xyz);
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(coords,shell.triangles);
        yes(topology.checked&&topology.closedOrientedByIndex,label+" oriented cap topology");
        Cad3dSignedVolume.Result volume=Cad3dSignedVolume.measure(coords,shell.triangles,topology);
        yes(volume.available&&volume.signedVolume>0,label+" signed volume");
        Cad3dSurfaceMetrics.Result area=Cad3dSurfaceMetrics.measure(coords,shell.triangles);
        yes(area.degenerateCount==0,label+" no degenerate face");
        double angleStep=Math.toRadians(angle)/steps;
        double expectedSectorArea=0.5d*steps*Math.sin(angleStep)*(4-1);
        double expectedVolume=expectedSectorArea*3;
        double arc=(2d+1d)*(2d*Math.sin(angleStep/2d))*steps*3d;
        double expectedArea=2*expectedSectorArea+arc+2*3;
        near(volume.signedVolume,expectedVolume,.04,label+" exact faceted volume");
        near(area.area,expectedArea,.04,label+" faceted area including end caps");
        near(volume.centroidZ,1.5d,.005,label+" centroid height");
        return volume;
    }
    public static void main(String[] args)throws Exception {
        double[] section={1,0,2,0,2,3,1,3};
        int steps=48;
        CadRevolveShell.Solid quarter=CadRevolveShell.revolve(section,0,0,steps,90d);
        Cad3dSignedVolume.Result q=inspect(quarter,90d,steps,"quarter turn");
        yes(q.centroidX>0.5&&q.centroidY>0.5,"quarter-turn center in first quadrant");
        near(q.centroidX,q.centroidY,.005,"quarter-turn bisector symmetry");
        CadRevolveShell.Solid half=CadRevolveShell.revolve(section,0,0,steps,180d);
        Cad3dSignedVolume.Result h=inspect(half,180d,steps,"half turn");
        near(h.centroidX,0,.005,"half-turn mid-plane symmetry");
        yes(h.centroidY>0.5,"half turn centered above X axis");
        CadRevolveShell.Solid threeQuarter=CadRevolveShell.revolve(section,0,0,steps,270d);
        Cad3dSignedVolume.Result tq=inspect(threeQuarter,270d,steps,"270-degree turn");
        yes(tq.centroidX<0&&tq.centroidY>0,"270 sector centroid location");

        CadRevolveShell.Solid clockwise=CadRevolveShell.revolve(
            new double[]{1,3,2,3,2,0,1,0},0,0,steps,90d);
        inspect(clockwise,90d,steps,"clockwise input");
        CadRevolveShell.Solid shifted=CadRevolveShell.revolve(section,1000,-500,steps,90d);
        Cad3dSignedVolume.Result moved=inspect(shifted,90d,steps,"translated WCS");
        near(moved.centroidX,q.centroidX+1000,.005,"translated centroid X");
        near(moved.centroidY,q.centroidY-500,.005,"translated centroid Y");
        double[] concave={1,0,3,0,3,1,2,1,2,2,1,2};
        CadRevolveShell.Solid l=CadRevolveShell.revolve(concave,0,0,steps,90d);
        yes(l.isWatertight()&&
            Cad3dMeshTopology.inspect(floats(l.xyz),l.triangles).closedOrientedByIndex,
            "concave radial profile with triangulated watertight end caps");

        CadRevolveShell.Solid full=CadRevolveShell.revolve(section,0,0,steps,360d);
        yes(full.sweepDegrees==360d&&full.xyz.length/3==4*steps&&
            full.triangles.length/3==8*steps,
            "old full-turn closed seam remains unchanged");
        invalid(()->CadRevolveShell.revolve(section,0,0,steps,0d),"zero angle");
        invalid(()->CadRevolveShell.revolve(section,0,0,steps,4d),"under minimum angle");
        invalid(()->CadRevolveShell.revolve(section,0,0,steps,360.001d),"greater than one turn");
        invalid(()->CadRevolveShell.revolve(section,0,0,steps,-90d),"reverse sweep unsupported");
        invalid(()->CadRevolveShell.revolve(section,0,0,steps,Double.NaN),"NaN sweep");
        invalid(()->CadRevolveShell.revolve(new double[]{0,0,2,0,2,3,0,3},
            0,0,steps,90d),"axis-touching profile");

        File file=File.createTempFile("musacad-revolve-sector-", ".dxf");
        try{
            StringWriter writer=new StringWriter();
            quarter.writeDxf(writer,"MEKANIK_REVOLVE_90");
            Files.write(file.toPath(),writer.toString().getBytes(StandardCharsets.UTF_8));
            Dxf3dMesh reopened=Dxf3dMesh.read(file);
            yes(reopened.triangles.length==quarter.triangles.length,"DXF end caps survive readback");
            near(reopened.minZ,0d,1e-6,"DXF lower Z");
            near(reopened.maxZ,3d,1e-6,"DXF upper Z");
            near(Cad3dSurfaceMetrics.measure(reopened.xyz,reopened.triangles).area,
                Cad3dSurfaceMetrics.measure(floats(quarter.xyz),quarter.triangles).area,
                .04d,"DXF surface area parity including caps");
            yes(!Cad3dMeshTopology.inspect(reopened.xyz,reopened.triangles).closedOrientedByIndex,
                "unwelded DXF faces not falsely certified as ACIS watertight");
        }finally{if(!file.delete())file.deleteOnExit();}
        System.out.println("CAD REVOLVE angular sectors: end caps, volume, centroid and DXF reopened PASS");
    }
}
