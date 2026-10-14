import com.musa.cad.Cad3dMeshTopology;

public final class Cad3dMeshTopologyTest {
    private static final float[] TETRA={
        0,0,0, 1,0,0, 0,1,0, 0,0,1
    };
    // Four outward-oriented triangle faces.
    private static final int[] TETRA_FACES={
        0,2,1, 0,1,3, 0,3,2, 1,2,3
    };
    private static void need(boolean value,String why){
        if(!value)throw new AssertionError(why);
    }
    private static void invalid(float[] xyz,int[] triangles){
        try{
            Cad3dMeshTopology.inspect(xyz,triangles);
            throw new AssertionError("expected malformed mesh rejection");
        }catch(IllegalArgumentException expected){}
    }
    public static void main(String[] args){
        Cad3dMeshTopology.Result good=
                Cad3dMeshTopology.inspect(TETRA,TETRA_FACES);
        need(good.checked&&good.closedOrientedByIndex,
                "closed outward-oriented tetra has paired index edges");
        need(good.triangles==4&&good.boundaryEdges==0&&
             good.orientationConflicts==0&&good.nonManifoldEdges==0,
             "closed topology diagnostics");

        Cad3dMeshTopology.Result open=Cad3dMeshTopology.inspect(TETRA,
                new int[]{0,2,1, 0,1,3, 0,3,2});
        need(open.checked&&!open.closedOrientedByIndex&&
             open.boundaryEdges==3,"missing triangle has three boundary edges");

        Cad3dMeshTopology.Result wrongWinding=Cad3dMeshTopology.inspect(TETRA,
                new int[]{0,1,2, 0,1,3, 0,3,2, 1,2,3});
        need(!wrongWinding.closedOrientedByIndex&&
             wrongWinding.orientationConflicts==3,
             "one reversed triangle yields three winding conflicts");

        Cad3dMeshTopology.Result triple=Cad3dMeshTopology.inspect(TETRA,
                new int[]{0,2,1, 0,1,3, 0,3,2, 1,2,3, 0,2,1});
        need(!triple.closedOrientedByIndex&&triple.nonManifoldEdges==3,
                "duplicated face yields three non-manifold edges");

        // Two identical triangles facing opposite directions have all
        // paired edges, but enclose no solid. Do not certify a zero-thickness
        // CAD sheet as closed just because its edges pair.
        Cad3dMeshTopology.Result doubled=Cad3dMeshTopology.inspect(TETRA,
                new int[]{0,1,2, 0,2,1});
        need(doubled.boundaryEdges==0&&doubled.orientationConflicts==0,
                "opposite faces can trick edge-only manifold checks");
        need(doubled.duplicateFaces==1&&!doubled.closedOrientedByIndex,
                "duplicate opposing facet prevents false solid certification");

        Cad3dMeshTopology.Result collinear=Cad3dMeshTopology.inspect(
                new float[]{0,0,0, 1,1,1, 2,2,2},
                new int[]{0,1,2});
        need(collinear.degenerateTriangles==1&&!collinear.closedOrientedByIndex,
                "zero-area face cannot close a valid shell");

        Cad3dMeshTopology.Result repeated=Cad3dMeshTopology.inspect(TETRA,
                new int[]{0,0,2});
        need(repeated.degenerateTriangles==1,"repeated vertex rejected");

        // Geometrically matching duplicate coordinates are deliberately NOT
        // welded: a CAD export must not claim a solid on unshared indices.
        float[] disconnected={0,0,0, 1,0,0, 0,1,0, 0,0,0, 1,0,0, 0,0,1};
        Cad3dMeshTopology.Result split=Cad3dMeshTopology.inspect(disconnected,
                new int[]{0,1,2, 3,4,5});
        need(!split.closedOrientedByIndex&&split.boundaryEdges==6,
                "duplicate coincident vertex coordinates stay open");

        invalid(TETRA,new int[]{0,1,99});
        invalid(new float[]{Float.NaN,0,0,1,0,0,0,1,0},new int[]{0,1,2});
        invalid(TETRA,new int[]{0,1});
        invalid(new float[]{0,1},new int[]{});
        int[] huge=new int[(Cad3dMeshTopology.MAX_AUDITED_TRIANGLES+1)*3];
        Cad3dMeshTopology.Result skipped=Cad3dMeshTopology.inspect(TETRA,huge);
        need(!skipped.checked&&!skipped.closedOrientedByIndex,
            "huge CAD drawing not falsely certified without audit");

        System.out.println("CAD 3D mesh topological shell audit: manifold, winding, open edges PASS");
    }
}
