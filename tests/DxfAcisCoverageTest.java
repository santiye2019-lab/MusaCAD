import com.musa.cad.DxfAcisCoverage;
import com.musa.cad.DxfBlocks;
import java.util.ArrayList;
import java.util.List;

/** ACIS source/payload diagnostics are separate from engineering analysis. */
public final class DxfAcisCoverageTest {
    private static void tag(List<String> a,int code,String val){
        a.add(Integer.toString(code));a.add(val);
    }
    private static void start(List<String>a,String type){tag(a,0,type);}
    private static void need(boolean p,String why){if(!p)throw new AssertionError(why);}

    public static void main(String[]args)throws Exception{
        List<String> doc=new ArrayList<>();
        start(doc,"SECTION");tag(doc,2,"ENTITIES");
        // DWG R2018 body backed by external AcDs: entity record survives,
        // but converting it alone offers zero usable geometric data.
        start(doc,"REGION");tag(doc,8,"SOLIDS");tag(doc,5,"A1");tag(doc,70,"2");
        start(doc,"3DSOLID");tag(doc,8,"SOLIDS");tag(doc,5,"A2");
        tag(doc,1,"sat-acis-part-one");tag(doc,3,"sat-acis-part-two");
        start(doc,"BODY");tag(doc,8,"SOLIDS");tag(doc,5,"A3");tag(doc,310,"41424943");
        start(doc,"BODY");tag(doc,8,"SOLIDS");tag(doc,5,"A4");tag(doc,310,"41G1");
        start(doc,"ENDSEC");start(doc,"EOF");

        DxfBlocks.Result expanded=DxfBlocks.expand(doc);
        DxfAcisCoverage.Snapshot audit=expanded.acisCoverage.snapshot();
        need(audit.solidCount==1&&audit.regionCount==1&&audit.bodyCount==2,
            "All three ACIS entity families must be counted");
        need(audit.entityCount()==4,"Audit lost an ACIS entity");
        need(audit.withInlineData==2&&audit.withoutInlineData==2,
            "Entity identity must not be mistaken for valid SAT/SAB payload");
        need(audit.invalidHexChunks==1,"Corrupt SAB hex data was accepted");
        need(audit.inlineTextChars==34,"ASCII payload segments not counted");
        need(audit.inlineBinaryBytes==4,"Valid binary hex should be counted in bytes");
        need(expanded.placements.size()==4,
            "ACIS audit must leave original entity placements untouched");

        DxfAcisCoverage none=new DxfAcisCoverage();
        List<String> tags=List.of("8","0","1","ACIS","3"," continuation");
        none.accept("LINE",tags,0,tags.size());
        need(none.snapshot().entityCount()==0,"Ordinary LINE must remain ignored");

        DxfAcisCoverage bulk=new DxfAcisCoverage();
        List<String> missing=List.of("5","A1","8","SOLIDS");
        for(int i=0;i<100_000;i++)bulk.accept("REGION",missing,0,missing.size());
        need(bulk.snapshot().withoutInlineData==100000,
            "Large count must stream without retaining body data");
        System.out.println("PASS: source ACIS identity and DXF payload are separately audited");
    }
}
