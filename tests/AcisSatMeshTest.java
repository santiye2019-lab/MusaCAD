import com.musa.cad.AcisSatMesh;
import java.util.Arrays;
import java.util.List;

public final class AcisSatMeshTest {
    public static void main(String[] args){
        String sat=cubeSat();
        String encrypted=crypt(sat);
        AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(Arrays.asList(
            encrypted.substring(0,encrypted.length()/2),
            encrypted.substring(encrypted.length()/2)
        ));
        if(!mesh.hasGeometry())throw new AssertionError("encrypted ACIS SAT must produce geometry");
        if(!mesh.complete||mesh.faces!=6||mesh.tessellatedFaces!=6||mesh.unsupportedFaces!=0)
            throw new AssertionError("cube planar faces must tessellate completely");
        if(mesh.xyz.length!=24)throw new AssertionError("cube must deduplicate to 8 real vertices");
        if(mesh.triangles.length!=36)throw new AssertionError("cube must produce 12 triangles");
        if(mesh.edges.length!=24)throw new AssertionError("cube must preserve 12 real B-Rep edges");

        testCylinderSide();
        testImplicitInvertedCoedgeLoop();
        testUnsupportedFaceWireframe();
        testMissingAcisReferencesStillCountFaces();

        AcisSatMesh.Result bad=AcisSatMesh.parseDxfChunks(List.of("ACIS_PLACEHOLDER"));
        if(bad.hasGeometry())throw new AssertionError("invalid ACIS payload must not invent geometry");
        if(bad.danglingReferences!=0)throw new AssertionError("Invalid placeholder has no node ids to audit");

        System.out.println("ACIS SAT B-Rep cases passed: encrypted payload, planar solids, cylindrical/conic loft and safe fallback.");
    }


    /** Matches real R2013 implicit SAT ids, I-sense coedges and shuffled edge order. */
    private static void testImplicitInvertedCoedgeLoop(){
        String[] records={
            "asmheader $-1",
            "body $-1 $-1 $-1 $2 $-1 $-1",
            "lump $-1 $-1 $3 $1",
            "shell $-1 $-1 $-1 $-1 $-1 $4 $-1 $2",
            "face $-1 $-1 $-1 $-1 $5 $3 $-1 $6 forward single",
            "loop $-1 $-1 $-1 $-1 $7 $4",
            "plane-surface $-1 0 0 0 0 0 1 1 0 0 forward_v I I I I",
            "coedge $-1 $-1 $-1 $8 $9 $-1 $10 I $5 $0 $-1",
            "coedge $-1 $-1 $-1 $11 $7 $-1 $12 I $5 $0 $-1",
            "coedge $-1 $-1 $-1 $7 $11 $-1 $13 I $5 $0 $-1",
            "edge $-1 $-1 $-1 $14 0 $15 1 $7 $16 forward 7 unknown",
            "coedge $-1 $-1 $-1 $9 $8 $-1 $17 I $5 $0 $-1",
            "edge $-1 $-1 $-1 $18 0 $14 1 $8 $19 forward 7 unknown",
            "edge $-1 $-1 $-1 $15 0 $20 1 $9 $21 forward 7 unknown",
            "vertex $-1 $-1 $-1 $10 $2 $22",
            "vertex $-1 $-1 $-1 $10 $2 $23",
            "straight-curve $-1 0 0 0 1 0 0 I I",
            "edge $-1 $-1 $-1 $20 0 $18 1 $11 $24 forward 7 unknown",
            "vertex $-1 $-1 $-1 $17 $2 $25",
            "straight-curve $-1 0 0 0 0 1 0 I I",
            "vertex $-1 $-1 $-1 $13 $2 $26",
            "straight-curve $-1 1 0 0 0 1 0 I I",
            "point $-1 $-1 $-1 0 0 0",
            "point $-1 $-1 $-1 1 0 0",
            "straight-curve $-1 1 1 0 -1 0 0 I I",
            "point $-1 $-1 $-1 0 1 0",
            "point $-1 $-1 $-1 1 1 0"
        };
        AcisSatMesh.Result complete=AcisSatMesh.parseDxfChunks(List.of(implicitSat(records)));
        if(!complete.complete||complete.faces!=1||complete.tessellatedFaces!=1||
           complete.triangles.length!=6||complete.edges.length!=8)
            throw new AssertionError("Real SAT coedge connectivity must produce exactly two actual planar triangles");
        // A broken next-reference must not be closed with a fabricated line.
        records[8]=records[8].replace("$11 $7","$999 $7");
        AcisSatMesh.Result broken=AcisSatMesh.parseDxfChunks(List.of(implicitSat(records)));
        if(broken.complete||broken.triangles.length!=0)
            throw new AssertionError("Unclosed coedge ring was wrongly filled as a valid ACIS face");
    }

    private static String implicitSat(String[] records){
        StringBuilder out=new StringBuilder("700 0 1 0\nMusaCAD implicit SAT\n1 1e-06 1e-10\n");
        for(String record:records)out.append(record).append(" #\n");
        return out.append("End-of-ACIS-data\n").toString();
    }

    private static void testMissingAcisReferencesStillCountFaces(){
        // Broken SAB->SAT linkage is not proof of a non-existent 3D body.
        // Mesh is absent, but the declared face must stay visible in audits.
        String sat="700 0 1 0\nMusaCAD missing-face fixture\n1 1e-06 1e-10\n"
            +"-0 face $-1 $-1 $-1 $-1 $999 $-1 $-1 $1 forward single #\n"
            +"-1 plane-surface $-1 0 0 0 0 0 1 1 0 0 forward_v I I I I #\n"
            +"End-of-ACIS-data\n";
        AcisSatMesh.Result result=AcisSatMesh.parseDxfChunks(List.of(sat));
        if(result.hasGeometry()||result.complete||result.faces!=1||
           result.unsupportedFaces!=1||result.tessellatedFaces!=0||
           result.danglingReferences<1)
            throw new AssertionError("Missing ACIS geometry must retain its real face inventory");
    }

    private static void testUnsupportedFaceWireframe(){
        String sat="700 0 1 0\nACIS orphan-edge test\n1 1e-06 1e-10\n"
            +"-0 point $-1 0 0 0 #\n"
            +"-1 point $-1 3 0 0 #\n"
            +"-10 vertex $-1 $-1 $0 #\n"
            +"-11 vertex $-1 $-1 $1 #\n"
            +"-20 straight-curve $-1 0 0 0 1 0 0 I I #\n"
            +"-30 edge $-1 $10 0 $11 1 $-1 $20 forward 7 unknown #\n"
            +"-40 spline-surface $-1 #\n"
            +"-50 face $-1 $-1 $-1 $-1 $-1 $40 forward single #\n"
            +"End-of-ACIS-data\n";
        AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(List.of(sat));
        if(!mesh.hasGeometry()||mesh.complete||mesh.faces!=1||mesh.unsupportedFaces!=1)
            throw new AssertionError("Unsupported ACIS face must retain only true source edges");
        if(mesh.triangles.length!=0||mesh.edges.length!=2)
            throw new AssertionError("Wireframe fallback invented triangles or lost a real edge");
    }

    private static void testCylinderSide(){
        String sat="700 0 1 0\nMusaCAD cylinder\n1 1e-06 1e-10\n"
            +"-0 point $-1 1 0 0 #\n"
            +"-1 point $-1 1 0 2 #\n"
            +"-10 vertex $-1 $30 $0 #\n"
            +"-11 vertex $-1 $31 $1 #\n"
            +"-20 ellipse-curve $-1 0 0 0 0 0 1 1 0 0 1 I I #\n"
            +"-21 ellipse-curve $-1 0 0 2 0 0 1 1 0 0 1 I I #\n"
            +"-30 edge $-1 $10 0 $10 6.283185307179586 $40 $20 forward 7 unknown #\n"
            +"-31 edge $-1 $11 0 $11 6.283185307179586 $41 $21 forward 7 unknown #\n"
            +"-40 coedge $-1 $40 $40 $-1 $30 forward $50 $-1 #\n"
            +"-41 coedge $-1 $41 $41 $-1 $31 forward $51 $-1 #\n"
            +"-50 loop $-1 $-1 $40 $60 #\n"
            +"-51 loop $-1 $-1 $41 $60 #\n"
            +"-70 cone-surface $-1 0 0 0 0 0 1 1 0 0 1 I I 0 1 1 forward_v I I I I #\n"
            +"-60 face $-1 $-1 $50 $-1 $-1 $70 forward single #\n"
            +"End-of-ACIS-data\n";
        AcisSatMesh.Result mesh=AcisSatMesh.parseDxfChunks(List.of(sat));
        if(!mesh.hasGeometry()||!mesh.complete||mesh.faces!=1||mesh.tessellatedFaces!=1)
            throw new AssertionError("cylindrical ACIS side must tessellate completely");
        if(mesh.triangles.length<180)throw new AssertionError("cylinder side must be a real curved triangle strip");
        if(mesh.xyz.length<180)throw new AssertionError("ellipse boundaries must be sampled as real 3D rings");
    }

    private static String cubeSat(){
        StringBuilder s=new StringBuilder();
        s.append("700 0 1 0\n");
        s.append("MusaCAD ACIS SAT test\n");
        s.append("1 1e-06 1e-10\n");
        double[][] p={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
        for(int i=0;i<p.length;i++)
            s.append("-").append(i).append(" point $-1 ").append(p[i][0]).append(' ').append(p[i][1]).append(' ').append(p[i][2]).append(" #\n");
        for(int i=0;i<8;i++)s.append("-").append(10+i).append(" vertex $-1 $-1 $").append(i).append(" #\n");

        int[][] faces={{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
        double[][] normals={{0,0,-1},{0,0,1},{0,-1,0},{1,0,0},{0,1,0},{-1,0,0}};
        for(int f=0;f<faces.length;f++){
            int loop=300+f,face=400+f,surface=500+f;
            for(int e=0;e<4;e++){
                int a=faces[f][e],b=faces[f][(e+1)%4];
                int edge=100+f*4+e,curve=150+f*4+e,co=200+f*4+e;
                double dx=p[b][0]-p[a][0],dy=p[b][1]-p[a][1],dz=p[b][2]-p[a][2];
                s.append("-").append(curve).append(" straight-curve $-1 ")
                    .append(p[a][0]).append(' ').append(p[a][1]).append(' ').append(p[a][2]).append(' ')
                    .append(dx).append(' ').append(dy).append(' ').append(dz).append(" I I #\n");
                s.append("-").append(edge).append(" edge $-1 $").append(10+a).append(" 0 $").append(10+b)
                    .append(" 1 $").append(co).append(" $").append(curve).append(" forward 7 unknown #\n");
            }
            for(int e=0;e<4;e++){
                int co=200+f*4+e,next=200+f*4+(e+1)%4,prev=200+f*4+(e+3)%4,edge=100+f*4+e;
                s.append("-").append(co).append(" coedge $-1 $").append(next).append(" $").append(prev)
                    .append(" $-1 $").append(edge).append(" forward $").append(loop).append(" $-1 #\n");
            }
            s.append("-").append(loop).append(" loop $-1 $-1 $").append(200+f*4).append(" $").append(face).append(" #\n");
            s.append("-").append(surface).append(" plane-surface $-1 0 0 0 ")
                .append(normals[f][0]).append(' ').append(normals[f][1]).append(' ').append(normals[f][2])
                .append(" 1 0 0 forward_v I I I I #\n");
            s.append("-").append(face).append(" face $-1 $-1 $").append(loop).append(" $-1 $-1 $")
                .append(surface).append(" forward single #\n");
        }
        s.append("End-of-ACIS-data\n");
        return s.toString();
    }

    private static String crypt(String value){
        String compact=value.replace("\n"," ");
        StringBuilder out=new StringBuilder(compact.length());
        for(int i=0;i<compact.length();i++){
            char c=compact.charAt(i);
            out.append(c<=32||c>=159?c:(char)(159-c));
        }
        return out.toString();
    }
}
