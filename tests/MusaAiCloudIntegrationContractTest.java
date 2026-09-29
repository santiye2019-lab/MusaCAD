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
        String expert=read("app/src/main/java/com/musa/cad/MusaAiMechanicalExpert.java");
        String disciplineExpert=read("app/src/main/java/com/musa/cad/MusaAiDisciplineExpert.java");
        String worker=read("server/ai-worker/src/index.js");
        String executor=read("app/src/main/java/com/musa/cad/MusaAiActionExecutor.java");
        String cadView=read("app/src/main/java/com/musa/cad/CadView.java");

        require(gradle,"MUSACAD_AI_API_URL","cloud endpoint env");
        require(gradle,"MUSACAD_AI_SESSION_URL","session endpoint env");
        require(main,"MusaAiCloudPolicy.shouldUseCloud","explicit cloud routing");
        require(main,"K_CLOUD_CONSENT","cloud privacy consent");
        require(main,"aiExecutor","background network execution");
        require(main,"pendingAiActions","pending edit proposals");
        require(panel,"addQuickPromptAuto(activity,quickRow,input,\"Gandalf\"","Gandalf one-tap action");
        require(panel,"\"Gandalf, bu projeyi tüm disiplinlerde derin analiz et ve raporla\"","Gandalf deep-analysis prompt");
        require(cloud,"setRequestProperty(\"Authorization\",\"Bearer \"+session.token)","short-lived bearer auth");
        require(cloud,"MusaAiCadJson.build","CAD-JSON request");
        require(session,"LicenseManager.cloudEntitlementProof","license proof exchange");
        require(license,"cloudEntitlementProof","signed entitlement proof");
        require(cadJson,"rawDrawingIncluded","no raw drawing policy");
        require(cadJson,"automaticEditsAllowed","no automatic edits policy");
        require(cadJson,"editActionsRequireUserApproval","user approval policy");
        require(main,"MusaAiMechanicalExpert.analyze","local MEKAI expert routing");
        require(main,"MusaAiDisciplineExpert.analyze","local discipline expert routing");
        require(cloud,"body.put(\"expertProfile\",expertProfile)","trusted expert profile request");
        require(expert,"GMEKAI","GMEKAI command surface");
        require(disciplineExpert,"GSTATIKAI_FULL","multi-discipline expert command surface");
        require(worker,"normalizeExpertProfile","server expert-profile allowlist");
        require(worker,"mechanicalExpertInstructions","server mechanical expert instructions");
        require(worker,"disciplineExpertInstructions","server discipline expert instructions");
        require(main,"MusaAiSessionService.developerCached()","developer-only edit application gate");
        require(main,"Gandalf Developer • Önizleme","explicit Gandalf preview dialog");
        require(main,"setPositiveButton(\"UYGULA\"","explicit user approval button");
        require(main,"undoLastGandalfBatch","protected Gandalf batch undo");
        require(executor,"MAX_ACTIONS=50","bounded proposal batch");
        require(executor,"cad.restoreCapturedSessionState(before)","atomic rollback");
        require(executor,"case \"cad_delete_entity\"","allowlisted destructive action");
        require(executor,"case \"cad_add_line\"","allowlisted drawing action");
        require(cadView,"applyAiMoveSource","sourceId move bridge");
        require(cadView,"applyAiReplaceTextSource","sourceId text bridge");
        require(cadView,"aiLayerExists","existing-layer guard");
        require(cadView,"applyAiAddPolyline","approved polyline bridge");
        require(cadView,"applyAiOffsetSource","approved offset bridge");
        require(cadView,"applyAiTrimLine","deterministic TRIM bridge");
        require(cadView,"applyAiExtendLine","deterministic EXTEND bridge");
        require(cadView,"applyAiContinuePath","approved path continuation bridge");
        require(cadView,"applyAiInsertLibraryBlock","approved mechanical block bridge");
        require(executor,"case \"cad_add_polyline\"","polyline proposal executor");
        require(executor,"case \"cad_trim_line\"","TRIM proposal executor");
        require(executor,"case \"cad_insert_mechanical_block\"","mechanical block proposal executor");
        require(worker,"\"cad_continue_path\"","advanced path tool");
        require(worker,"\"cad_add_pipe_note\"","mechanical note tool");

        forbid(cloud,"OPENAI_API_KEY","OpenAI secret in Android client");
        forbid(gradle,"sk-proj-","hardcoded API key");
        System.out.println("MusaAiCloudIntegrationContractTest OK");
    }
}
