import com.musa.cad.DxfBlocks;
import com.musa.cad.DxfTransparency;
import java.util.*;

public class DxfBlocksTest {
    private static String tags(Object... values){StringBuilder s=new StringBuilder();for(Object v:values)s.append(v).append('\n');return s.toString();}
    private static final String LINE=tags(0,"LINE",10,3,20,4,11,5,21,4);
    private static String block(String name,String body){return tags(0,"BLOCK",2,name,10,1,20,2)+body+tags(0,"ENDBLK");}
    private static String insert(String name,Object... extra){return tags(0,"INSERT",2,name)+tags(extra);}
    private static String dimension(String name,Object... extra){return tags(0,"DIMENSION",2,name)+tags(extra);}
    private static DxfBlocks.Result read(String blocks,String roots)throws Exception {
        String text=tags(0,"SECTION",2,"BLOCKS")+blocks+tags(0,"ENDSEC",0,"SECTION",2,"ENTITIES")+roots+tags(0,"ENDSEC",0,"EOF");
        return DxfBlocks.expand(Arrays.asList(text.split("\n")));
    }
    private static void placementPoint(DxfBlocks.Result r,int index,double x,double y){
        double[] actual=r.placements.get(index).transform.point(3,4);
        if(Math.abs(actual[0]-x)>1e-6||Math.abs(actual[1]-y)>1e-6)throw new AssertionError(Arrays.toString(actual));
    }
    private static void point(DxfBlocks.Result r,double x,double y){
        if(r.placements.size()!=1||r.skipped!=0)throw new AssertionError("Unexpected expansion count: "+r.placements.size()+"/"+r.skipped);
        placementPoint(r,0,x,y);
    }
    private static void skipped(DxfBlocks.Result r){if(r.skipped!=1||!r.placements.isEmpty())throw new AssertionError("Expected skipped reference");}
    public static void main(String[] args)throws Exception {
        String b=block("A",LINE);
        point(read(b,insert("A",10,10,20,20)),12,22);
        point(read(b,insert("a",10,10,20,20,50,90,41,2,42,3)),4,24);
        point(read(b,insert("A",41,-1)), -2,2);

        String nested=b+block("B",insert("A",10,5,20,6));
        point(read(nested,insert("B",10,10,20,20)),16,26);
        DxfBlocks.Result layer=read(b,insert("A",8,"BORU"));
        if(!layer.placements.get(0).layer.equals("BORU"))throw new AssertionError("Layer inheritance");
        String transparentBlock=block("T",tags(0,"LINE",440,16777216,10,3,20,4,11,5,21,4));
        DxfBlocks.Result transparency=read(transparentBlock,insert("T",440,33554560));
        if(DxfTransparency.opacity(transparency.placements.get(0).transparency,Collections.emptyMap())!=128)throw new AssertionError("Transparency inheritance");
        skipped(read(b,insert("MISSING")));
        skipped(read(block("A",insert("A")),insert("A")));

        DxfBlocks.Result columns=read(b,insert("A",10,10,20,20,70,2,44,5));
        if(columns.placements.size()!=2||columns.skipped!=0)throw new AssertionError("Column array");
        placementPoint(columns,0,12,22);placementPoint(columns,1,17,22);
        DxfBlocks.Result rows=read(b,insert("A",10,10,20,20,71,2,45,7));
        if(rows.placements.size()!=2||rows.skipped!=0)throw new AssertionError("Row array");
        placementPoint(rows,0,12,22);placementPoint(rows,1,12,29);
        DxfBlocks.Result grid=read(b,insert("A",10,10,20,20,70,2,71,2,44,5,45,7));
        if(grid.placements.size()!=4||grid.skipped!=0)throw new AssertionError("2x2 array");

        // Non-default OCS normals are no longer discarded.
        point(read(b,insert("A",10,10,20,20,210,0,220,0,230,-1)),-12,22);
        DxfBlocks.Result oblique=read(b,insert("A",10,10,20,20,210,0,220,1,230,1));
        if(oblique.placements.size()!=1||oblique.skipped!=0)throw new AssertionError("Oblique extrusion");
        double root=Math.sqrt(.5);placementPoint(oblique,0,-12,-22*root);

        DxfBlocks.Result repeated=read(b,insert("A")+insert("A",10,100));
        if(repeated.placements.size()!=2||repeated.skipped!=0)throw new AssertionError("Repeated block");
        skipped(read(b,insert("A",70,1001)));

        String dimBlock=block("*D1",LINE);
        DxfBlocks.Result dim=read(dimBlock,dimension("*D1",8,"DIM",10,10000,20,10000,13,-5000,23,9000,14,7000,24,-9000));
        point(dim,3,4);
        if(!dim.placements.get(0).layer.equals("DIM"))throw new AssertionError("Dimension layer inheritance");

        String nestedDim=dimBlock+block("C",dimension("*D1",8,"0"));
        point(read(nestedDim,insert("C",10,10,20,20)),12,22);
        skipped(read("",dimension("*MISSING")));
        System.out.println("17 block expansion cases passed");
    }
}
