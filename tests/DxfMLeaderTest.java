import com.musa.cad.DxfMLeader;
import java.util.*;

public final class DxfMLeaderTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}
    private static List<String> pairs(String...v){return Arrays.asList(v);}
    public static void main(String[] args){
        List<String> data=pairs(
            "300","CONTEXT_DATA{","40","1.0","10","16.6905","20","14.5431","41","0.18","140","0.18",
            "304","LEADER","12","17.7603","22","14.6331","42","0.0",
            "302","LEADER{","290","1","10","18.2103","20","14.5431","304","LEADER_LINE{",
            "10","20.3581","20","13.7380","91","0","305","}","271","0","303","}","301","}"
        );
        DxfMLeader.Result r=DxfMLeader.parse(data,0,data.size());
        require("LEADER".equals(r.text),"text");
        require(Math.abs(r.textX-17.7603)<1e-6&&Math.abs(r.textY-14.6331)<1e-6,"text point");
        require(Math.abs(r.textHeight-.18)<1e-9,"text height");
        require(r.leaderLines.size()==1,"one leader line");
        List<DxfMLeader.Point> line=r.leaderLines.get(0);
        require(line.size()==2,"node + endpoint");
        require(Math.abs(line.get(0).x-18.2103)<1e-6&&Math.abs(line.get(1).x-20.3581)<1e-6,"leader geometry");

        List<String> multi=pairs(
            "304","NOTE","12","3","22","4","41","2","302","LEADER{","10","1","20","1","304","LEADER_LINE{",
            "10","2","20","2","10","3","20","2","305","}","303","}"
        );
        DxfMLeader.Result m=DxfMLeader.parse(multi,0,multi.size());
        require(m.leaderLines.get(0).size()==3,"multi point leader");
        System.out.println("DxfMLeaderTest OK");
    }
}
