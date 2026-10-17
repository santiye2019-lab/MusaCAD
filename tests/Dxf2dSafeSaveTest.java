import com.musa.cad.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.concurrent.atomic.AtomicBoolean;

/** Pure host-JVM two-phase 2D DXF export contract, no phone or APK. */
public final class Dxf2dSafeSaveTest {
    private static void require(boolean ok,String message){
        if(!ok)throw new AssertionError(message);
    }
    private static String valid(){
        return "0\nSECTION\n2\nHEADER\n9\n$HANDSEED\n5\nA2\n0\nENDSEC\n"+
               "0\nSECTION\n2\nENTITIES\n"+
               "0\nLINE\n5\nA1\n8\nM-BORU\n10\n0\n20\n0\n11\n20\n21\n0\n"+
               "0\nENDSEC\n0\nEOF\n";
    }
    private static void write(OutputStream destination,String payload)throws IOException{
        // Simulate the real DxfWriter, which closes its own wrapper.
        try(BufferedWriter writer=new BufferedWriter(
                new OutputStreamWriter(destination,StandardCharsets.UTF_8))){
            writer.write(payload);
        }
    }
    private static boolean invalid(File stage){
        try{Dxf2dStructureVerifier.verify(stage);return false;}
        catch(IOException expected){return true;}
    }
    private static File tempFile(Path folder,String name,String value)throws IOException{
        File f=folder.resolve(name).toFile();
        Files.writeString(f.toPath(),value,StandardCharsets.UTF_8);
        return f;
    }
    private static void verifyRejectedStage(File source,File cache,
                                            String payload)throws Exception{
        AtomicBoolean opened=new AtomicBoolean(false);
        try{
            Dxf2dSafeSave.VerifiedStage stage=Dxf2dSafeSave.stage(
                source,cache,out->write(out,payload),
                Dxf2dStructureVerifier::verify);
            stage.delete();
            throw new AssertionError("Malformed DXF staged and accepted");
        }catch(IOException expected){}
        require(!opened.get(),"invalid staged DXF must not open provider");
    }
    public static void main(String[] args)throws Exception{
        Path folder=Files.createTempDirectory("musacad-dxf-stage-test-");
        try{
            File source=tempFile(folder,"source.dxf",valid());
            File cache=Files.createDirectory(folder.resolve("cache")).toFile();
            Dxf2dSafeSave.VerifiedStage stage=Dxf2dSafeSave.stage(
                source,cache,out->write(out,valid()),
                Dxf2dStructureVerifier::verify);
            try{
                AtomicBoolean opened=new AtomicBoolean(false);
                ByteArrayOutputStream saved=new ByteArrayOutputStream();
                long bytes=Dxf2dSafeSave.publish(stage,()->{
                    opened.set(true);return saved;
                });
                require(opened.get()&&bytes==valid().getBytes(StandardCharsets.UTF_8).length,
                        "staged output must publish full bytes only after proof");
                require(valid().equals(saved.toString("UTF-8")),
                        "saved output must be identical to verified stage");
                stage.verify();
            }finally{stage.delete();}
            require(cache.listFiles().length==0,"successful stage must be cleaned");

            // Missing EOF and duplicate handles must be rejected on stage,
            // before the document provider gets any access.
            verifyRejectedStage(source,cache,valid().replace("0\nEOF\n",""));
            // Same 64-bit handle with different ASCII case/leading zero must
            // be rejected, including ENTITIES and DIMSTYLE 105.
            verifyRejectedStage(source,cache,valid().replace(
                "0\nENDSEC\n0\nEOF\n",
                "0\nLINE\n5\n000a1\n10\n0\n20\n0\n11\n1\n21\n1\n"+
                "0\nENDSEC\n0\nEOF\n"));
            verifyRejectedStage(source,cache,valid().replace(
                "0\nSECTION\n2\nENTITIES\n",
                "0\nSECTION\n2\nTABLES\n0\nDIMSTYLE\n105\na1\n0\nENDSEC\n"+
                "0\nSECTION\n2\nENTITIES\n"));
            verifyRejectedStage(source,cache,valid().replace(
                "0\nLINE\n5\nA1\n","0\nLINE\n5\n0\n"));
            verifyRejectedStage(source,cache,valid().replace(
                "0\nLINE\n5\nA1\n","0\nLINE\n5\nFFFFFFFFFFFFFFFFF\n"));
            verifyRejectedStage(source,cache,valid().replace("0\nENDSEC\n0\nEOF\n",
                "0\nLINE\n5\nA1\n10\n1\n20\n2\n11\n3\n21\n4\n"+
                "0\nENDSEC\n0\nEOF\n"));
            verifyRejectedStage(source,cache,valid().replace("11\n20\n21\n0\n",
                "11\nNaN\n21\n0\n"));
            verifyRejectedStage(source,cache,valid().replace("11\n20\n21\n0\n",
                "21\n0\n"));
            verifyRejectedStage(source,cache,valid().replace("11\n20\n21\n0\n",
                "11\n1e999\n21\n0\n"));
            verifyRejectedStage(source,cache,valid().replace("11\n20\n21\n0\n",
                "11\n0x1p2\n21\n0\n"));
            // The SAME portable C state machine must reject structural traps
            // before SAF destination is opened, even if file bytes are valid.
            verifyRejectedStage(source,cache,valid().replace(
                "0\nSECTION\n2\nENTITIES\n",
                "0\nSECTION\n2\nENTITIES\n0\nSECTION\n2\nBLOCKS\n"));
            verifyRejectedStage(source,cache,valid().replace(
                "0\nSECTION\n2\nENTITIES\n",
                "0\nSECTION\n0\nLINE\n"));
            verifyRejectedStage(source,cache,valid().replace(
                "0\nENDSEC\n0\nEOF\n",
                "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n"));
            verifyRejectedStage(source,cache,valid()+"0\nLINE\n");
            require(cache.listFiles().length==0,
                    "all invalid temporary outputs must be deleted");

            // DXF writer throws after writing a partial temp output.
            try{
                Dxf2dSafeSave.stage(source,cache,out->{
                    out.write("0\nSECTION\n".getBytes(StandardCharsets.UTF_8));
                    throw new IOException("simulated writer failure");
                },Dxf2dStructureVerifier::verify);
                throw new AssertionError("writer failure incorrectly accepted");
            }catch(IOException expected){}
            require(cache.listFiles().length==0,"failed writer must clean stage");

            // A changed source with identical size/mtime is detected before
            // opening the destination, even after a successful stage.
            stage=Dxf2dSafeSave.stage(source,cache,out->write(out,valid()),
                                     Dxf2dStructureVerifier::verify);
            try{
                long time=source.lastModified();
                String mutated=valid().replace("M-BORU","E-BORU");
                require(mutated.length()==valid().length(),"same-size source fixture");
                Files.writeString(source.toPath(),mutated,StandardCharsets.UTF_8);
                source.setLastModified(time);
                AtomicBoolean opened=new AtomicBoolean(false);
                try{
                    Dxf2dSafeSave.publish(stage,()->{
                        opened.set(true);return new ByteArrayOutputStream();
                    });
                    throw new AssertionError("source mutation accepted");
                }catch(IOException expected){}
                require(!opened.get(),"mutated source must be refused before SAF open");
            }finally{stage.delete();}
            Files.writeString(source.toPath(),valid(),StandardCharsets.UTF_8);

            // A hostile or damaged staged file with same length/mtime is
            // detected before opening the destination.
            stage=Dxf2dSafeSave.stage(source,cache,out->write(out,valid()),
                                     Dxf2dStructureVerifier::verify);
            try{
                File staged=stage.file();
                long stamp=staged.lastModified();
                String poisoned=valid().replace("M-BORU","X-BORU");
                Files.writeString(staged.toPath(),poisoned,StandardCharsets.UTF_8);
                staged.setLastModified(stamp);
                AtomicBoolean opened=new AtomicBoolean(false);
                try{
                    Dxf2dSafeSave.publish(stage,()->{
                        opened.set(true);return new ByteArrayOutputStream();
                    });
                    throw new AssertionError("staged tampering accepted");
                }catch(IOException expected){}
                require(!opened.get(),"tampered stage must not open SAF");
            }finally{stage.delete();}

            // No semantic comparison beyond DXF syntax is asserted.
            require(cache.listFiles().length==0,"no stage files leaked");
            System.out.println("2D DXF staged save, source SHA-256 and SAF prepublish gate PASS");
        }finally{
            try(java.util.stream.Stream<Path> paths=Files.walk(folder)){
                paths.sorted(java.util.Comparator.reverseOrder()).forEach(p->{
                    try{Files.deleteIfExists(p);}catch(IOException ignored){}
                });
            }
        }
    }
}
