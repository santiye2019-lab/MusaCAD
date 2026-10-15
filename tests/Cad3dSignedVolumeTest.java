import com.musa.cad.Cad3dMeshTopology;
import com.musa.cad.Cad3dSignedVolume;

public final class Cad3dSignedVolumeTest {
    private static void need(boolean ok,String why){
        if(!ok)throw new AssertionError(why);
    }
    private static void close(double actual,double expected,double tolerance,String label){
        if(Math.abs(actual-expected)>tolerance)
            throw new AssertionError(label+" "+actual+" expected "+expected);
    }
    private static Cad3dSignedVolume.Result volume(float[] xyz,int[] faces){
        return Cad3dSignedVolume.measure(xyz,faces,
                Cad3dMeshTopology.inspect(xyz,faces));
    }
    public static void main(String[] args){
        float[] tetra={0,0,0,1,0,0,0,1,0,0,0,1};
        int[] outward={0,2,1,0,1,3,0,3,2,1,2,3};
        Cad3dSignedVolume.Result good=volume(tetra,outward);
        need(good.available&&good.triangleCount==4,
                "oriented tetra mesh supports provisional signed volume");
        close(good.signedVolume,1d/6d,1e-12,"unit tetrahedron");
        close(good.centroidX,0.25d,1e-12,"tetra centroid X");
        close(good.centroidY,0.25d,1e-12,"tetra centroid Y");
        close(good.centroidZ,0.25d,1e-12,"tetra centroid Z");
        int[] inward=outward.clone();
        for(int i=0;i<inward.length;i+=3){
            int tmp=inward[i+1];inward[i+1]=inward[i+2];inward[i+2]=tmp;
        }
        Cad3dSignedVolume.Result reverse=volume(tetra,inward);
        need(reverse.available,"all-inward orientation is consistent");
        close(reverse.signedVolume,-1d/6d,1e-12,"inward orientation keeps signed result");
        close(reverse.magnitude(),1d/6d,1e-12,"inward magnitude");
        close(reverse.centroidX,0.25d,1e-12,"reversed tetra centroid X");
        close(reverse.centroidY,0.25d,1e-12,"reversed tetra centroid Y");
        close(reverse.centroidZ,0.25d,1e-12,"reversed tetra centroid Z");

        float[] translated=tetra.clone();
        for(int i=0;i<translated.length;i++)translated[i]+=1000000f;
        Cad3dSignedVolume.Result offset=volume(translated,outward);
        close(offset.signedVolume,1d/6d,1e-12,
                "large-world-offset coordinates are origin independent");
        close(offset.centroidX,1000000.25d,1e-9,"translated centroid X");
        close(offset.centroidY,1000000.25d,1e-9,"translated centroid Y");
        close(offset.centroidZ,1000000.25d,1e-9,"translated centroid Z");

        float[] cube={
            0,0,0, 1,0,0, 1,1,0, 0,1,0,
            0,0,1, 1,0,1, 1,1,1, 0,1,1
        };
        int[] cubeFaces={
            0,1,2,0,2,3,4,6,5,4,7,6,
            0,4,5,0,5,1,1,5,6,1,6,2,
            2,6,7,2,7,3,3,7,4,3,4,0
        };
        Cad3dSignedVolume.Result box=volume(cube,cubeFaces);
        need(box.available,"indexed cube topology closes");
        close(box.magnitude(),1d,1e-12,"unit cube");
        close(box.centroidX,0.5d,1e-12,"cube centroid X");
        close(box.centroidY,0.5d,1e-12,"cube centroid Y");
        close(box.centroidZ,0.5d,1e-12,"cube centroid Z");
        close(volume(cube,new int[]{0,1,2}).magnitude(),0d,0d,
                "open face never provides final-looking mesh volume");
        Cad3dSignedVolume.Result open=volume(cube,new int[]{0,1,2});
        need(!open.available,"open mesh must reject signed volume");
        need(Double.isNaN(open.centroidX)&&Double.isNaN(open.centroidY)&&Double.isNaN(open.centroidZ),
                "open mesh must never expose a plausible-looking centroid");
        Cad3dSignedVolume.Result doubled=volume(tetra,
                new int[]{0,1,2,0,2,1});
        need(!doubled.available,"two coincident double-sided facets have no valid volume");

        int[] huge=new int[(Cad3dMeshTopology.MAX_AUDITED_TRIANGLES+1)*3];
        need(!volume(tetra,huge).available,"unaudited huge CAD shell not certified");

        try{
            Cad3dSignedVolume.measure(tetra,new int[]{0,1,2},null);
            throw new AssertionError("missing topology preflight should be rejected");
        }catch(IllegalArgumentException expected){}
        System.out.println("CAD signed mesh volume and centroid: tetra, cube, reverse winding, offsets and open-shell rejection PASS");
    }
}
