import java.nio.file.*;

public final class MusaAiProjectPackageContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        has(main,"currentAiProjectPackageDrawings");
        has(main,"MusaAiProjectPackage.asksPackageReview");
        has(main,"MusaAiProjectPackage.generate");
        has(main,"p.boqModel");
        has(main,"lastAiReportSourceIds=Collections.emptyList()");
        has(main,"openVectorProjectCount");
        has(panel,"host.onPrompt(prompt,previousTurns.toString(),requestReply)");
        int pkg=main.indexOf("MusaAiProjectPackage.asksPackageReview");
        int single=main.indexOf("MusaAiDetailedReport.asksDetailedReport");
        if(pkg<0||single<0||pkg>single)throw new AssertionError("package audit must route before single-file detailed report");
        System.out.println("MusaAiProjectPackageContractTest OK");
    }
}
