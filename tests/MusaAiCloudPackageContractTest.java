import java.nio.file.*;

public final class MusaAiCloudPackageContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String service=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String policy=read("app/src/main/java/com/musa/cad/MusaAiCloudPolicy.java");
        String worker=read("server/ai-worker/src/index.js");
        has(main,"MusaAiCloudPolicy.shouldUseProjectPackage");
        has(main,"K_CLOUD_PACKAGE_CONSENT");
        has(main,"packageMode?K_CLOUD_PACKAGE_CONSENT:");
        has(main,"hybridVisual?K_CLOUD_VISUAL_CONSENT:K_CLOUD_CONSENT");
        has(main,"boolean hybridVisual=(MusaAiAnalysisIntent.isReview(raw)||");
        has(main,"MusaAiAnalysisIntent.isCombinedVisualReview(raw))&&!packageMode;");
        has(main,"MusaAiCloudService.analyzePackage");
        has(main,"Gandalf Proje Paketi");
        has(service,"MusaAiCadPackageJson.build");
        has(service,"body.put(\"cadPackage\"");
        has(policy,"shouldUseProjectPackage");
        has(worker,"musacad-cad-package/v1");
        has(worker,"Package mode is deliberately read-only");
        has(worker,"allowEditProposals && !packageMode");
        has(worker,"validCadPackage");
        has(worker,"packageMode");
        System.out.println("MusaAiCloudPackageContractTest OK");
    }
}
