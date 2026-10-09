import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiSelfHostedProviderContractTest {
    private static String file(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void check(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
    public static void main(String[]args)throws Exception{
        String worker=file("server/ai-worker/src/index.js");
        String workflow=file(".github/workflows/deploy-gandalf-ai.yml");
        String cloud=file("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String activity=file("app/src/main/java/com/musa/cad/MainActivity.java");
        check(worker.contains("requested === \"selfhosted\""),"Explicit self-host provider required");
        check(worker.contains("safeSelfHostedEndpoint"),"HTTPS endpoint safety validation");
        check(worker.contains("SELFHOSTED_AI_API_KEY"),"Upstream secret stays at Worker");
        check(worker.contains("validateVisualEvidence"),"Existing consented image validation preserved");
        check(worker.contains("verifySession"),"Existing license session authentication preserved");
        check(worker.contains("data:image/jpeg;base64,"),"Multimodal image sent via chat-compatible API");
        check(worker.contains("parseGeminiChatOutput(data)"),"Chat-compatible response parser shared");
        check(worker.contains("(useSelfHosted || useCloudflareQwen) && !parsedOutput.reply"),
            "Empty AI answer never reported as success");
        check(workflow.contains("provider:"),"Operator explicitly chooses provider");
        check(workflow.contains("default: gemini"),"Existing live Gemini default retained");
        check(workflow.contains("Check self-hosted Qwen readiness"),"Must probe server before deployment");
        check(workflow.contains("MUSACAD_SELFHOSTED_AI_ENDPOINT"),"GitHub secret for Qwen URL");
        check(workflow.contains("MUSACAD_SELFHOSTED_AI_API_KEY"),"GitHub secret for gateway key");
        check(workflow.contains("provider=gemini")==false,"No automatic Gemini fallback");
        check(cloud.contains("public final String provider,model;"),
            "Actual model metadata transported to Android");
        check(activity.contains("Çevrim içi AI motoru: "),
            "AI model identity shown in real engineering report");
        check(!cloud.contains("SELFHOSTED_AI_API_KEY"),
            "No model provider credential in Android source");
        System.out.println("MusaAiSelfHostedProviderContractTest OK");
    }
}
