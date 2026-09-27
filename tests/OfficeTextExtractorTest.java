import com.musa.cad.OfficeTextExtractor;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.zip.*;

public final class OfficeTextExtractorTest {
    private static byte[] zip(String[][] entries)throws Exception{
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        try(ZipOutputStream out=new ZipOutputStream(bytes)){
            for(String[] e:entries){out.putNextEntry(new ZipEntry(e[0]));out.write(e[1].getBytes(StandardCharsets.UTF_8));out.closeEntry();}
        }
        return bytes.toByteArray();
    }
    private static void contains(String text,String expected){if(text==null||!text.contains(expected))throw new AssertionError("Missing: "+expected+" in "+text);}

    public static void main(String[] args)throws Exception{
        String doc="<w:document xmlns:w=\"urn:w\"><w:body><w:p><w:r><w:t>Merhaba</w:t></w:r></w:p><w:p><w:r><w:t>MusaCAD</w:t></w:r></w:p></w:body></w:document>";
        String docText=OfficeTextExtractor.extract(new ByteArrayInputStream(zip(new String[][]{{"word/document.xml",doc}})),"rapor.docx",null);
        contains(docText,"Merhaba");contains(docText,"MusaCAD");

        String strings="<sst xmlns=\"urn:x\"><si><t>Pompa</t></si></sst>";
        String sheet="<worksheet xmlns=\"urn:x\"><sheetData><row><c r=\"A1\" t=\"s\"><v>0</v></c><c r=\"B1\"><v>5.5</v></c></row></sheetData></worksheet>";
        String xlsx=OfficeTextExtractor.extract(new ByteArrayInputStream(zip(new String[][]{{"xl/sharedStrings.xml",strings},{"xl/worksheets/sheet1.xml",sheet}})),"metraj.xlsx",null);
        contains(xlsx,"A1");contains(xlsx,"Pompa");contains(xlsx,"B1");contains(xlsx,"5.5");

        String slide="<p:sld xmlns:p=\"urn:p\" xmlns:a=\"urn:a\"><p:cSld><a:p><a:r><a:t>Sunum metni</a:t></a:r></a:p></p:cSld></p:sld>";
        String ppt=OfficeTextExtractor.extract(new ByteArrayInputStream(zip(new String[][]{{"ppt/slides/slide1.xml",slide}})),"sunum.pptx",null);
        contains(ppt,"Slayt 1");contains(ppt,"Sunum metni");

        System.out.println("OfficeTextExtractorTest OK");
    }
}
