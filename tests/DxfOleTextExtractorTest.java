import com.musa.cad.DxfOleTextExtractor;
import java.io.*;
import java.util.zip.*;

public final class DxfOleTextExtractorTest {
    private static byte[] workbook()throws Exception{
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        try(ZipOutputStream zip=new ZipOutputStream(bytes)){
            zip.putNextEntry(new ZipEntry("xl/sharedStrings.xml"));
            zip.write(("<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"+
                "<si><t>MALZEME</t></si><si><t>VANA</t></si></sst>").getBytes("UTF-8"));
            zip.closeEntry();
            zip.putNextEntry(new ZipEntry("xl/worksheets/sheet1.xml"));
            zip.write(("<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData><row r=\"1\">"+
                "<c r=\"A1\" t=\"s\"><v>0</v></c><c r=\"B1\" t=\"s\"><v>1</v></c>"+
                "<c r=\"C1\"><v>12</v></c></row></sheetData></worksheet>").getBytes("UTF-8"));
            zip.closeEntry();
        }
        byte[] z=bytes.toByteArray(),out=new byte[z.length+7];
        for(int i=0;i<7;i++)out[i]=(byte)(i+1);
        System.arraycopy(z,0,out,7,z.length);return out;
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n+" in "+s);}
    public static void main(String[]args)throws Exception{
        DxfOleTextExtractor.Result x=DxfOleTextExtractor.extract(workbook(),"EXCEL");
        if(!x.structured)throw new AssertionError("xlsx not structured");
        if(x.sheetCount!=1)throw new AssertionError("sheet "+x.sheetCount);
        if(x.cellCount!=3)throw new AssertionError("cells "+x.cellCount);
        has(x.text,"A1\tMALZEME");has(x.text,"B1\tVANA");has(x.text,"C1\t12");

        byte[] legacy="xxxx Microsoft Excel VANA DN100 ADET 12 yyyy".getBytes("ISO-8859-1");
        DxfOleTextExtractor.Result y=DxfOleTextExtractor.extract(legacy,"EXCEL");
        if(y.structured)throw new AssertionError("legacy marked structured");
        has(y.text,"Microsoft Excel VANA DN100 ADET 12");
        System.out.println("DxfOleTextExtractorTest OK");
    }
}
