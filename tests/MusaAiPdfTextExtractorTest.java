import com.musa.cad.*;
import java.io.*;
import java.nio.charset.StandardCharsets;

public final class MusaAiPdfTextExtractorTest {
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args)throws Exception{
        String pdf="%PDF-1.4\n"+
            "1 0 obj << /Length 70 >>\nstream\n"+
            "BT /F1 12 Tf (C30 B420C K1 30x60) Tj 0 -14 Td (RADYE TEMEL SDS 1.10) Tj ET\n"+
            "endstream\nendobj\n%%EOF";
        String text=MusaAiPdfTextExtractor.extract(new ByteArrayInputStream(pdf.getBytes(StandardCharsets.ISO_8859_1)));
        has(text,"C30 B420C K1 30x60");
        has(text,"RADYE TEMEL SDS 1.10");

        boolean failed=false;
        try{
            MusaAiPdfTextExtractor.extract(new ByteArrayInputStream("%PDF-1.4\n%%EOF".getBytes(StandardCharsets.ISO_8859_1)));
        }catch(IOException expected){failed=true;}
        if(!failed)throw new AssertionError("image/no-text PDF should not be accepted as readable text");
        System.out.println("MusaAiPdfTextExtractorTest OK");
    }
}