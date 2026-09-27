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

        AcisSatMesh.Result bad=AcisSatMesh.parseDxfChunks(List.of("ACIS_PLACEHOLDER"));
        if(bad.hasGeometry())throw new AssertionError("invalid ACIS payload must not invent geometry");

        System.out.println("ACIS SAT B-Rep cases passed: encrypted payload, planar 3DSOLID tessellation and safe fallback.");
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
