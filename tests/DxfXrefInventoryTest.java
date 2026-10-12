import com.musa.cad.DxfBlocks;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;

public final class DxfXrefInventoryTest {
    private static final String CONTENT=
        "0\nSECTION\n2\nBLOCKS\n"
       +"0\nBLOCK\n2\nMEP_REFERENCE\n70\n4\n1\nMEP/site.dwg\n"
       +"0\nENDBLK\n"
       +"0\nBLOCK\n2\nSTRUCTURE_OVERLAY\n70\n8\n1\nSTRUCT/structure.dwg\n"
       +"0\nENDBLK\n"
       +"0\nBLOCK\n2\nLOCAL_SYMBOL\n70\n0\n10\n0\n20\n0\n"
       +"0\nLINE\n10\n0\n20\n0\n11\n1\n21\n1\n"
       +"0\nENDBLK\n"
       +"0\nENDSEC\n"
       +"0\nSECTION\n2\nENTITIES\n"
       +"0\nINSERT\n2\nMEP_REFERENCE\n"
       +"0\nINSERT\n2\nLOCAL_SYMBOL\n"
       +"0\nENDSEC\n0\nEOF\n";
    private static void check(boolean condition,String message){if(!condition)throw new AssertionError(message);}
    private static void assertRefs(DxfBlocks.Result result,int renderedCount){
        check(result.externalReferences.size()==2,"two XREFs must be inventoried");
        DxfBlocks.ExternalReference a=result.externalReferences.get(0),b=result.externalReferences.get(1);
        check(a.name.equals("MEP_REFERENCE")&&a.path.equals("MEP/site.dwg")&&!a.overlay,
            "attached XREF name/path/flag");
        check(b.name.equals("STRUCTURE_OVERLAY")&&b.path.equals("STRUCT/structure.dwg")&&b.overlay,
            "overlay XREF name/path/flag");
        check(result.skipped>=1,"unresolved XREF must not be reported as drawn geometry");
        check(renderedCount==1,"ordinary local block remains drawable");
    }
    public static void main(String[]args)throws Exception{
        DxfBlocks.Result direct=DxfBlocks.expand(Arrays.asList(CONTENT.split("\n")));
        assertRefs(direct,direct.placements.size());
        File file=File.createTempFile("musacad-xref-", ".dxf");
        try{
            Files.writeString(file.toPath(),CONTENT,StandardCharsets.UTF_8);
            java.util.ArrayList<DxfBlocks.Placement> rendered=new java.util.ArrayList<>();
            DxfBlocks.Result streamed=DxfBlocks.expand(file,StandardCharsets.UTF_8,
                rendered::add);
            assertRefs(streamed,rendered.size());
        }finally{Files.deleteIfExists(file.toPath());}
        String summary=com.musa.cad.CadXrefSummary.describe(direct.externalReferences);
        check(summary.contains("MEP_REFERENCE")&&summary.contains("MEP/site.dwg"),"attached XREF visible in drawing detail");
        check(summary.contains("STRUCTURE_OVERLAY")&&summary.contains("[overlay]"),"overlay flag visible in drawing detail");
        check(summary.contains("yüklenmemiş")&&summary.contains("içeri aktarılmadı"),"do not claim external DWG was loaded");
        String bidiContent=CONTENT.replace("MEP/site.dwg","MEP/\\u202Esite.dwg");
        DxfBlocks.Result bidi=DxfBlocks.expand(Arrays.asList(bidiContent.split("\\n")));
        check(!com.musa.cad.CadXrefSummary.describe(bidi.externalReferences).contains("\\u202E"),
              "untrusted DXF paths must not inject bidi override into Android dialog");
        StringBuilder many=new StringBuilder("0\\nSECTION\\n2\\nBLOCKS\\n");
        for(int i=0;i<19;i++)many.append("0\\nBLOCK\\n2\\nXR").append(i)
            .append("\\n70\\n4\\n1\\nfolder/ref").append(i).append(".dwg\\n0\\nENDBLK\\n");
        many.append("0\\nENDSEC\\n0\\nSECTION\\n2\\nENTITIES\\n0\\nENDSEC\\n0\\nEOF\\n");
        DxfBlocks.Result large=DxfBlocks.expand(Arrays.asList(many.toString().split("\\n")));
        String bounded=com.musa.cad.CadXrefSummary.describe(large.externalReferences);
        check(bounded.contains("7 ek referans")&&!bounded.contains("XR12 [attach]"),"XREF dialog must be bounded");
        System.out.println("XREF attachment, overlay and local-block preservation passed");
    }
}
