import com.musa.cad.CadPrismBrep;
import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Dxf3dMesh;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/**
 * Host-only bounded 3D extrusion -> DXF 3DFACE -> MusaCAD reopen parity.
 * It does NOT prove native binary DWG writing, ACIS B-Rep or all AutoCAD operations.
 */
public final class CadPrismBrepRoundtripTest {
    private static void expect(boolean value,String why){if(!value)throw new AssertionError(why);}
    private static void near(double actual,double expected,double precision,String why){
        if(!Double.isFinite(actual)||Math.abs(actual-expected)>precision)
            throw new AssertionError(why+" expected "+expected+" got "+actual);
    }
    private static float[] asFloat(double[] source){
        float[] out=new float[source.length];
        for(int i=0;i<out.length;i++)out[i]=(float)source[i];
        return out;
    }
    public static void main(String[]args)throws Exception{
        double[] square={0,0,2,0,2,2,0,2};
        CadPrismBrep.Solid solid=CadPrismBrep.extrude(square,5,3);
        float[] coords=asFloat(solid.xyz);
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(coords,solid.triangles);
        expect(topology.checked&&topology.closedOrientedByIndex,
            "generated polygonal extrusion must have a consistently oriented indexed shell");
        Cad3dSignedVolume.Result volume=Cad3dSignedVolume.measure(coords,solid.triangles,topology);
        expect(volume.available,"generated shell must provide provisional volume");
        near(volume.signedVolume,12d,1e-9,"solid signed volume");
        near(volume.centroidX,1d,1e-9,"extruded solid centroid x");
        near(volume.centroidY,1d,1e-9,"extruded solid centroid y");
        near(volume.centroidZ,6.5d,1e-9,"extruded solid centroid z");
        Cad3dSurfaceMetrics.Result surface=Cad3dSurfaceMetrics.measure(coords,solid.triangles);
        near(surface.area,32d,1e-9,"prismatic surface triangle area");
        near(surface.centroidX,1d,1e-9,"surface center x");
        near(surface.centroidY,1d,1e-9,"surface center y");
        near(surface.centroidZ,6.5d,1e-9,"surface center z");
        File file=File.createTempFile("musacad-prism-", ".dxf");
        try{
            try(Writer writer=new OutputStreamWriter(new FileOutputStream(file),StandardCharsets.UTF_8)){
                solid.writeDxf(writer,"MEKANIK_TESISAT");
            }
            String raw=new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8);
            expect(raw.contains("8\nMEKANIK_TESISAT\n"),"discipline layer must survive export");
            Dxf3dMesh loaded=Dxf3dMesh.read(file);
            expect(loaded.triangles.length/3==solid.triangles.length/3,
                "3DFACE readback must preserve face count and tessellation");
            expect(loaded.xyz.length/3==4*(solid.triangles.length/3),
                "3DFACE format carries four source corners (triangle 4th equals 3rd)");
            near(loaded.minZ,5d,1e-6,"DXF reopened bottom Z");
            near(loaded.maxZ,8d,1e-6,"DXF reopened top Z");
            Cad3dSurfaceMetrics.Result reopened=Cad3dSurfaceMetrics.measure(loaded.xyz,loaded.triangles);
            near(reopened.area,surface.area,1e-5,"DXF reopened area parity");
            // Distinct 3DFACE rows decode as separate vertices; never mistake
            // them for a welded topology-certified native ACIS solid.
            expect(!Cad3dMeshTopology.inspect(loaded.xyz,loaded.triangles).closedOrientedByIndex,
                "unwelded 3DFACE DXF must not be falsely certified as watertight");
        }finally{if(!file.delete())file.deleteOnExit();}
        System.out.println("CAD EXTRUDE polygonal shell: topology, volume, centroid, surface and DXF reopen PASS");
    }
}
