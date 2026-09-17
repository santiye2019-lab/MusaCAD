import com.musa.cad.DxfHatch;
import java.util.*;

public class DxfHatchTest {
    private static List<String> tags(Object... values){ArrayList<String>a=new ArrayList<>();for(Object value:values)a.add(String.valueOf(value));return a;}
    public static void main(String[] args)throws Exception{
        List<String> solid=tags(2,"SOLID",70,1,91,1,92,2,72,0,73,1,93,4,10,0,20,0,10,10,20,0,10,10,20,5,10,0,20,5,97,0,75,0,98,0);
        DxfHatch.Result s=DxfHatch.parse(solid,0,solid.size());
        if(!s.solid||s.loops.size()!=1||s.loops.get(0).points.size()!=4||!s.loops.get(0).closed)throw new AssertionError("solid polyline");

        List<String> patterned=tags(2,"ANSI31",70,0,91,1,92,2,72,0,73,1,93,3,10,0,20,0,10,10,20,0,10,5,20,5,97,0,75,0,76,1,52,45,41,2,78,1,53,45,43,0,44,0,45,0,46,1,79,2,49,.5,49,-.5,98,0);
        DxfHatch.Result p=DxfHatch.parse(patterned,0,patterned.size());
        if(p.solid||p.patternLines.size()!=1||p.patternLines.get(0).dashes.length!=2||Math.abs(p.patternScale-2)>1e-9)throw new AssertionError("pattern");

        List<String> edge=tags(70,1,91,1,92,0,93,2,72,1,10,0,20,0,11,10,21,0,72,2,10,10,20,5,40,5,50,-90,51,90,73,1,97,0,75,0,98,0);
        DxfHatch.Result e=DxfHatch.parse(edge,0,edge.size());
        if(e.loops.size()!=1||e.loops.get(0).points.size()<5)throw new AssertionError("edge loop");

        List<String> bulge=tags(70,1,91,1,92,2,72,1,73,1,93,2,10,0,20,0,42,1,10,10,20,0,42,0,97,0,75,0,98,0);
        DxfHatch.Result b=DxfHatch.parse(bulge,0,bulge.size());
        if(b.loops.get(0).points.size()<=2)throw new AssertionError("bulge expand");

        System.out.println("DXF hatch boundary/pattern cases passed");
    }
}
