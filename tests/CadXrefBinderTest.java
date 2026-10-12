import com.musa.cad.CadXrefBinder;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class CadXrefBinderTest {
    private static final String PARENT=
        "0\nSECTION\n2\nBLOCKS\n"+
        "0\nBLOCK\n2\nEXT_TEST\n70\n4\n1\nexternal.dwg\n10\n0\n20\n0\n"+
        "0\nENDBLK\n0\nENDSEC\n"+
        "0\nSECTION\n2\nENTITIES\n"+
        "0\nINSERT\n2\nEXT_TEST\n10\n100\n20\n200\n"+
        "0\nENDSEC\n0\nEOF\n";
    private static final String CHILD=
        "0\nSECTION\n2\nENTITIES\n"+
        "0\nLINE\n8\n0\n10\n0\n20\n0\n11\n10\n21\n20\n"+
        "0\nCIRCLE\n8\n0\n10\n4\n20\n5\n40\n2\n"+
        "0\nENDSEC\n0\nEOF\n";
    private static void check(boolean result,String message){
        if(!result)throw new AssertionError(message);
    }
    private static void reject(Path directory,String payload,String reason)throws Exception{
        File parent=directory.resolve("parent.dxf").toFile();
        File child=directory.resolve("bad.dxf").toFile();
        File destination=directory.resolve("rejected.dxf").toFile();
        Files.writeString(child.toPath(),payload,StandardCharsets.UTF_8);
        boolean fail=false;
        try{CadXrefBinder.bind(parent,child,"EXT_TEST",destination);}
        catch(java.io.IOException expected){fail=true;}
        check(fail,reason);
        check(!destination.exists(),"failed bind wrote a partial copy");
    }
    public static void main(String[]args)throws Exception{
        Path folder=Files.createTempDirectory("cad-xref-safe");
        try{
            File parent=folder.resolve("parent.dxf").toFile();
            File source=folder.resolve("child.dxf").toFile();
            File output=folder.resolve("bound.dxf").toFile();
            Files.writeString(parent.toPath(),PARENT,StandardCharsets.UTF_8);
            Files.writeString(source.toPath(),CHILD,StandardCharsets.UTF_8);
            CadXrefBinder.bind(parent,source,"EXT_TEST",output);
            String bound=Files.readString(output.toPath(),StandardCharsets.UTF_8);
            check(bound.contains("0\nLINE\n")&&bound.contains("0\nCIRCLE\n"),
                "real XREF entities not embedded in bound BLOCK");
            check(bound.contains("70\n0\n"),"external-reference flag remains on bound BLOCK");
            check(!bound.contains("external.dwg"),"XREF path not removed in bound copy");
            check(PARENT.equals(Files.readString(parent.toPath(),StandardCharsets.UTF_8)),
                "parent changed");
            check(CHILD.equals(Files.readString(source.toPath(),StandardCharsets.UTF_8)),
                "child changed");
            reject(folder,CHILD.replace("8\n0","8\nSPECIAL_MEP"),"nonzero custom layer accepted");
            reject(folder,CHILD.replace("0\nLINE","0\nINSERT\n2\nSUBBLOCK\n0\nLINE"),
                "nested source blocks accepted");
            reject(folder,CHILD.replace("8\n0\n10\n0","8\n0\n340\nAA\n10\n0"),
                "external group handle reference accepted");
            boolean overwrite=false;
            try{CadXrefBinder.bind(parent,source,"EXT_TEST",parent);}
            catch(java.io.IOException expected){overwrite=true;}
            check(overwrite,"original overwritten");
            System.out.println("Safe user-selected XREF binding, partial rejection and original safety passed");
        }finally{
            try(java.util.stream.Stream<Path> paths=Files.walk(folder)){
                paths.sorted(java.util.Comparator.reverseOrder()).forEach(x->{
                    try{Files.deleteIfExists(x);}catch(Exception ignored){}
                });
            }
        }
    }
}
