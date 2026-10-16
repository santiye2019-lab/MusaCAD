import com.musa.cad.*;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Source record-range integrity proof for atomic BREAK2 save.
 * Only exact direct-root ENTITIES LINE records are replaceable in pairs.
 */
public final class DxfBreakSourceBoundaryGuardTest {
    private static final class Fixture {
        final Path file;
        final SourceRange range;
        Fixture(Path f,SourceRange r){file=f;range=r;}
    }
    private static void group(List<String> lines,int code,String value){
        lines.add(Integer.toString(code));lines.add(value);
    }
    private static Fixture fixture(boolean targetInBlocks,String type)throws Exception{
        List<String> lines=new ArrayList<>();
        group(lines,0,"SECTION");group(lines,2,"HEADER");
        group(lines,9,"$HANDSEED");group(lines,5,"A0");
        group(lines,0,"ENDSEC");
        group(lines,0,"SECTION");group(lines,2,"BLOCKS");
        group(lines,0,"BLOCK");group(lines,2,"B");
        if(targetInBlocks){
            int start=lines.size();group(lines,0,type);group(lines,5,"A1");
            group(lines,8,"M-BORU");group(lines,10,"0");group(lines,20,"0");
            group(lines,11,"20");group(lines,21,"0");
            int end=lines.size();
            group(lines,0,"ENDBLK");group(lines,0,"ENDSEC");
            group(lines,0,"SECTION");group(lines,2,"ENTITIES");
            group(lines,0,"POINT");group(lines,10,"1");group(lines,20,"1");
            group(lines,0,"ENDSEC");group(lines,0,"EOF");
            Path file=Files.createTempFile("musacad-break-guard-block-",".dxf");
            Files.write(file,lines,StandardCharsets.UTF_8);
            return new Fixture(file,new SourceRange(start+2,start,end));
        }
        group(lines,0,"ENDBLK");group(lines,0,"ENDSEC");
        group(lines,0,"SECTION");group(lines,2,"ENTITIES");
        group(lines,0,"LINE");group(lines,5,"AA");group(lines,8,"SURVIVES");
        group(lines,10,"-5");group(lines,20,"0");group(lines,11,"-1");group(lines,21,"0");
        int start=lines.size();group(lines,0,type);group(lines,5,"A1");
        group(lines,8,"M-BORU");group(lines,10,"0");group(lines,20,"0");
        group(lines,11,"20");group(lines,21,"0");
        int end=lines.size();
        group(lines,0,"CIRCLE");group(lines,5,"A2");group(lines,8,"PROTECTED");
        group(lines,10,"30");group(lines,20,"0");group(lines,40,"3");
        group(lines,0,"ENDSEC");group(lines,0,"EOF");
        Path file=Files.createTempFile("musacad-break-guard-",".dxf");
        Files.write(file,lines,StandardCharsets.UTF_8);
        return new Fixture(file,new SourceRange(start+2,start,end));
    }
    private static List<SourceReplacement> fragments(SourceRange range){
        SourceReplacement left=new SourceReplacement(range.sourceId,range,
            "M-BORU",0xFF00FF00,CadEdit.line(0,0,7,0));
        SourceReplacement right=new SourceReplacement(range.sourceId,range,
            "E-PRIZ",0xFFFF0000,CadEdit.line(13,0,20,0));
        return Arrays.asList(left,right);
    }
    private static void verify(Path source,List<SourceReplacement> records,
                               List<SourceRange> removals)throws IOException{
        DxfBreakSourceBoundaryGuard.verify(source.toFile(),StandardCharsets.UTF_8,
                                           records,removals);
    }
    private static void rejected(Path source,List<SourceReplacement> records,
                                 List<SourceRange> removals,String label)throws Exception{
        try{verify(source,records,removals);
            throw new AssertionError("Accepted unsafe BREAK: "+label);
        }catch(IOException expected){}
    }
    public static void main(String[] args)throws Exception{
        Fixture f=fixture(false,"LINE");
        try{
            verify(f.file,fragments(f.range),Collections.singletonList(f.range));
            rejected(f.file,fragments(f.range),Collections.emptyList(),
                     "source deletion missing");
            rejected(f.file,fragments(f.range),Arrays.asList(f.range,f.range),
                     "duplicate source deletion");
            SourceRange spills=new SourceRange(f.range.sourceId,
                          f.range.startLine,f.range.endLineExclusive+2);
            rejected(f.file,fragments(f.range),Collections.singletonList(spills),
                     "would delete next CIRCLE start");
            SourceRange startsEarly=new SourceRange(f.range.sourceId,
                          f.range.startLine-2,f.range.endLineExclusive);
            rejected(f.file,fragments(startsEarly),Collections.singletonList(startsEarly),
                     "would cut preceding LINE");
            SourceRange extendsPast=new SourceRange(f.range.sourceId,
                          f.range.startLine,f.range.endLineExclusive+2);
            rejected(f.file,fragments(extendsPast),Collections.singletonList(extendsPast),
                     "would cut the next CIRCLE");
            SourceRange idMismatch=new SourceRange(f.range.sourceId+2,
                          f.range.startLine,f.range.endLineExclusive);
            List<SourceReplacement> mismatch=new ArrayList<>(fragments(f.range));
            mismatch.set(1,new SourceReplacement(idMismatch.sourceId,idMismatch,
                 "X",0xFF00FF00,CadEdit.line(13,0,20,0)));
            rejected(f.file,mismatch,Collections.singletonList(f.range),
                     "unequal source IDs must not form a safe split");
            List<SourceReplacement> triple=new ArrayList<>(fragments(f.range));
            triple.add(fragments(f.range).get(0));
            rejected(f.file,triple,Collections.singletonList(f.range),
                     "three pieces unsupported without source provenance");
            // Ordinary single-source edits are intentionally not gated.
            verify(f.file,Collections.singletonList(fragments(f.range).get(0)),
                  Collections.singletonList(f.range));
            verify(f.file,Collections.emptyList(),Collections.emptyList());
            // An altered file is detected when its original LINE became CIRCLE.
            Fixture converted=fixture(false,"CIRCLE");
            try{rejected(converted.file,fragments(converted.range),
                         Collections.singletonList(converted.range),
                         "source has changed type");}
            finally{Files.deleteIfExists(converted.file);}
            Fixture nested=fixture(true,"LINE");
            try{rejected(nested.file,fragments(nested.range),
                         Collections.singletonList(nested.range),
                         "record is inside BLOCKS, not ENTITIES");}
            finally{Files.deleteIfExists(nested.file);}
        }finally{Files.deleteIfExists(f.file);}
        System.out.println("BREAK2 exact source LINE identity/range preflight PASS");
    }
}
