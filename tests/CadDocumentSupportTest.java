import com.musa.cad.CadDocumentSupport;

public final class CadDocumentSupportTest {
    private static void eq(Object expected,Object actual){if(!expected.equals(actual))throw new AssertionError("Expected "+expected+" but got "+actual);}
    private static void yes(boolean value){if(!value)throw new AssertionError("Expected true");}
    private static void no(boolean value){if(value)throw new AssertionError("Expected false");}

    public static void main(String[] args){
        eq(CadDocumentSupport.Kind.PDF,CadDocumentSupport.kind("plan.pdf",null));
        eq(CadDocumentSupport.Kind.DOCX,CadDocumentSupport.kind("rapor.DOCX","application/octet-stream"));
        eq(CadDocumentSupport.Kind.XLSX,CadDocumentSupport.kind("metraj.xlsx",null));
        eq(CadDocumentSupport.Kind.PPTX,CadDocumentSupport.kind("sunum.pptx",null));
        eq(CadDocumentSupport.Kind.LEGACY_WORD,CadDocumentSupport.kind("eski.doc",null));
        yes(CadDocumentSupport.readableInApp("rapor.docx",null));
        yes(CadDocumentSupport.readableInApp("metraj.xlsx",null));
        yes(CadDocumentSupport.readableInApp("sunum.pptx",null));
        yes(CadDocumentSupport.readableInApp("dosya.pdf",null));
        no(CadDocumentSupport.readableInApp("eski.xls",null));
        no(CadDocumentSupport.isDocument("cizim.dwg","application/dwg"));
        System.out.println("CadDocumentSupportTest OK");
    }
}
