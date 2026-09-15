import com.musa.cad.DxfPolyMesh;
import java.util.*;

public class DxfPolyMeshTest {
    private static void count(double[] segments,int expected,String name){
        if(segments.length%4!=0||segments.length/4!=expected)throw new AssertionError(name+": "+segments.length/4);
    }
    private static boolean edge(double[] s,double x1,double y1,double x2,double y2){
        for(int i=0;i+3<s.length;i+=4){
            boolean forward=Math.abs(s[i]-x1)<1e-9&&Math.abs(s[i+1]-y1)<1e-9&&Math.abs(s[i+2]-x2)<1e-9&&Math.abs(s[i+3]-y2)<1e-9;
            boolean reverse=Math.abs(s[i]-x2)<1e-9&&Math.abs(s[i+1]-y2)<1e-9&&Math.abs(s[i+2]-x1)<1e-9&&Math.abs(s[i+3]-y1)<1e-9;
            if(forward||reverse)return true;
        }
        return false;
    }
    public static void main(String[] args){
        double[] square={0,0,1,0,0,1,1,1};
        double[] mesh=DxfPolyMesh.polygonMesh(square,2,2,false,false);count(mesh,4,"2x2 open mesh");
        if(!edge(mesh,0,0,1,0)||!edge(mesh,0,0,0,1)||!edge(mesh,1,0,1,1)||!edge(mesh,0,1,1,1))throw new AssertionError("mesh topology");

        double[] grid={0,0,1,0, 0,1,1,1, 0,2,1,2};
        count(DxfPolyMesh.polygonMesh(grid,3,2,false,false),7,"3x2 open mesh");
        count(DxfPolyMesh.polygonMesh(grid,3,2,true,false),9,"closed M mesh");
        count(DxfPolyMesh.polygonMesh(new double[]{0,0},2,2,false,false),0,"short mesh input");

        List<int[]> face=new ArrayList<>();face.add(new int[]{1,2,4,3});
        double[] pf=DxfPolyMesh.polyface(square,face);count(pf,4,"polyface square");
        if(!edge(pf,0,0,1,0)||!edge(pf,1,0,1,1)||!edge(pf,1,1,0,1)||!edge(pf,0,1,0,0))throw new AssertionError("polyface topology");

        List<int[]> hidden=new ArrayList<>();hidden.add(new int[]{1,-2,4,3});
        double[] hiddenEdge=DxfPolyMesh.polyface(square,hidden);count(hiddenEdge,3,"hidden polyface edge");
        if(edge(hiddenEdge,1,0,1,1))throw new AssertionError("negative index edge must be hidden");

        List<int[]> duplicate=new ArrayList<>();duplicate.add(new int[]{1,2,4});duplicate.add(new int[]{1,4,3});
        count(DxfPolyMesh.polyface(square,duplicate),5,"shared edge dedupe");
        List<int[]> invalid=new ArrayList<>();invalid.add(new int[]{1,99,0,0});count(DxfPolyMesh.polyface(square,invalid),0,"invalid face indices");
        System.out.println("7 polygon mesh and polyface cases passed");
    }
}
