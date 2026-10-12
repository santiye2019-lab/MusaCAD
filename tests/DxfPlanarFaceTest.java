import com.musa.cad.DxfPlanarFace;
import java.util.ArrayList;
import java.util.List;

public final class DxfPlanarFaceTest {
    static void expect(boolean value, String message) {
        if(!value)throw new AssertionError(message);
    }
    static List<String> tags(double[][] xy, int flags){
        ArrayList<String> values=new ArrayList<>();
        for(int i=0;i<xy.length;i++){
            values.add(Integer.toString(10+i));values.add(Double.toString(xy[i][0]));
            values.add(Integer.toString(20+i));values.add(Double.toString(xy[i][1]));
        }
        values.add("70");values.add(Integer.toString(flags));
        return values;
    }
    static void is(DxfPlanarFace.Point p,double x,double y){
        expect(p.x==x&&p.y==y,"Wrong vertex order/coordinate");
    }
    public static void main(String[] args){
        double[][] corners={{0,0},{10,0},{0,10},{10,10}};
        for(String type:new String[]{"SOLID","TRACE"}){
            List<String> t=tags(corners,0);
            DxfPlanarFace.Geometry g=DxfPlanarFace.parse(type,t,0,t.size());
            expect(g!=null&&g.outline.size()==4,"Quad dropped");
            is(g.outline.get(0),0,0);
            is(g.outline.get(1),10,0);
            is(g.outline.get(2),10,10); // DXF corner 4
            is(g.outline.get(3),0,10); // DXF corner 3
            expect(g.visibleEdges.size()==4,"Quad perimeter must have four edges");
            for(DxfPlanarFace.Edge e:g.visibleEdges)
                expect(e.a.x==e.b.x||e.a.y==e.b.y,
                    "Incorrect winding introduced a crossing diagonal");
        }
        List<String> t=tags(new double[][]{{1,1},{5,1},{1,5},{1,5}},0);
        DxfPlanarFace.Geometry tri=DxfPlanarFace.parse("SOLID",t,0,t.size());
        expect(tri!=null&&tri.outline.size()==3&&tri.visibleEdges.size()==3,
            "Triangle fourth vertex not collapsed");
        // Only 3 vertex groups (common in some exported DXF) must be accepted.
        t=tags(new double[][]{{1,1},{5,1},{1,5}},0);
        expect(DxfPlanarFace.parse("TRACE",t,0,t.size()).outline.size()==3,
            "3-vertex TRACE rejected");
        t=tags(corners,2);
        DxfPlanarFace.Geometry face=DxfPlanarFace.parse("3DFACE",t,0,t.size());
        expect(face!=null&&face.visibleEdges.size()==3,
            "3DFACE hidden edge 2-3 still drawn");
        for(DxfPlanarFace.Edge e:face.visibleEdges)
            expect(!(e.a.x==10&&e.a.y==0&&e.b.x==0&&e.b.y==10),
                "Hidden diagonal visible");
        t=tags(corners,15);
        expect(DxfPlanarFace.parse("3DFACE",t,0,t.size()).visibleEdges.isEmpty(),
            "All four hidden edges should remain hidden");
        t=tags(new double[][]{{0,0},{10,0}},0);
        expect(DxfPlanarFace.parse("SOLID",t,0,t.size())==null,
            "Truncated face must not silently generate origin points");
        t=tags(corners,0);
        t.set(1,"NaN");
        expect(DxfPlanarFace.parse("3DFACE",t,0,t.size())==null,
            "Non-finite CAD coordinates must be rejected");
        t=tags(corners,0);
        t.remove(7); // Missing Y group for 4th vertex: reject malformed pair stream
        expect(DxfPlanarFace.parse("SOLID",t,0,t.size())==null,
            "Malformed group pairs must not create plausible coordinates");
        System.out.println("PASS: SOLID/TRACE winding, 3DFACE hidden edges, triangle and malformed-data safety");
    }
}
