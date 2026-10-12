import com.musa.cad.CadBlockAttributes;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public final class CadBlockAttributesTest {
    private static final String SOURCE=
        "0\nSECTION\n2\nENTITIES\n"+
        "0\nINSERT\n2\nMEP_EQUIPMENT\n66\n1\n10\n20\n20\n30\n"+
        "0\nATTRIB\n2\nCIHAZ_NO\n1\nAHU-01\n5\nA1\n10\n21\n20\n32\n"+
        "0\nATTRIB\n2\nKAPASITE\n1\n13500 m3/h\n5\nA2\n10\n21\n20\n33\n"+
        "0\nSEQEND\n"+
        "0\nINSERT\n2\nPUMP\n66\n1\n"+
        "0\nATTRIB\n2\nPOMPA_NO\n1\nP-01\n5\nB1\n"+
        "0\nSEQEND\n0\nENDSEC\n0\nEOF\n";
    private static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    public static void main(String[]args)throws Exception {
        Path folder=Files.createTempDirectory("cad-attrs");
        File source=folder.resolve("original.dxf").toFile();
        File output=folder.resolve("edited.dxf").toFile();
        try{
            Files.writeString(source.toPath(),SOURCE,StandardCharsets.UTF_8);
            List<CadBlockAttributes.Attribute> items=CadBlockAttributes.list(source);
            check(items.size()==3,"3 ordinary INSERT attribute values expected");
            check(items.get(1).tag.equals("KAPASITE")&&items.get(1).value.equals("13500 m3/h"),
                "capacity tag/value lost");
            CadBlockAttributes.replace(source,output,1,"16000 m3/h");
            String exported=Files.readString(output.toPath(),StandardCharsets.UTF_8);
            check(exported.contains("1\n16000 m3/h\n"),"replaced attribute missing");
            check(exported.contains("1\nAHU-01\n")&&exported.contains("1\nP-01\n"),
                "other block attributes modified");
            check(SOURCE.equals(Files.readString(source.toPath(),StandardCharsets.UTF_8)),
                "original source ATTRIB modified");
            boolean rejected=false;
            try{CadBlockAttributes.replace(source,output,2,"unsafe\ninjection");}
            catch(java.io.IOException expected){rejected=true;}
            check(rejected,"multiline DXF tag injection accepted");
            rejected=false;
            try{CadBlockAttributes.replace(source,source,1,"DO NOT TRUNCATE");}
            catch(java.io.IOException expected){rejected=true;}
            check(rejected,"direct source overwrite accepted");
            System.out.println("DXF INSERT/ATTRIB editing, other attributes, source preservation passed");
        }finally{
            Files.deleteIfExists(output.toPath());
            Files.deleteIfExists(source.toPath());
            Files.deleteIfExists(folder);
        }
    }
}
