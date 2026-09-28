import java.nio.file.*;

public final class MusaAiReportEvidenceContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String cad=read("app/src/main/java/com/musa/cad/CadView.java");
        String out=read("app/src/main/java/com/musa/cad/MusaAiReportExport.java");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        has(cad,"aiEvidenceSnapshot");
        has(cad,"reportEvidenceExporting");
        has(out,"word/media/evidence.png");
        has(out,"rIdEvidence");
        has(out,"Çizim kanıt görüntüsü");
        has(main,"lastAiReportSourceIds");
        has(main,"cad.aiEvidenceSnapshot(lastAiReportSourceIds)");
        has(main,"kanıt görüntüsü rapora eklendi");
        System.out.println("MusaAiReportEvidenceContractTest OK");
    }
}
