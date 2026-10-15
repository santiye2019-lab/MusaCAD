import com.musa.cad.CadPrismBrep;
import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;
import com.musa.cad.Cad3dSurfaceMetrics;
import com.musa.cad.Dxf3dMesh;
import java.io.File;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Limited non-axial polygonal EXTRUDE / straight SWEEP geometry regression. */
public final class CadObliqueSweepTest {
    private static void yes(boolean pass,String why){
        if(!pass)throw new AssertionError(why);
    }
    private static void near(double value,double target,double tolerance,String label){
        if(!Double.isFinite(value)||Math.abs(value-target)>tolerance)
            throw new AssertionError(label+": expected "+target+" got "+value);
    }
    private static void invalid(Runnable task,String label){
        try{task.run();}
        catch(IllegalArgumentException expected){return;}
        throw new AssertionError("Invalid straight SWEEP accepted: "+label);
    }
    private static float[] floats(double[] xyz){
        float[] out=new float[xyz.length];
        for(int i=0;i<out.length;i++)out[i]=(float)xyz[i];
        return out;
    }
    private static void inspect(CadPrismBrep.Solid solid,double volume,
                                double surface,double cx,double cy,double cz,String label){
        yes(solid.isWatertight(),label+" indexed topological shell");
        near(solid.volume,volume,1e-9,label+" analytic volume");
        near(solid.surfaceArea,surface,1e-8,label+" analytic surface");
        near(solid.centroidX,cx,1e-9,label+" analytic center X");
        near(solid.centroidY,cy,1e-9,label+" analytic center Y");
        near(solid.centroidZ,cz,1e-9,label+" analytic center Z");
        float[] v=floats(solid.xyz);
        Cad3dMeshTopology.Result topology=Cad3dMeshTopology.inspect(v,solid.triangles);
        yes(topology.checked&&topology.closedOrientedByIndex,label+" manifold by oriented indices");
        Cad3dSignedVolume.Result signed=Cad3dSignedVolume.measure(v,solid.triangles,topology);
        yes(signed.available,label+" signed tetrahedron mass properties available");
        near(signed.signedVolume,volume,1e-5,label+" independent volume");
        near(signed.centroidX,cx,1e-4,label+" independent center X");
        near(signed.centroidY,cy,1e-4,label+" independent center Y");
        near(signed.centroidZ,cz,1e-4,label+" independent center Z");
        Cad3dSurfaceMetrics.Result area=Cad3dSurfaceMetrics.measure(v,solid.triangles);
        near(area.area,surface,1e-5,label+" independent triangular area");
        yes(area.degenerateCount==0,label+" no degenerate triangles");
    }
    public static void main(String[] args)throws Exception{
        double[] square={0,0,2,0,2,2,0,2};
        double sideArea=4d*Math.hypot(3d,4d)+4d*Math.hypot(3d,3d);
        double surface=8d+sideArea;
        CadPrismBrep.Solid slanted=CadPrismBrep.sweepLinear(square,5,3,4,3);
        inspect(slanted,12d,surface,2.5d,3d,6.5d,"slanted square");
        yes(slanted.xyz.length==24&&slanted.triangles.length==36,
            "swept prism preserves rings and triangulated caps");
        near(slanted.xyz[12],3d,1e-9,"destination upper X shifts 3");
        near(slanted.xyz[13],4d,1e-9,"destination upper Y shifts 4");
        near(slanted.xyz[14],8d,1e-9,"destination upper Z reaches 8");

        CadPrismBrep.Solid down=CadPrismBrep.sweepLinear(square,5,3,4,-3);
        inspect(down,12d,surface,2.5d,3d,3.5d,"negative Z displacement");
        near(down.xyz[0],3d,1e-9,"downward move lower destination ring X");
        near(down.xyz[1],4d,1e-9,"downward move lower destination ring Y");
        near(down.xyz[2],2d,1e-9,"downward move lower Z reaches 2");
        near(down.xyz[12],0d,1e-9,"upper original source unchanged X");

        CadPrismBrep.Solid cw=CadPrismBrep.sweepLinear(
            new double[]{0,2,2,2,2,0,0,0},5,3,4,3);
        inspect(cw,12d,surface,2.5d,3d,6.5d,"clockwise input orientation");

        CadPrismBrep.Solid zOnly=CadPrismBrep.sweepLinear(square,5,0,0,3);
        CadPrismBrep.Solid original=CadPrismBrep.extrude(square,5,3);
        near(zOnly.surfaceArea,original.surfaceArea,1e-9,"axial geometry unchanged");
        inspect(zOnly,12d,32d,1d,1d,6.5d,"axial backward compatibility");
        yes(zOnly.triangles.length==original.triangles.length,"original caps kept");

        CadPrismBrep.Solid moved=CadPrismBrep.sweepLinear(
            new double[]{100000,200000,100002,200000,100002,200002,100000,200002},
            90000,3,4,3);
        inspect(moved,12d,surface,100002.5,200003,90001.5,"large WCS");

        // Concave L profile: shear does not alter volume and cannot fold
        // a validated watertight polygonal extrusion into self-intersection.
        double[] concave={0,0,3,0,3,1,1,1,1,3,0,3};
        CadPrismBrep.Solid l=CadPrismBrep.sweepLinear(concave,0,2,3,2);
        yes(l.isWatertight()&&l.triangles.length/3==20,"concave path topology");
        near(l.volume,10d,1e-9,"concave volume preserved");
        near(l.centroidX,2.1d,1e-9,"concave centroid X");
        near(l.centroidY,2.6d,1e-9,"concave centroid Y");
        near(l.centroidZ,1d,1e-9,"concave centroid Z");
        inspect(l,10d,l.surfaceArea,2.1d,2.6d,1d,"concave vector extrusion");

        yes(square[0]==0d&&square[1]==0d&&square[2]==2d,
            "original source polygon must not mutate");
        invalid(()->CadPrismBrep.sweepLinear(square,0,1,2,0),"coplanar vector");
        invalid(()->CadPrismBrep.sweepLinear(square,0,1,2,1e-10),"almost coplanar");
        invalid(()->CadPrismBrep.sweepLinear(square,0,Double.NaN,1,2),"NaN displacement X");
        invalid(()->CadPrismBrep.sweepLinear(square,0,1,Double.POSITIVE_INFINITY,2),"inf displacement Y");
        invalid(()->CadPrismBrep.sweepLinear(square,0,1,2,Double.NaN),"NaN displacement Z");
        invalid(()->CadPrismBrep.sweepLinear(square,0,1e9,1e9,3),
                "coordinates exceeding finite model bounds");
        invalid(()->CadPrismBrep.sweepLinear(new double[]{0,0,2,2,0,2,2,0},
                0,1,1,3),"crossing polygon source");

        File file=File.createTempFile("musacad-oblique-sweep-", ".dxf");
        try{
            StringWriter dxf=new StringWriter();
            slanted.writeDxf(dxf,"MEKANIK_KANAL");
            String payload=dxf.toString();
            yes(payload.contains("8\nMEKANIK_KANAL\n"),"DXF original layer retained");
            Files.write(file.toPath(),payload.getBytes(StandardCharsets.UTF_8));
            Dxf3dMesh read=Dxf3dMesh.read(file);
            yes(read.triangles.length==slanted.triangles.length,
                "original tessellation count survives DXF reopen");
            near(read.minZ,5d,1e-6,"readback min Z");
            near(read.maxZ,8d,1e-6,"readback max Z");
            near(Cad3dSurfaceMetrics.measure(read.xyz,read.triangles).area,
                surface,1e-5,"readback triangulated surface area");
            yes(!Cad3dMeshTopology.inspect(read.xyz,read.triangles).closedOrientedByIndex,
                "unwelded DXF faces must not be falsely certified native ACIS");
        }finally{if(!file.delete())file.deleteOnExit();}
        System.out.println("CAD oblique SWEEP linear: direction, signed volume, center, area, WCS, DXF reopen PASS");
    }
}
