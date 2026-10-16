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
    private static List<Dxf2dSavedLineReopenProof.Expected> expect()throws Exception{
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
            fails(saved,good.replace("11\n7\n","11\n6\n"),"first endpoint");
            fails(saved,good.replace("10\n13\n","10\n12\n"),"second endpoint");
            fails(saved,good.replace("8\nM-BORU\n","8\nM-SOGUTMA\n"),
                  "first layer");
            fails(saved,good.replace("8\nE-PRIZ\n","8\nE-ZAYIF\n"),
                  "second independent layer");
            fails(saved,good.replace("6\nCENTER\n","6\nDASHED\n"),
                  "first line pattern");
            fails(saved,good.replace("48\n2.25\n","48\n1.25\n"),
                  "first line scale");
            fails(saved,good.replace("370\n70\n","370\n35\n"),
                  "first lineweight");
            fails(saved,good.replace("420\n1193214\n","420\n1193215\n"),
                  "explicit TrueColor");
            fails(saved,good.replace("8\nE-PRIZ\n",
                                    "8\nE-PRIZ\n420\n11\n"),
                  "BYLAYER becomes explicit");
            fails(saved,good.replace("410\nLayout1\n",
                                    "410\nLayout2\n"),
                  "child layout");
            fails(saved,good.replace("5\nA21\n","5\nA22\n"),
                  "child handle changed");
            // Syntactically valid DXF with second child omitted.
            String single=good.substring(0,good.indexOf("0\nLINE\n5\nA21\n"))+
                          "0\nENDSEC\n0\nEOF\n";
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
            fails(saved,buffer.toString().replace("62\n0\n","62\n256\n"),
                  "wrong BYBLOCK ACI");

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
