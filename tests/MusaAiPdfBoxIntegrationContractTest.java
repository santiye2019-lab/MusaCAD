import java.nio.file.*;

public final class MusaAiPdfBoxIntegrationContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String gradle=read("app/build.gradle");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String pdf=read("app/src/main/java/com/musa/cad/MusaAiPdfBoxTextExtractor.java");
        has(gradle,"com.tom-roush:pdfbox-android:2.0.27.0");
        has(main,"MusaAiPdfBoxTextExtractor.extract(getApplicationContext(),in)");
        has(pdf,"PDFBoxResourceLoader.init");
        has(pdf,"PDDocument.load(pdf)");
        has(pdf,"PDFTextStripper");
        has(pdf,"setSortByPosition(true)");
        has(pdf,"MAX_PAGES=2500");
        has(pdf,"MusaAiPdfTextExtractor.extract(new ByteArrayInputStream(pdf))");
        has(pdf,"does not perform OCR");
        System.out.println("MusaAiPdfBoxIntegrationContractTest OK");
    }
}
