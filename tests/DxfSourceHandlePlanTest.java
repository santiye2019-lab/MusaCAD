import com.musa.cad.*;
import java.io.*;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;

/**
 * Host-only actual DXF source scan and on-disk LINE handle readback.
 * Does not assert original binary DWG identity equivalence.
 */
public final class DxfSourceHandlePlanTest {
    private static void require(boolean b,String message){
        if(!b)throw new AssertionError(message);
    }
    private static void tag(BufferedWriter out,int group,String value)throws IOException{
        out.write(Integer.toString(group));out.newLine();
        out.write(value);out.newLine();
    }
    private static Path source(String headerSeed,String first,String second,String dimstyle)
            throws Exception{
        Path path=Files.createTempFile("musacad-handle-source-",".dxf");
        try(BufferedWriter w=Files.newBufferedWriter(path,StandardCharsets.UTF_8)){
            tag(w,0,"SECTION");tag(w,2,"HEADER");
            tag(w,9,"$HANDSEED");tag(w,5,headerSeed);
            tag(w,0,"ENDSEC");
            tag(w,0,"SECTION");tag(w,2,"TABLES");
            tag(w,0,"TABLE");tag(w,2,"DIMSTYLE");
            tag(w,0,"DIMSTYLE");tag(w,105,dimstyle);tag(w,2,"MyStyle");
            tag(w,0,"ENDTAB");tag(w,0,"ENDSEC");
            tag(w,0,"SECTION");tag(w,2,"ENTITIES");
            tag(w,0,"LINE");tag(w,5,first);tag(w,8,"0");
            tag(w,10,"0");tag(w,20,"0");tag(w,11,"20");tag(w,21,"0");
            tag(w,0,"LINE");tag(w,5,second);tag(w,8,"0");
            tag(w,10,"30");tag(w,20,"0");tag(w,11,"40");tag(w,21,"0");
            tag(w,0,"ENDSEC");tag(w,0,"EOF");
        }
        return path;
    }

    /** Simulates DxfWriter's header rewrite and exact LINE record emitter. */
    private static Path save(Path original,DxfSourceHandlePlan plan)throws Exception{
        Path path=Files.createTempFile("musacad-saved-identities-",".dxf");
        SourceReplacement left=new SourceReplacement(18,new SourceRange(18,0,10),
             "M-BORU",0xFF123456,SourceReplacement.COLOR_EXPLICIT,"DASHED",1.5d,35,
             CadEdit.line(0,0,7,0));
        SourceReplacement right=new SourceReplacement(18,new SourceRange(18,0,10),
             "E-PRIZ",0xFF00FF00,SourceReplacement.COLOR_BYLAYER,"BYLAYER",1d,25,
             CadEdit.line(13,0,20,0));
        try(BufferedReader reader=Files.newBufferedReader(original,StandardCharsets.UTF_8);
            BufferedWriter writer=Files.newBufferedWriter(path,StandardCharsets.UTF_8)){
            boolean header=false,pendingSection=false,nextIsSeed=false,inEntities=false;
            while(true){
                String codeLine=reader.readLine();
                if(codeLine==null)break;
                String val=reader.readLine();
                if(val==null)throw new IOException("bad fixture");
                int group=Integer.parseInt(codeLine.trim());
                String value=val.trim();
                if(group==0&&"SECTION".equals(value))pendingSection=true;
                else if(pendingSection&&group==2){
                    header="HEADER".equals(value);
                    inEntities="ENTITIES".equals(value);
                    pendingSection=false;
                }else if(group==0&&"ENDSEC".equals(value)){
                    if(inEntities){
                        DxfLineRecordWriter.write(writer,left,"Model",0,0,7,0,
                                                  plan.handleAt(0));
                        DxfLineRecordWriter.write(writer,right,"Model",13,0,20,0,
                                                  plan.handleAt(1));
                    }
                    inEntities=header=false;
                }else if(header&&group==9)nextIsSeed="$HANDSEED".equals(value);
                else if(header&&nextIsSeed&&group==5){
                    value=plan.nextSeed();nextIsSeed=false;
                }
                tag(writer,group,value);
            }
        }
        return path;
    }

    private static List<Map<Integer,String>> readLines(Path path)throws Exception{
        List<Map<Integer,String>> out=new ArrayList<>();
        boolean sectionPending=false,inEntities=false;
        try(BufferedReader r=Files.newBufferedReader(path,StandardCharsets.UTF_8)){
            Map<Integer,String> current=null;
            while(true){
                String raw=r.readLine();if(raw==null)break;
                String value=r.readLine();
                if(value==null)throw new IOException("truncated output DXF");
                int group=Integer.parseInt(raw.trim());value=value.trim();
                if(group==0&&"SECTION".equals(value)){sectionPending=true;continue;}
                if(sectionPending&&group==2){
                    inEntities="ENTITIES".equals(value);sectionPending=false;continue;
                }
                if(group==0&&"ENDSEC".equals(value)){inEntities=false;continue;}
                if(!inEntities)continue;
                if(group==0&&"LINE".equals(value)){
                    current=new LinkedHashMap<>();out.add(current);current.put(0,value);
                }else if(current!=null)current.put(group,value);
            }
        }
        return out;
    }
    private static void rejected(Path fixture)throws Exception{
        try{DxfSourceHandlePlan.prepare(fixture.toFile(),StandardCharsets.UTF_8,2);
            throw new AssertionError("bad source handle accepted");
        }catch(IOException expected){}
    }
    public static void main(String[] args)throws Exception{
        Path input=source("0100","00a1","B2","3FF");
        Path output=null;
        try{
            DxfSourceHandlePlan plan=DxfSourceHandlePlan.prepare(
                input.toFile(),StandardCharsets.UTF_8,2);
            require(plan.count()==2,"two replacement LINE handles");
            require("400".equals(plan.handleAt(0))&&"401".equals(plan.handleAt(1))&&
                    "402".equals(plan.nextSeed()),"reserve DIMSTYLE 105 and header seed");
            output=save(input,plan);
            List<Map<Integer,String>> lines=readLines(output);
            require(lines.size()==4,"preserve original two plus two replacement LINEs");
            require("A1".equals(DxfSourceHandlePlan.canonical(lines.get(0).get(5)))&&
                    "B2".equals(DxfSourceHandlePlan.canonical(lines.get(1).get(5))),
                    "old DXF handles unchanged");
            require("400".equals(lines.get(2).get(5))&&
                    "401".equals(lines.get(3).get(5)),
                    "two new LINE groups have distinct IDs");
            require(!lines.get(2).get(5).equals(lines.get(3).get(5)),
                    "unique saved identities");
            require("M-BORU".equals(lines.get(2).get(8))&&
                    "E-PRIZ".equals(lines.get(3).get(8))&&
                    "1193046".equals(lines.get(2).get(420))&&
                    !lines.get(3).containsKey(420),
                    "styles and BYLAYER survive identity-enabled DXF write");
            String disk=Files.readString(output);
            require(disk.contains("$HANDSEED\n5\n402\n"),
                    "DXF HEADER next handle must advance past both children");
            DxfSourceHandlePlan secondSave=DxfSourceHandlePlan.prepare(
                output.toFile(),StandardCharsets.UTF_8,1);
            require(new BigInteger(secondSave.handleAt(0),16).compareTo(
                        new BigInteger("402",16))>0,
                    "save/reopen/resave must not reuse child handle");

            // Invalid or duplicate-form hexadecimal values are rejected before
            // the writer emits any half-formed output entity.
            SourceReplacement replacement=new SourceReplacement(18,
                new SourceRange(18,0,10),"M",0xFF123456,CadEdit.line(0,0,7,0));
            StringWriter sink=new StringWriter();
            BufferedWriter w=new BufferedWriter(sink);
            try{
                DxfLineRecordWriter.write(w,replacement,"Model",0,0,7,0,"BAD!");
                throw new AssertionError("malformed handle accepted");
            }catch(IOException expected){}
            w.flush();require(sink.toString().isEmpty(),"invalid handle leaked DXF groups");
            for(String hex:new String[]{"0","-1","GG","ABCDEF12345678901"}){
                try{DxfSourceHandlePlan.canonical(hex);
                    throw new AssertionError("invalid handle accepted "+hex);
                }catch(IOException expected){}
            }
            try{DxfSourceHandlePlan.prepare(input.toFile(),
                         StandardCharsets.UTF_8,0);
                throw new AssertionError("zero replacement count accepted");
            }catch(IOException expected){}
        }finally{
            Files.deleteIfExists(input);
            if(output!=null)Files.deleteIfExists(output);
        }
        Path bad=source("FF","0","A2","B3");
        try{rejected(bad);}finally{Files.deleteIfExists(bad);}
        Path exhausted=source("FFFFFFFFFFFFFFFE","1","2","3");
        try{rejected(exhausted);}finally{Files.deleteIfExists(exhausted);}
        Path malformed=source("A1","ZZ","2","3");
        try{rejected(malformed);}finally{Files.deleteIfExists(malformed);}
        System.out.println("DXF source LINE handles unique, HEADER HANDSEED updated and saved readback OK");
    }
}
