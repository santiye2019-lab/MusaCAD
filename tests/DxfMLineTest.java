import com.musa.cad.DxfMLine;
import java.util.*;

public class DxfMLineTest {
    private static List<String> tags(Object... values){ArrayList<String> out=new ArrayList<>();for(Object v:values)out.add(String.valueOf(v));return out;}
    private static void near(double actual,double expected,String name){if(Math.abs(actual-expected)>1e-9)throw new AssertionError(name+" "+actual+" != "+expected);}
    private static void seg(DxfMLine.Segment s,double x1,double y1,double x2,double y2,String name){near(s.x1,x1,name+" x1");near(s.y1,y1,name+" y1");near(s.x2,x2,name+" x2");near(s.y2,y2,name+" y2");}
    private static List<String> twoVertex(Object... firstElementExtra){
        ArrayList<Object> v=new ArrayList<>(Arrays.asList(
            71,1,72,2,73,2,
            11,0,21,0,12,1,22,0,13,0,23,1,
            74,2,41,1,41,0,75,0,
            74,2,41,-1,41,0,75,0,
            11,10,21,0,12,1,22,0,13,0,23,1,
            74,2,41,1,41,0,75,0,
            74,2,41,-1,41,0,75,0
        ));
        if(firstElementExtra.length>0){
            int at=0;for(int p=0;p<v.size()-1;p+=2)if(v.get(p).equals(74)){at=p;break;}
            v.set(at+1,2+firstElementExtra.length/2);
            int insert=at+6;for(Object o:firstElementExtra)v.add(insert++,o);
        }
        return tags(v.toArray());
    }
    public static void main(String[] args){
        DxfMLine.Result r=DxfMLine.parse(twoVertex(),0,twoVertex().size());
        if(r.segments.size()!=2||r.vertexCount!=2||r.elementCount!=2)throw new AssertionError("basic mline count");
        seg(r.segments.get(0),0,1,10,1,"top");seg(r.segments.get(1),0,-1,10,-1,"bottom");

        List<String> cut=twoVertex(41,3,41,5);
        DxfMLine.Result c=DxfMLine.parse(cut,0,cut.size());
        if(c.segments.size()!=3)throw new AssertionError("cut segment count "+c.segments.size());
        seg(c.segments.get(0),0,1,3,1,"cut before");seg(c.segments.get(1),5,1,10,1,"cut after");

        List<String> rotated=tags(71,1,72,2,73,1,
            11,0,21,0,12,0,22,1,13,-1,23,0,74,2,41,2,41,0,75,0,
            11,0,21,10,12,0,22,1,13,-1,23,0,74,2,41,2,41,0,75,0);
        DxfMLine.Result rr=DxfMLine.parse(rotated,0,rotated.size());
        if(rr.segments.size()!=1)throw new AssertionError("rotated count");
        seg(rr.segments.get(0),-2,0,-2,10,"rotated");

        if(!DxfMLine.parse(tags(71,1,72,1,73,2),0,6).segments.isEmpty())throw new AssertionError("single vertex must be empty");
        System.out.println("DXF MLINE element geometry cases passed");
    }
}
