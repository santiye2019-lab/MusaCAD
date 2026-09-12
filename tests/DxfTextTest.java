import com.musa.cad.DxfText;

public class DxfTextTest {
    private static void check(String expected,String input){
        String actual=DxfText.plain(input);
        if(!expected.equals(actual))throw new AssertionError(actual+" != "+expected);
    }
    public static void main(String[] args){
        check("Bir\nİki", "Bir\\P\\U+0130ki");
        check("Boru Ø50", "{\\C1;Boru %%c50}");
        check("1/2", "\\S1#2;");
        check("{etiket}\\", "\\{etiket\\}\\\\");
        check("Metin", "\\LMetin\\l");
        check("\\Hbroken", "\\Hbroken");
        check("Türkçe ğış", "Türkçe ğış");
        System.out.println("7 DXF text cases passed");
    }
}
