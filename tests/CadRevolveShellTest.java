import com.musa.cad.CadRevolveShell;
import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Dxf3dMesh;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public final class CadRevolveShellTest {
    private static void yes(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    private static void near(double actual,double expected,double epsilon,String message){
        if(!Double.isFinite(actual)||Math.abs(actual-expected)>epsilon)
            throw new AssertionError(message+" expected "+expected+" actual "+actual);
    }
    private static void fail(Runnable code,String label){
        try{code.run();}catch(IllegalArgumentException expected){return;}
        throw new AssertionError("Invalid REVOLVE geometry accepted: "+label);
    }
    private static float[] toFloat(double[] d){
        float[] f=new float[d.length];
        for(int i=0;i<d.length;i++)f[i]=(float)d[i];
        return f;
    }
    private static void assertShell(CadRevolveShell.Solid solid,double expectedVolume,
                                    double expectedArea,double x,double y,double z,String label){
        yes(solid.isWatertight(),label+": mesh index edge incidence");
        float[] xyz=toFloat(solid.xyz);
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(xyz,solid.triangles);
        yes(topology.checked&&topology.closedOrientedByIndex,label+": topology audit");
        Cad3dSignedVolume.Result mass=Cad3dSignedVolume.measure(xyz,solid.triangles,topology);
        yes(mass.available,label+": signed volume must be available");
        near(mass.signedVolume,expectedVolume,0.025,label+": signed polygonal volume");
        near(mass.centroidX,x,0.01,label+": XYZ center X");
        near(mass.centroidY,y,0.01,label+": XYZ center Y");
        near(mass.centroidZ,z,0.01,label+": XYZ center Z");
        Cad3dSurfaceMetrics.Result surface=Cad3dSurfaceMetrics.measure(xyz,solid.triangles);
        near(surface.area,expectedArea,0.025,label+": triangulated surface area");
        yes(surface.degenerateCount==0,label+": no manufactured zero area faces");
    }
    public static void main(String[] args)throws Exception {
        int segments=64;
        double[] section={1,0,2,0,2,3,1,3};
        CadRevolveShell.Solid ring=CadRevolveShell.revolve(section,0,0,segments);
        near(ring.profileArea,3,1e-12,"radial-Z section area");
        yes(ring.sectionVertices==4&&ring.angularSegments==segments,"profile and angular counts");
        yes(ring.xyz.length/3==4*segments&&ring.triangles.length/3==8*segments,
            "faceted full revolution triangle and point inventory");
        double theta=2*Math.PI/segments;
        double area=0.5*segments*Math.sin(theta)*(4-1);
        double expectedVol=area*3;
        double outerAndInner=2*(2+1)*Math.sin(theta/2)*segments*3;
        double expectedSurface=2*area+outerAndInner;
        assertShell(ring,expectedVol,expectedSurface,0,0,1.5,"rectangular full-turn section");

        CadRevolveShell.Solid reversed=CadRevolveShell.revolve(
            new double[]{1,3,2,3,2,0,1,0},0,0,segments);
        assertShell(reversed,expectedVol,expectedSurface,0,0,1.5,"clockwise profile");
        CadRevolveShell.Solid translated=CadRevolveShell.revolve(section,10000,-20000,segments);
        assertShell(translated,expectedVol,expectedSurface,10000,-20000,1.5,"world-offset axis");

        double[] concave={1,0,3,0,3,1,2,1,2,2,1,2};
        CadRevolveShell.Solid partial=CadRevolveShell.revolve(concave,0,0,segments);
        yes(partial.isWatertight()&&partial.triangles.length/3==12*segments,
            "valid concave polygonal radial section");
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(
            toFloat(partial.xyz),partial.triangles);
        yes(topology.closedOrientedByIndex,"concave revolution has oriented shell");
        yes(Cad3dSignedVolume.measure(toFloat(partial.xyz),partial.triangles,topology).signedVolume>0,
            "concave revolution has positive signed volume");

        fail(()->CadRevolveShell.revolve(new double[]{0,0,2,0,2,3,0,3},0,0,64),"touches axis");
        fail(()->CadRevolveShell.revolve(new double[]{-1,0,2,0,2,3,-1,3},0,0,64),"crosses axis");
        fail(()->CadRevolveShell.revolve(new double[]{1,0,3,3,1,3,3,0},0,0,64),"bow tie");
        fail(()->CadRevolveShell.revolve(new double[]{1,0,2,0,2,0,1,3},0,0,64),"repeated point");
        fail(()->CadRevolveShell.revolve(section,0,0,11),"insufficient segments");
        fail(()->CadRevolveShell.revolve(section,0,0,129),"excess segments");
        fail(()->CadRevolveShell.revolve(section,Double.NaN,0,64),"bad axis");
        fail(()->CadRevolveShell.revolve(new double[]{1,0,2,0,2,Double.POSITIVE_INFINITY,1,3},0,0,64),"bad Z");

        StringWriter buffer=new StringWriter();ring.writeDxf(buffer,"MEKANIK_REVOLVE");
        String raw=buffer.toString();
        yes(raw.contains("8\nMEKANIK_REVOLVE\n")&&raw.endsWith("0\nEOF\n"),
            "DXF layer and EOF semantics");
        File tmp=File.createTempFile("musacad-revolve-", ".dxf");
        try{
            Files.write(tmp.toPath(),raw.getBytes(StandardCharsets.UTF_8));
            Dxf3dMesh loaded=Dxf3dMesh.read(tmp);
            yes(loaded.triangles.length==ring.triangles.length,
                "DXF reopening must retain triangle count");
            near(loaded.minZ,0d,1e-5,"DXF reopened bottom");
            near(loaded.maxZ,3d,1e-5,"DXF reopened top");
            Cad3dSurfaceMetrics.Result restored=Cad3dSurfaceMetrics.measure(loaded.xyz,loaded.triangles);
            near(restored.area,expectedSurface,0.025,"DXF reopened surface area");
            yes(!Cad3dMeshTopology.inspect(loaded.xyz,loaded.triangles).closedOrientedByIndex,
                "Unwelded independent DXF 3DFACE vertices must not be falsely certified watertight");
        }finally{if(!tmp.delete())tmp.deleteOnExit();}
        try{ring.writeDxf(new StringWriter(),"BAD\n0\nLINE");
            throw new AssertionError("DXF layer injection accepted");
        }catch(IllegalArgumentException expected){}
        System.out.println("CAD 3D bounded polygonal REVOLVE: profile, shell, orientation, measurements and DXF reopen PASS");
    }
}
