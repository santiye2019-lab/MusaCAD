import com.musa.cad.CadDxfSaveStage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Must not open the SAF destination until the entire DXF has passed validation. */
public final class CadDxfSaveStageTest {
    private static final String VALID=
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n"
       +"0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n1.123456789012\n"
       +"20\n2\n11\n3\n21\n4\n0\nENDSEC\n0\nEOF\n";
    private static void yes(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    private static void rejected(File cache,String source,String message)throws Exception{
        boolean rejected=false;
        try{CadDxfSaveStage.prepare(cache,out->out.write(source.getBytes(StandardCharsets.UTF_8)));}
        catch(IOException expected){rejected=true;}
        yes(rejected,message+" must be rejected");
        String[] remaining=cache.list((dir,name)->name.startsWith("MusaCAD_dxf_kayit_"));
        yes(remaining!=null&&remaining.length==0,"rejected DXF leaked a staging file");
    }
    public static void main(String[]args)throws Exception{
        Path folder=Files.createTempDirectory("cad-dxf-stage");
        File cache=folder.toFile();
        try{
            rejected(cache,"0\nSECTION\n2\nENTITIES\n0\nENDSEC\n",
                "missing EOF");
            rejected(cache,"0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nEOF\n",
                "missing ENTITIES");
            rejected(cache,VALID.replace("0\nENDSEC\n0\nEOF\n","0\nEOF\n"),
                "unterminated ENTITIES");
            rejected(cache,VALID+"0\nPOINT\n","trailing data after EOF");
            rejected(cache,VALID+"0\n","incomplete label pair");
            rejected(cache,"NOT_AN_INT\nSECTION\n"+VALID,"bad group code");
            rejected(cache,"0\nSECTION\n2\nENTITIES\n0\nSECTION\n2\nHEADER\n"
                +"0\nENDSEC\n0\nEOF\n","nested section");
            boolean caught=false;
            try{
                CadDxfSaveStage.prepare(cache,out->{
                    out.write(VALID.getBytes(StandardCharsets.UTF_8));
                    throw new IOException("writer fail after partial output");
                });
            }catch(IOException expected){caught=true;}
            yes(caught,"writer exception must abort staging");

            ByteArrayOutputStream destination=new ByteArrayOutputStream();
            destination.write("PROTECT EXISTING".getBytes(StandardCharsets.UTF_8));
            try{
                CadDxfSaveStage.prepare(cache,out->out.write("0\nEOF\n".getBytes(StandardCharsets.UTF_8)));
                throw new AssertionError("invalid saved file accepted");
            }catch(IOException expected){}
            yes(destination.toString(StandardCharsets.UTF_8).equals("PROTECT EXISTING"),
                "invalid staging modified destination");

            File stage=CadDxfSaveStage.prepare(cache,
                out->out.write(VALID.getBytes(StandardCharsets.UTF_8)));
            try{
                ByteArrayOutputStream good=new ByteArrayOutputStream();
                long bytes=CadDxfSaveStage.publish(stage,good);
                yes(bytes==stage.length(),"published size");
                yes(VALID.equals(good.toString(StandardCharsets.UTF_8)),
                    "all original DXF tags and coordinate precision preserved");
            }finally{
                Files.deleteIfExists(stage.toPath());
            }
            yes(cache.list().length==0,"stage cleaned");
            System.out.println("Staged DXF preflight and source-side nontruncating save tests passed.");
        }finally{Files.deleteIfExists(folder);}
    }
}
