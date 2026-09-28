import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiCloudIntegrationContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p),StandardCharsets.UTF_8);}
    private static void require(String s,String n,String area){if(!s.contains(n))throw new AssertionError(area+" missing "+n);}
    private static void forbid(String s,String n,String area){if(s.contains(n))throw new AssertionError(area+" must not contain "+n);}
    public static void main(String[]args)throws Exception{
        String gradle=read("app/build.gradle");
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        String cloud=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String session=read("app/src/main/java/com/musa/cad/MusaAiSessionService.java");
        String license=read("app/src/main/java/com/musa/cad/LicenseManager.java");
        String cadJson=read("app/src/main/java/com/musa/cad/MusaAiCadJson.java");

        require(gradle,"MUSACAD_AI_API_URL","cloud endpoint env");
        require(gradle,"MUSACAD_AI_SESSION_URL","session endpoint env");
        require(main,"MusaAiCloudPolicy.shouldUseCloud","explicit cloud routing");
        require(main,"K_CLOUD_CONSENT","cloud privacy consent");
        require(main,"aiExecutor","background network execution");
        require(main,"pendingAiActions","pending edit proposals");
        require(panel,"{\"Gandalf\",\"Gandalf, bu projeyi mekanik açıdan derin analiz et ve raporla\"}","Gandalf quick prompt");
        require(cloud,"Authorization\",\"Bearer \"+session.token","short-lived bearer auth");
        require(cloud,"MusaAiCadJson.build","CAD-JSON request");
        require(session,"LicenseManager.cloudEntitlementProof","license proof exchange");
        require(license,"cloudEntitlementProof","signed entitlement proof");
        require(cadJson,"\"rawDrawingIncluded\":false","no raw drawing policy");
        require(cadJson,"\"automaticEditsAllowed\":false","no automatic edits policy");

        forbid(cloud,"OPENAI_API_KEY","OpenAI secret in Android client");
        forbid(gradle,"sk-proj-","hardcoded API key");
        System.out.println("MusaAiCloudIntegrationContractTest OK");
    }
}
