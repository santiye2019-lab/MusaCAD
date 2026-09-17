import com.musa.cad.DxfText;

public class DxfTextTest {
    private static int cases;
    private static void check(String expected,String input){
        String actual=DxfText.plain(input);cases++;
        if(!expected.equals(actual))throw new AssertionError(actual+" != "+expected);
    }
    public static void main(String[] args){
        check("Bir\nİki", "Bir\\P\\U+0130ki");
        check("Boru Ø50", "{\\C1;Boru %%c50}");
        check("Boru Ø50 ±2°", "Boru %%C50 %%P2%%D");
        check("1/2", "\\S1#2;");
        check("+0.010/-0.000", "\\S+0.010^-0.000;");
        check("{etiket}\\", "\\{etiket\\}\\\\");
        check("Metin", "\\LMetin\\l");
        check("\\Hbroken", "\\Hbroken");
        check("ABC", "%%065%%066%%067");
        check("% tamam", "%%% tamam");
        check("Alt Ust", "%%uAlt%%u %%oUst%%o");
        check("Türkçe ğış", "Türkçe ğış");
        System.out.println(cases+" DXF text cases passed");
    }
}
