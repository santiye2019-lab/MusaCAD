import com.musa.cad.DxfBulge;
import java.util.*;

public class DxfBulgeTest {
    private static void near(double actual,double expected,String name){if(Math.abs(actual-expected)>.06)throw new AssertionError(name+": "+actual+" != "+expected);}
    private static DxfBulge.Point nearestMid(List<DxfBulge.Point> points,double x){DxfBulge.Point best=null;double d=Double.POSITIVE_INFINITY;for(DxfBulge.Point p:points){double n=Math.abs(p.x-x);if(n<d){d=n;best=p;}}return best;}
    public static void main(String[]args){
        List<DxfBulge.Point> straight=DxfBulge.expand(Arrays.asList(new DxfBulge.Vertex(0,0,0),new DxfBulge.Vertex(10,0,0)),false);
        if(straight.size()!=2)throw new AssertionError("straight point count");near(straight.get(1).x,10,"straight end x");

        List<DxfBulge.Point> semi=DxfBulge.expand(Arrays.asList(new DxfBulge.Vertex(0,0,1),new DxfBulge.Vertex(10,0,0)),false);
        if(semi.size()<32)throw new AssertionError("semicircle tessellation");DxfBulge.Point mid=nearestMid(semi,5);near(mid.x,5,"semi mid x");near(mid.y,-5,"positive bulge midpoint");near(semi.get(semi.size()-1).x,10,"semi end x");near(semi.get(semi.size()-1).y,0,"semi end y");

        List<DxfBulge.Point> negative=DxfBulge.expand(Arrays.asList(new DxfBulge.Vertex(0,0,-1),new DxfBulge.Vertex(10,0,0)),false);
        near(nearestMid(negative,5).y,5,"negative bulge midpoint");

        double quarter=Math.tan(Math.PI/8d);List<DxfBulge.Point> q=DxfBulge.expand(Arrays.asList(new DxfBulge.Vertex(0,0,quarter),new DxfBulge.Vertex(10,0,0)),false);
        if(q.size()<16)throw new AssertionError("quarter arc tessellation");near(q.get(0).x,0,"quarter start");near(q.get(q.size()-1).x,10,"quarter end");

        List<DxfBulge.Point> closed=DxfBulge.expand(Arrays.asList(new DxfBulge.Vertex(0,0,0),new DxfBulge.Vertex(10,0,0),new DxfBulge.Vertex(10,10,1)),true);
        if(closed.size()<35)throw new AssertionError("closed final bulge missing");DxfBulge.Point end=closed.get(closed.size()-1);near(end.x,0,"closed end x");near(end.y,0,"closed end y");
        System.out.println("DXF polyline bulge cases passed");
    }
}
