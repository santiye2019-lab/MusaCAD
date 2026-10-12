import com.musa.cad.CadDxfAttributePatch;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public final class CadDxfAttributePatchTest {
    private static final String SOURCE =
        "0\nSECTION\n2\nENTITIES\n"
      + "0\nINSERT\n5\nA1\n2\nPUMP\n66\n1\n"
      + "0\nATTRIB\n5\n10\n2\nCODE\n1\nOLD\n8\nMECHANICAL\n"
      + "0\nATTRIB\n5\n11\n2\nDN\n1\n50\n"
      + "0\nSEQEND\n5\n12\n"
      + "0\nINSERT\n5\nB2\n2\nPUMP\n66\n1\n"
      + "0\nATTRIB\n5\n13\n2\nCODE\n1\nUNTOUCHED\n"
      + "0\nSEQEND\n5\n14\n"
      + "0\nTEXT\n5\n15\n1\nOLD\n"
      + "0\nENDSEC\n0\nEOF\n";
    private static void assertTrue(boolean ok,String message) {
        if (!ok) throw new AssertionError(message);
    }
    private static void failsWithoutBytes(File source, CadDxfAttributePatch.Change... changes) throws Exception {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        boolean failed = false;
        try {
            CadDxfAttributePatch.write(source, output, StandardCharsets.UTF_8, Arrays.asList(changes));
        } catch (IOException expected) { failed = true; }
        assertTrue(failed,"must reject missing, ambiguous or damaged patch");
        assertTrue(output.size() == 0,"preflight must not write to destination on failure");
    }
    public static void main(String[]args)throws Exception {
        File source=File.createTempFile("musacad-attribute-source-", ".dxf");
        try {
            Files.writeString(source.toPath(), SOURCE, StandardCharsets.UTF_8);
            byte[] original=Files.readAllBytes(source.toPath());
            ByteArrayOutputStream out=new ByteArrayOutputStream();
            int n=CadDxfAttributePatch.write(source,out,StandardCharsets.UTF_8,Arrays.asList(
                new CadDxfAttributePatch.Change("a1","code","ÇALIŞIYOR"),
                new CadDxfAttributePatch.Change("A1","DN","65")));
            assertTrue(n==2,"two values edited");
            String expected=SOURCE.replace("2\nCODE\n1\nOLD\n8\nMECHANICAL","2\nCODE\n1\nÇALIŞIYOR\n8\nMECHANICAL")
                                  .replace("2\nDN\n1\n50\n","2\nDN\n1\n65\n");
            assertTrue(expected.equals(out.toString(StandardCharsets.UTF_8)),"exact expected DXF tags");
            assertTrue(Arrays.equals(original,Files.readAllBytes(source.toPath())),"source file must remain untouched");
            assertTrue(out.toString(StandardCharsets.UTF_8).contains("2\nCODE\n1\nUNTOUCHED\n"),"other INSERT preserved");
            assertTrue(out.toString(StandardCharsets.UTF_8).contains("0\nTEXT\n5\n15\n1\nOLD\n"),"unrelated text preserved");
            ByteArrayOutputStream legacy=new ByteArrayOutputStream();
            boolean encodingRejected=false;
            try {
                CadDxfAttributePatch.write(source,legacy,
                    java.nio.charset.Charset.forName("windows-1252"),
                    Collections.singletonList(new CadDxfAttributePatch.Change("A1","CODE","Türkçe 😀")));
            } catch(IOException expectedError) { encodingRejected=true; }
            assertTrue(encodingRejected&&legacy.size()==0,"unmappable ANSI characters rejected before publish");
            failsWithoutBytes(source,new CadDxfAttributePatch.Change("MISSING","CODE","X"));
            failsWithoutBytes(source,new CadDxfAttributePatch.Change("A1","MISSING","X"));
            failsWithoutBytes(source,new CadDxfAttributePatch.Change("A1","CODE","X"),
                                       new CadDxfAttributePatch.Change("a1","code","Y"));
            boolean rejected=false;
            try { new CadDxfAttributePatch.Change("A1","CODE","bad\n0\nLINE"); }
            catch (IllegalArgumentException expectedError){rejected=true;}
            assertTrue(rejected,"tag injection must be rejected");
            Files.writeString(source.toPath(),SOURCE.replace("0\nENDSEC\n0\nEOF\n","0\nENDSEC\n"),StandardCharsets.UTF_8);
            failsWithoutBytes(source,new CadDxfAttributePatch.Change("A1","CODE","X"));
            Files.writeString(source.toPath(),SOURCE.replace("2\nDN\n1\n50\n",
                "2\nDN\n1\n50\n0\nATTRIB\n5\n17\n2\nDN\n1\n100\n"),StandardCharsets.UTF_8);
            failsWithoutBytes(source,new CadDxfAttributePatch.Change("A1","DN","65"));
        } finally {
            Files.deleteIfExists(source.toPath());
        }
        System.out.println("DXF INSERT/ATTRIB handle-tag safe preflight and Unicode roundtrip passed");
    }
}
