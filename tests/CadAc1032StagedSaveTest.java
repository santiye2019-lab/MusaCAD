import com.musa.cad.CadAc1032StagedSave;
import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.stream.Stream;

/**
 * Acceptance-protocol tests use SYNTHETIC bytes and a stub verifier.
 * They DO NOT prove any AC1032 binary encoder or independent CAD reader.
 */
public final class CadAc1032StagedSaveTest {
    private static final byte[] AC1032_STUB=new byte[128];
    static{
        byte[] magic="AC1032".getBytes(StandardCharsets.US_ASCII);
        System.arraycopy(magic,0,AC1032_STUB,0,magic.length);
        Arrays.fill(AC1032_STUB,6,AC1032_STUB.length,(byte)0x42);
    }

    private static void need(boolean value,String reason){
        if(!value)throw new AssertionError(reason);
    }
    private static CadAc1032StagedSave.Encoder stubEncoder=(original,edit,out)->
            out.write(AC1032_STUB);
    private static CadAc1032StagedSave.IndependentReopenVerifier stubVerifier=
            (original,edit,staged)->true;

    private static boolean rejected(File original,File edit,File out,
                    CadAc1032StagedSave.Encoder encoder,
                    CadAc1032StagedSave.IndependentReopenVerifier verifier){
        try{
            CadAc1032StagedSave.publishNew(original,edit,out,encoder,verifier);
            return false;
        }catch(IOException expected){return true;}
    }

    private static long leftovers(Path folder)throws IOException{
        try(Stream<Path> files=Files.list(folder)){
            return files.filter(x->x.getFileName().toString()
                    .startsWith(".musacad-ac1032-")).count();
        }
    }

    public static void main(String[] args)throws Exception{
        Path folder=Files.createTempDirectory("musacad-ac1032-staging-");
        try{
            File source=folder.resolve("original.dwg").toFile();
            File edit=folder.resolve("edited.dxf").toFile();
            File destination=folder.resolve("new-modern.dwg").toFile();
            Files.write(source.toPath(),AC1032_STUB);
            byte[] original=Files.readAllBytes(source.toPath());
            byte[] initialEdit="0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n"
                    .getBytes(StandardCharsets.UTF_8);
            Files.write(edit.toPath(),initialEdit);

            need(rejected(source,edit,destination,null,stubVerifier),
                    "no encoder must not publish");
            need(rejected(source,edit,destination,stubEncoder,null),
                    "no independent verifier must not publish");
            need(!destination.exists(),"unconfigured encoder refuses publication");
            need(rejected(source,edit,source,stubEncoder,stubVerifier),
                    "cannot overwrite original DWG");
            need(rejected(source,edit,edit,stubEncoder,stubVerifier),
                    "cannot overwrite edited DXF");

            CadAc1032StagedSave.Encoder oldVersion=(s,e,out)->
                    out.write("AC1015".getBytes(StandardCharsets.US_ASCII));
            need(rejected(source,edit,destination,oldVersion,stubVerifier),
                    "R2000 output may not impersonate AC1032");
            CadAc1032StagedSave.Encoder fakeModern=(s,e,out)->
                    out.write(new byte[128]);
            need(rejected(source,edit,destination,fakeModern,stubVerifier),
                    "wrong magic refused");
            need(rejected(source,edit,destination,stubEncoder,
                    (s,e,candidate)->false),
                    "failed semantic reopen must block output");
            need(leftovers(folder)==0,"failed verification removes stage");

            byte[] changed=initialEdit.clone();
            changed[3]^=0x01;
            need(rejected(source,edit,destination,
                    (s,e,out)->{out.write(AC1032_STUB);Files.write(e.toPath(),changed);},
                    stubVerifier),"modified edit during writer rejects");
            Files.write(edit.toPath(),initialEdit);

            byte[] changedSource=original.clone();
            changedSource[80]^=0x01;
            need(rejected(source,edit,destination,
                    (s,e,out)->{out.write(AC1032_STUB);
                        Files.write(s.toPath(),changedSource);},
                    stubVerifier),"modified original during encoder rejects");
            Files.write(source.toPath(),original);

            // Distinct names may still alias the SAME inode through a hard
            // link. Source and edit must have independent backing files.
            Path alias=folder.resolve("alias-edited.dxf");
            Files.createLink(alias,source.toPath());
            need(rejected(source,alias.toFile(),destination,stubEncoder,stubVerifier),
                    "hardlinked DXF alias of the original DWG must be rejected");
            Files.delete(alias);

            // A buggy/malicious independent reader must not be able to
            // replace the staged file with a symlink after SHA capture.
            need(rejected(source,edit,destination,stubEncoder,
                    (s,e,stage)->{
                        Files.delete(stage.toPath());
                        Files.createSymbolicLink(stage.toPath(),s.toPath());
                        return true;
                    }),"symlink-swapped stage must be rejected");
            need(Arrays.equals(Files.readAllBytes(source.toPath()),original),
                    "symlink-stage cleanup must never remove original DWG");

            need(rejected(source,edit,destination,stubEncoder,
                    (s,e,stage)->{
                        byte[] altered=Files.readAllBytes(stage.toPath());
                        altered[85]^=0x2a;
                        Files.write(stage.toPath(),altered);
                        return true;
                    }),"staged DWG mutation by verifier rejects");
            need(leftovers(folder)==0,"modified stage cleaned");

            // This validates only the publication protocol: stub verifier
            // is intentionally not a real DWG decode/geometry parity check.
            CadAc1032StagedSave.publishNew(source,edit,destination,
                    stubEncoder,stubVerifier);
            need(destination.isFile()&&
                 Arrays.equals(Files.readAllBytes(destination.toPath()),AC1032_STUB),
                 "preverified synthetic data published create-new-only");
            need(Arrays.equals(Files.readAllBytes(source.toPath()),original),
                 "original DWG remains byte-identical");
            need(Arrays.equals(Files.readAllBytes(edit.toPath()),initialEdit),
                 "edited DXF remains byte-identical");
            need(leftovers(folder)==0,"no stage files remain after publication");
            need(rejected(source,edit,destination,stubEncoder,stubVerifier),
                 "existing destination NEVER overwritten");
            need(Arrays.equals(Files.readAllBytes(destination.toPath()),AC1032_STUB),
                 "existing destination unchanged after rejected overwrite");
            System.out.println("AC1032 staged-save safety protocol PASS (synthetic bytes; no real DWG writer)");
        }finally{
            try(Stream<Path> files=Files.list(folder)){
                files.forEach(path->{
                    try{Files.deleteIfExists(path);}
                    catch(IOException ignored){}
                });
            }
            Files.deleteIfExists(folder);
        }
    }
}
