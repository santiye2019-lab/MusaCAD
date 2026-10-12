import com.musa.cad.CadSkippedSummary;
import com.musa.cad.DxfBlocks;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public final class DxfSkippedInventoryTest {
    private static void check(boolean ok,String what){if(!ok)throw new AssertionError(what);}
    private static String code(Object... values){
        StringBuilder b=new StringBuilder();
        for(Object value:values)b.append(value).append('\n');
        return b.toString();
    }
    private static void verify(DxfBlocks.Result result,int drawings){
        check(result.skipped==69,"correct cumulative skip count: "+result.skipped);
        check(result.skippedByType.get("INSERT")==68,"INSERT counts retained");
        check(result.skippedByType.get("ACAD_TABLE")==1,"ACAD_TABLE counts retained");
        check(result.skippedSamples.size()==20,"bounded diagnostic samples");
        DxfBlocks.SkippedRecord first=result.skippedSamples.get(0);
        check("INSERT".equals(first.type)&&"11".equals(first.handle),"first skipped handle");
        check("MECHANICAL".equals(first.layer),"effective layer preserved");
        check(first.reason.contains("absent"),"explicit missing block reason");
        check(result.skippedSamples.get(1).reason.contains("Zero"),"scale failure reason");
        String message=CadSkippedSummary.describe(result.skipped,result.skippedSamples,result.skippedByType);
        check(message.contains("INSERT: 68")&&message.contains("ACAD_TABLE: 1"),"skip summary types");
        check(message.contains("katman=MECHANICAL")&&message.contains("#11"),"summary layer and handle");
        check(message.contains("orijinal DWG'ye göre"),"no full file parity claim");
        check(message.contains("ek atlama")&&message.length()<4000,"bounded displayed summary");
        check(drawings==1,"local valid block geometry remains rendered");
    }
    public static void main(String[]args)throws Exception{
        StringBuilder source=new StringBuilder();
        source.append(code(0,"SECTION",2,"BLOCKS",0,"BLOCK",2,"GOOD",10,0,20,0));
        source.append(code(0,"LINE",10,0,20,0,11,1,21,1));
        source.append(code(0,"ENDBLK",0,"ENDSEC",0,"SECTION",2,"ENTITIES"));
        source.append(code(0,"INSERT",5,"11",8,"MECHANICAL",2,"MISSING"));
        source.append(code(0,"INSERT",5,"12",8,"MECHANICAL",2,"GOOD",41,0));
        source.append(code(0,"INSERT",5,"13",8,"MECHANICAL",2,"GOOD",70,0));
        source.append(code(0,"INSERT",5,"14",8,"MECHANICAL",2,"GOOD",210,1,220,0,230,0));
        source.append(code(0,"ACAD_TABLE",5,"15",8,"SHEET",2,"NOT_THERE"));
        source.append(code(0,"INSERT",5,"16",8,"LAYER1",2,"GOOD",10,4,20,5));
        for(int i=0;i<64;i++)source.append(code(0,"INSERT",5,Integer.toHexString(i+0x20),8,"MISSING_LAYER",2,"UNAVAILABLE"));
        source.append(code(0,"ENDSEC",0,"EOF"));
        DxfBlocks.Result memory=DxfBlocks.expand(Arrays.asList(source.toString().split("\n")));
        verify(memory,memory.placements.size());
        File f=File.createTempFile("musacad-skipped-evidence-", ".dxf");
        try{
            Files.writeString(f.toPath(),source.toString(),StandardCharsets.UTF_8);
            List<DxfBlocks.Placement> rendered=new ArrayList<>();
            DxfBlocks.Result streamed=DxfBlocks.expand(f,StandardCharsets.UTF_8,rendered::add);
            verify(streamed,rendered.size());
        }finally{Files.deleteIfExists(f.toPath());}
        check(CadSkippedSummary.describe(0,Collections.emptyList(),Collections.emptyMap()).contains("kanıtlanmış değildir"),"zero skip not full parity guarantee");
        System.out.println("Skipped CAD diagnostics: streaming/list parity, handle/layer, skip reasons, memory cap passed");
    }
}
