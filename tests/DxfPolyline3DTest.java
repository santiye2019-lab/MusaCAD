import com.musa.cad.DxfPolyline3D;
import java.util.ArrayList;
import java.util.List;

/** CAD geometry-fidelity gate for DXF POLYLINE modes (host JVM; no APK). */
public final class DxfPolyline3DTest {
    private static void yes(boolean ok,String reason){if(!ok)throw new AssertionError(reason);}
    private static void group(List<String> tags,int code,String value){
        tags.add(Integer.toString(code));tags.add(value);
    }
    private static DxfPolyline3D.Vertex vertex(double x,double y,double z,int flags,int... indices){
        List<String> record=new ArrayList<>();
        group(record,10,Double.toString(x));group(record,20,Double.toString(y));
        group(record,30,Double.toString(z));group(record,70,Integer.toString(flags));
        for(int i=0;i<Math.min(4,indices.length);i++)
            group(record,71+i,Integer.toString(indices[i]));
        DxfPolyline3D.Vertex result=DxfPolyline3D.readVertex(record,0,record.size());
        yes(result!=null,"vertex could not be parsed");
        return result;
    }
    private static void same(DxfPolyline3D.Segment edge,double x1,double y1,double x2,double y2){
        yes(edge.start.x==x1&&edge.start.y==y1&&edge.end.x==x2&&edge.end.y==y2,
            "edge ordering or geometry was corrupted");
    }
    public static void main(String[]args){
        DxfPolyline3D.Vertex a=vertex(0,0,11,32);
        DxfPolyline3D.Vertex b=vertex(10,0,12,32);
        DxfPolyline3D.Vertex c=vertex(10,10,13,32);
        List<DxfPolyline3D.Vertex> line=List.of(a,b,c);
        List<DxfPolyline3D.Segment> segments=DxfPolyline3D.segments(8,0,0,line);
        yes(segments.size()==2,"3D open polyline must have 2 edges");
        yes(segments.get(0).start.z==11&&segments.get(0).end.z==12,
            "Z coordinates were lost");
        yes(DxfPolyline3D.segments(8|1,0,0,line).size()==3,
            "3D closed polyline must include final edge");
        yes(DxfPolyline3D.segments(0,0,0,line).isEmpty(),
            "2D polylines must remain handled by 2D bulge engine");

        List<DxfPolyline3D.Vertex> face=List.of(
            vertex(0,0,1,192),vertex(10,0,2,192),
            vertex(10,10,3,192),vertex(0,10,4,192),
            vertex(0,0,0,128,1,-2,3,4));
        segments=DxfPolyline3D.segments(64,0,0,face);
        yes(segments.size()==3,"Polyface with one invisible edge must expose 3 edges");
        same(segments.get(0),0,0,10,0);
        same(segments.get(1),10,10,0,10);
        same(segments.get(2),0,10,0,0);

        List<DxfPolyline3D.Vertex> triangle=List.of(
            vertex(0,0,2,192),vertex(10,0,2,192),vertex(0,5,3,192),
            vertex(0,0,0,128,1,2,3));
        yes(DxfPolyline3D.segments(64,0,0,triangle).size()==3,
            "Triangular polyface is lost");
        List<DxfPolyline3D.Vertex> invalid=List.of(
            vertex(0,0,0,192),vertex(10,0,0,192),vertex(0,5,0,192),
            vertex(0,0,0,128,1,2,999));
        yes(DxfPolyline3D.segments(64,0,0,invalid).isEmpty(),
            "Invalid face indices must not connect unrelated vertices");

        List<DxfPolyline3D.Vertex> mesh=List.of(
            vertex(0,0,0,64),vertex(1,0,0,64),vertex(2,0,0,64),
            vertex(0,1,0,64),vertex(1,1,0,64),vertex(2,1,0,64));
        segments=DxfPolyline3D.segments(16,2,3,mesh);
        yes(segments.size()==7,"2x3 polygon mesh must have 7 edges");
        yes(DxfPolyline3D.segments(16,2,4,mesh).isEmpty(),
            "Truncated mesh must not return plausible edges");

        List<String> malformed=new ArrayList<>();
        group(malformed,10,"NaN");group(malformed,20,"4");
        yes(DxfPolyline3D.readVertex(malformed,0,malformed.size())==null,
            "Non-finite 3D coordinate was accepted");
        List<String> faceWithoutPosition=new ArrayList<>();
        group(faceWithoutPosition,70,"128");
        group(faceWithoutPosition,71,"1");group(faceWithoutPosition,72,"2");
        group(faceWithoutPosition,73,"3");
        yes(DxfPolyline3D.readVertex(faceWithoutPosition,0,faceWithoutPosition.size())!=null,
            "Polyface index-only record should not require XYZ");

        System.out.println("PASS: 3D POLYLINE, polyface, mesh, hidden edges and invalid-data regression");
    }
}
