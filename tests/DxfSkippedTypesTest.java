import com.musa.cad.DxfBlocks;
import java.io.File;
import java.nio.file.Files;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/** Skipped CAD objects must remain auditable on both loading paths. */
public final class DxfSkippedTypesTest {
    private static final String DXF=
        "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n2\nXREF_MECH\n70\n4\n1\nmissing/mech.dwg\n"
        +"0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        +"0\nINSERT\n2\nXREF_MECH\n"
        +"0\nINSERT\n2\nMISSING_BLOCK\n"
        +"0\nACAD_TABLE\n2\nMISSING_TABLE\n"
        +"0\nLINE\n10\n1\n20\n1\n11\n9\n21\n8\n"
        +"0\nENDSEC\n0\nEOF\n";
    private static void verify(DxfBlocks.Result r){
        if(r.skipped!=3)throw new AssertionError("expected 3 unrenderable references, got "+r.skipped);
        if(r.skippedByType.getOrDefault("INSERT",0)!=2)throw new AssertionError("missing INSERT breakdown");
        if(r.skippedByType.getOrDefault("ACAD_TABLE",0)!=1)throw new AssertionError("missing ACAD_TABLE breakdown");
        int total=0;for(int value:r.skippedByType.values())total+=value;
        if(total!=r.skipped)throw new AssertionError("count vs grouped types mismatch");
    }
    public static void main(String[]args)throws Exception{
        verify(DxfBlocks.expand(Arrays.asList(DXF.split("\n"))));
        File temp=File.createTempFile("musacad-skipped-", ".dxf");
        try{
            Files.writeString(temp.toPath(),DXF,StandardCharsets.UTF_8);
            verify(DxfBlocks.expand(temp,StandardCharsets.UTF_8,placement->{}));
        }finally{Files.deleteIfExists(temp.toPath());}
        System.out.println("DXF skipped-type counts preserved in buffered and streamed readers");
    }
}
