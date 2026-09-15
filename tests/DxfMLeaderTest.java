import com.musa.cad.DxfMLeader;
import java.util.*;

public class DxfMLeaderTest {
    private static List<String> tags(Object... v){ArrayList<String> r=new ArrayList<>();for(Object o:v)r.add(String.valueOf(o));return r;}
    private static void close(double a,double b){if(Math.abs(a-b)>1e-9)throw new AssertionError(a+" != "+b);}
    public static void main(String[] args){
        DxfMLeader.Data d=DxfMLeader.parse(tags(
            300,"CONTEXT_DATA{",10,100,20,200,41,8,304,"{\\C256;K-01\\PDUVAR}",
            302,"LEADER{",10,120,20,200,40,7,
            304,"LEADER_LINE{",10,140,20,180,10,145,20,160,305,"}",303,"}",301,"}"));
        if(!d.text.contains("K-01"))throw new AssertionError("mleader text");close(d.textX,100);close(d.textY,200);close(d.textHeight,8);
        if(d.leaders.size()!=1)throw new AssertionError("single leader");double[] p=d.leaders.get(0).points;
        if(p.length!=6)throw new AssertionError("landing plus vertices");close(p[0],120);close(p[1],200);close(p[4],145);close(p[5],160);close(d.leaders.get(0).arrowSize,7);

        d=DxfMLeader.parse(tags(300,"CONTEXT_DATA{",304,"NOTE",10,1,20,2,
            302,"LEADER{",10,3,20,4,304,"LEADER_LINE{",10,5,20,6,305,"}",304,"LEADER_LINE{",10,7,20,8,305,"}",303,"}",301,"}"));
        if(d.leaders.size()!=2)throw new AssertionError("separate leader lines");
        if(!"NOTE".equals(d.text))throw new AssertionError("structural 304 must not replace content");

        d=DxfMLeader.parse(tags(300,"CONTEXT_DATA{",304,"TEXT",10,1,20,2,302,"LEADER{",10,5,20,5,304,"LEADER_LINE{",10,5,20,5,10,9,20,9,305,"}",303,"}",301,"}"));
        if(d.leaders.size()!=1||d.leaders.get(0).points.length!=4)throw new AssertionError("duplicate landing cleanup");

        d=DxfMLeader.parse(tags(300,"CONTEXT_DATA{",304,"TEXT",10,"bad",20,2,302,"LEADER{",304,"LEADER_LINE{",10,4,20,"bad",305,"}",303,"}",301,"}"));
        if(!Double.isNaN(d.textX)||!d.leaders.isEmpty())throw new AssertionError("malformed coordinates ignored");
        System.out.println("DXF MULTILEADER context cases passed");
    }
}
