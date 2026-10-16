import com.musa.cad.DxfStream;
import com.musa.cad.DxfBlocks;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/** Regression: a DXF EOF must never replay previously read bytes. */
public final class DxfStreamEofTest {
    private static void check(boolean good) {
        if(!good)throw new AssertionError("DXF EOF replay/seek contract");
    }
    private static void exercise(String end) throws Exception {
        File file=File.createTempFile("musacad-stream-eof-", ".dxf");
        try {
            String content="0\nSECTION\n2\nENTITIES\n"+
                "0\nLINE\n5\nA1\n10\n0\n20\n0\n11\n1\n21\n0\n"+
                "0\nENDSEC\n0\nEOF"+end;
            Files.writeString(file.toPath(),content,StandardCharsets.UTF_8);
            try(DxfStream stream=new DxfStream(file,StandardCharsets.UTF_8)) {
                String[] types={"SECTION","LINE","ENDSEC","EOF"};
                for(String type:types) {
                    DxfBlocks.Record record=stream.next();
                    check(record!=null&&type.equals(record.type));
                }
                for(int i=0;i<5;i++)check(stream.next()==null);
                // An explicit seek may reinitialize the reader, even after EOF.
                stream.seek(0,0);
                for(String type:types)check(type.equals(stream.next().type));
                check(stream.next()==null);
            }
        } finally {Files.deleteIfExists(file.toPath());}
    }
    public static void main(String[] args) throws Exception {
        exercise("\n");
        exercise("");
        exercise("\r\n");
        System.out.println("DXF streaming EOF no-phantom-entities and seek PASS");
    }
}
