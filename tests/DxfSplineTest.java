import com.musa.cad.DxfSpline;
import java.util.*;

public class DxfSplineTest {
    private static void near(double a,double b,String n){if(Math.abs(a-b)>.015)throw new AssertionError(n+": "+a+" != "+b);}
    private static List<String> tags(Object...v){ArrayList<String>a=new ArrayList<>();for(Object x:v)a.add(String.valueOf(x));return a;}
    public static void main(String[]args)throws Exception{
        List<DxfSpline.Point> c=Arrays.asList(new DxfSpline.Point(1,0),new DxfSpline.Point(1,1),new DxfSpline.Point(0,1));
        List<Double>w=Arrays.asList(1d,Math.sqrt(.5),1d),k=Arrays.asList(0d,0d,0d,1d,1d,1d);
        DxfSpline.Point mid=DxfSpline.evaluate(c,w,k,2,.5);near(mid.x,Math.sqrt(.5),"quarter x");near(mid.y,Math.sqrt(.5),"quarter y");

        List<String> rational=tags(70,4,71,2,72,6,73,3,74,0,40,0,40,0,40,0,40,1,40,1,40,1,41,1,41,Math.sqrt(.5),41,1,10,1,20,0,10,1,20,1,10,0,20,1);
        DxfSpline.Result r=DxfSpline.parse(rational,0,rational.size());if(!r.exactNurbs||r.points.size()<90)throw new AssertionError("rational NURBS not sampled");DxfSpline.Point rm=r.points.get(r.points.size()/2);near(rm.x,Math.sqrt(.5),"parsed quarter x");near(rm.y,Math.sqrt(.5),"parsed quarter y");

        List<String> fit=tags(70,0,71,3,74,4,11,0,21,0,11,5,21,5,11,10,21,0,11,15,21,5);DxfSpline.Result f=DxfSpline.parse(fit,0,fit.size());if(f.exactNurbs||f.points.size()<50)throw new AssertionError("fit fallback");near(f.points.get(0).x,0,"fit start x");near(f.points.get(f.points.size()-1).x,15,"fit end x");

        List<String> linear=tags(70,16,71,1,10,0,20,0,10,10,20,5,10,20,20,0);DxfSpline.Result l=DxfSpline.parse(linear,0,linear.size());if(l.points.size()!=3)throw new AssertionError("linear spline should keep controls");near(l.points.get(1).x,10,"linear middle");
        System.out.println("DXF NURBS spline cases passed");
    }
}
