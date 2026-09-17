import com.musa.cad.DxfBlocks;
import com.musa.cad.DxfColor;
import java.util.*;

public class DxfBlocksTest {
    private static String tags(Object... values){StringBuilder s=new StringBuilder();for(Object v:values)s.append(v).append('\n');return s.toString();}
    private static final String LINE=tags(0,"LINE",10,3,20,4,11,5,21,4);
    private static String line(Object... extra){return tags(0,"LINE",10,3,20,4,11,5,21,4)+tags(extra);}
    private static String block(String name,String body){return tags(0,"BLOCK",2,name,10,1,20,2)+body+tags(0,"ENDBLK");}
    private static String insert(String name,Object... extra){return tags(0,"INSERT",2,name)+tags(extra);}
    private static String layer(String name,Object... extra){return tags(0,"LAYER",2,name)+tags(extra);}
    private static DxfBlocks.Result read(String blocks,String roots)throws Exception {return read("",blocks,roots);}
    private static DxfBlocks.Result read(String layers,String blocks,String roots)throws Exception {
        String text=tags(0,"SECTION",2,"TABLES",0,"TABLE",2,"LAYER")+layers+tags(0,"ENDTAB",0,"ENDSEC")+
            tags(0,"SECTION",2,"BLOCKS")+blocks+tags(0,"ENDSEC",0,"SECTION",2,"ENTITIES")+roots+tags(0,"ENDSEC",0,"EOF");
        return DxfBlocks.expand(Arrays.asList(text.split("\n")));
    }
    private static void point(DxfBlocks.Result r,double x,double y){
        if(r.placements.size()!=1||r.skipped!=0)throw new AssertionError("Unexpected expansion count");
        double[] actual=r.placements.get(0).transform.point(3,4);
        if(Math.abs(actual[0]-x)>1e-6||Math.abs(actual[1]-y)>1e-6)throw new AssertionError(Arrays.toString(actual));
    }
    private static void color(DxfBlocks.Result r,int expected){
        if(r.placements.size()!=1||r.placements.get(0).color!=expected)throw new AssertionError("Color expected "+Integer.toHexString(expected)+" got "+(r.placements.isEmpty()?"none":Integer.toHexString(r.placements.get(0).color)));
    }
    private static void skipped(DxfBlocks.Result r){if(r.skipped!=1||!r.placements.isEmpty())throw new AssertionError("Expected skipped reference");}
    public static void main(String[] args)throws Exception {
        String b=block("A",LINE);
        point(read(b,insert("A",10,10,20,20)),12,22);
        point(read(b,insert("a",10,10,20,20,50,90,41,2,42,3)),4,24);
        point(read(b,insert("A",41,-1)), -2,2);
        String nested=b+block("B",insert("A",10,5,20,6));
        point(read(nested,insert("B",10,10,20,20)),16,26);
        DxfBlocks.Result inheritedLayer=read(b,insert("A",8,"BORU"));
        if(!inheritedLayer.placements.get(0).layer.equals("BORU"))throw new AssertionError("Layer inheritance");
        skipped(read(b,insert("MISSING")));
        skipped(read(block("A",insert("A")),insert("A")));
        skipped(read(b,insert("A",70,2)));
        skipped(read(b,insert("A",230,-1)));
        DxfBlocks.Result repeated=read(b,insert("A")+insert("A",10,100));
        if(repeated.placements.size()!=2||repeated.skipped!=0)throw new AssertionError("Repeated block");

        String layers=layer("BORU",62,3)+layer("TRUE",62,2,420,0x123456);
        DxfBlocks.Result direct=read(layers,"",line(8,"BORU"));
        color(direct,DxfColor.aciArgb(3));
        if(!direct.placements.get(0).directRoot)throw new AssertionError("Direct ENTITIES record must be editable root");
        color(read(layers,"",line(8,"BORU",62,1)),DxfColor.aciArgb(1));
        color(read(layers,"",line(8,"TRUE")),DxfColor.trueColorArgb(0x123456));
        color(read(layers,"",line(8,"BORU",420,0xABCDEF)),DxfColor.trueColorArgb(0xABCDEF));
        DxfBlocks.Result insideBlock=read(layers,block("C",line(62,0)),insert("C",62,5));
        color(insideBlock,DxfColor.aciArgb(5));
        if(insideBlock.placements.get(0).directRoot)throw new AssertionError("Block-expanded member must not be edited as a root source record");
        color(read(layers,block("C",line(62,0)),insert("C",8,"BORU",62,256)),DxfColor.aciArgb(3));
        String nestedColor=block("INNER",line(62,0))+block("OUTER",insert("INNER",62,0));
        color(read(layers,nestedColor,insert("OUTER",62,6)),DxfColor.aciArgb(6));
        System.out.println("19 block/color/source-origin expansion cases passed");
    }
}
