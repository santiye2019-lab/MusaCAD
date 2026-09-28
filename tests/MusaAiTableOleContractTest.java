import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiTableOleContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String parser=read("app/src/main/java/com/musa/cad/DxfParser.java");
        String index=read("app/src/main/java/com/musa/cad/MusaAiDrawingIndex.java");
        String engine=read("app/src/main/java/com/musa/cad/MusaAiTableOleAnalysis.java");
        require(main,"MusaAiTableOleAnalysis.answer","AI table/OLE bridge");
        require(parser,"DxfOleTextExtractor.extract","embedded Office extraction");
        require(parser,"new MusaAiDrawingIndex.OleItem","OLE index bridge");
        require(index,"public static final class OleItem","OLE AI metadata");
        require(engine,"Lejant analizi","legend analysis");
        require(engine,"Tablo analizi","table analysis");
        require(engine,"OLE / gömülü belge özeti","OLE summary");
        System.out.println("MusaAiTableOleContractTest OK");
    }
}
