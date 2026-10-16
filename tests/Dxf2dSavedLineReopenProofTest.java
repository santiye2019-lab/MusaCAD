import com.musa.cad.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;

/**
 * Actual disk DXF semantic reload, independent code/value reader.
 * It compares precisely emitted LINE handles, coordinates, layer,
 * TrueColor/BYLAYER/BYBLOCK, linetype/scale, lineweight, and layout.
 * No Android device / DWG binary / AutoCAD acceptance is claimed.
 */
public final class Dxf2dSavedLineReopenProofTest {
    private static void check(boolean yes,String message){
        if(!yes)throw new AssertionError(message);
    }
    private static final SourceReplacement FIRST=new SourceReplacement(
        120,new SourceRange(120,118,135),"M-BORU",0xFF1234FE,
        SourceReplacement.COLOR_EXPLICIT,"CENTER",2.25d,70,
        CadEdit.line(0,0,7,0));
    private static final SourceReplacement SECOND=new SourceReplacement(
        120,new SourceRange(120,118,135),"E-PRIZ",0xFF22BB44,
        SourceReplacement.COLOR_BYLAYER,"HIDDEN",1.25d,18,
        CadEdit.line(13,0,20,0));
    private static List<Dxf2dSavedLineReopenProof.Expected> expect()throws IOException{
        return Arrays.asList(
            new Dxf2dSavedLineReopenProof.Expected("A20","Model",FIRST,0,0,7,0),
            new Dxf2dSavedLineReopenProof.Expected("A21","Layout1",SECOND,13,0,20,0)
        );
    }
    private static String valid()throws IOException{
        StringWriter bytes=new StringWriter();
        BufferedWriter writer=new BufferedWriter(bytes);
        writer.write("0\nSECTION\n2\nENTITIES\n");
        DxfLineRecordWriter.write(writer,FIRST,"Model",0,0,7,0,"A20");
        DxfLineRecordWriter.write(writer,SECOND,"Layout1",13,0,20,0,"A21");
        writer.write("0\nENDSEC\n0\nEOF\n");
        writer.flush();return bytes.toString();
    }
    private static void verify(Path path,List<Dxf2dSavedLineReopenProof.Expected> expected)
            throws IOException{
        Dxf2dStructureVerifier.verify(path.toFile());
        Dxf2dSavedLineReopenProof.verify(path.toFile(),StandardCharsets.UTF_8,expected);
    }
    private static String tag(int code,String value){
        return String.format(java.util.Locale.US,"%3d",code)+"\n"+value+"\n";
    }
    private static String change(String document,int code,String from,String to){
        String old=tag(code,from);
        if(!document.contains(old))throw new AssertionError("missing DXF test group "+code+"="+from);
        return document.replace(old,tag(code,to));
    }
    private static void fails(Path path,String altered,String label)throws Exception{
        Files.writeString(path,altered,StandardCharsets.UTF_8);
        try{verify(path,expect());throw new AssertionError("accepted "+label);}
        catch(IOException expected){}
    }
    public static void main(String[] args)throws Exception{
        Path dir=Files.createTempDirectory("musacad-2d-semantic-reopen-");
        try{
            Path saved=dir.resolve("saved.dxf");
            String good=valid();
            Files.writeString(saved,good,StandardCharsets.UTF_8);
            verify(saved,expect());
            // A valid DXF document can still carry wrong source geometry,
            // wrong styles or missing children and MUST fail semantic readback.
            fails(saved,change(good,11,"7","6"),"first endpoint");
            fails(saved,change(good,10,"13","12"),"second endpoint");
            fails(saved,change(good,8,"M-BORU","M-SOGUTMA"),"first layer");
            fails(saved,change(good,8,"E-PRIZ","E-ZAYIF"),"second independent layer");
            fails(saved,change(good,6,"CENTER","DASHED"),"first line pattern");
            fails(saved,change(good,48,"2.25","1.25"),"first line scale");
            fails(saved,change(good,370,"70","35"),"first lineweight");
            fails(saved,change(good,420,"1193214","1193215"),"explicit TrueColor");
            fails(saved,good.replace(tag(8,"E-PRIZ"),
                    tag(8,"E-PRIZ")+tag(420,"11")),"BYLAYER becomes explicit");
            fails(saved,change(good,410,"Layout1","Layout2"),"child layout");
            fails(saved,change(good,5,"A21","A22"),"child handle changed");
            // A syntactically valid DXF missing only the second BREAK child.
            String secondMarker=tag(0,"LINE")+tag(5,"A21");
            int second=good.indexOf(secondMarker);
            if(second<0)throw new AssertionError("second LINE fixture not found");
            String single=good.substring(0,second)+"0\\nENDSEC\\n0\\nEOF\\n";
            fails(saved,single,"second LINE missing");

            // BYBLOCK saves ACI0 and no explicit group420.
            SourceReplacement byblock=new SourceReplacement(120,
                new SourceRange(120,118,135),"M-BORU",0xFF0000FF,
                SourceReplacement.COLOR_BYBLOCK,"CONTINUOUS",1d,25,
                CadEdit.line(1,2,4,5));
            StringWriter buffer=new StringWriter();
            try(BufferedWriter w=new BufferedWriter(buffer)){
                w.write("0\nSECTION\n2\nENTITIES\n");
                DxfLineRecordWriter.write(w,byblock,"Model",1,2,4,5,"AF0");
                w.write("0\nENDSEC\n0\nEOF\n");
            }
            Files.writeString(saved,buffer.toString(),StandardCharsets.UTF_8);
            verify(saved,Collections.singletonList(
                new Dxf2dSavedLineReopenProof.Expected(
                    "AF0","Model",byblock,1,2,4,5)));
            Files.writeString(saved,change(buffer.toString(),62,"0","256"),
                              StandardCharsets.UTF_8);
            try{
                verify(saved,Collections.singletonList(
                    new Dxf2dSavedLineReopenProof.Expected(
                        "AF0","Model",byblock,1,2,4,5)));
                throw new AssertionError("wrong BYBLOCK ACI accepted");
            }catch(IOException expected){}

            // Real staged-save callback: reject semantic mutation before
            // provider/SAF opener can be invoked.
            Path source=dir.resolve("source.dxf");
            Files.writeString(source,good,StandardCharsets.UTF_8);
            File cache=Files.createDirectory(dir.resolve("cache")).toFile();
            try{
                Dxf2dSafeSave.stage(source.toFile(),cache,
                    out->out.write(good.replace("11\n7\n","11\n6\n")
                                     .getBytes(StandardCharsets.UTF_8)),
                    stage->{
                        Dxf2dStructureVerifier.verify(stage);
                        Dxf2dSavedLineReopenProof.verify(
                            stage,StandardCharsets.UTF_8,expect());
                    });
                throw new AssertionError("wrong coordinates accepted in staging");
            }catch(IOException expected){}
            check(cache.listFiles().length==0,"failed stage file not cleaned");
            Dxf2dSafeSave.VerifiedStage passed=Dxf2dSafeSave.stage(
                source.toFile(),cache,out->out.write(good.getBytes(StandardCharsets.UTF_8)),
                stage->{
                    Dxf2dStructureVerifier.verify(stage);
                    Dxf2dSavedLineReopenProof.verify(
                        stage,StandardCharsets.UTF_8,expect());
                });
            try{
                ByteArrayOutputStream target=new ByteArrayOutputStream();
                long bytes=Dxf2dSafeSave.publish(passed,()->target);
                check(bytes==good.getBytes(StandardCharsets.UTF_8).length&&
                      good.equals(target.toString("UTF-8")),
                      "staged verified file differs from published DXF");
            }finally{passed.delete();}
            check(cache.listFiles().length==0,"staging cleanup");
            System.out.println("DXF saved LINE per-child semantic readback and staged publication PASS");
        }finally{
            try(java.util.stream.Stream<Path> files=Files.walk(dir)){
                files.sorted(Comparator.reverseOrder()).forEach(p->{
                    try{Files.deleteIfExists(p);}catch(IOException ignored){}
                });
            }
        }
    }
}
