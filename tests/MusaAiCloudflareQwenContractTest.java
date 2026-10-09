import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiCloudflareQwenContractTest {
    private static String read(String path)throws Exception{
        return Files.readString(Path.of(path),StandardCharsets.UTF_8);
    }
    private static void check(boolean v,String reason){
        if(!v)throw new AssertionError(reason);
    }
    public static void main(String[]args)throws Exception{
        String worker=read("server/ai-worker/src/index.js");
        String actions=read(".github/workflows/deploy-gandalf-ai.yml");
        String android=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        check(worker.contains("requested === \"cloudflare\""),"Explicit CF provider");
        check(worker.contains("@cf/qwen/qwen3.8-27b"),"Cloudflare vision model name");
        check(worker.contains("CLOUDFLARE_AI_API_TOKEN"),"AI credential never in APK");
        check(worker.contains("^[0-9a-fA-F]{32}$"),"Fixed account ID validation");
        check(worker.contains("verifySession"),"Signed developer/license session preserved");
        check(worker.contains("reasoning_effort: \"low\""),"Bounded reasoning");
        check(worker.contains("max_completion_tokens: Math.max(1536, maxOutputTokens)"),"Visible output room");
        check(worker.contains("data:image/jpeg;base64,"),"JPEG images are supplied");
        check(worker.contains("parsedOutput.reply && !parsedOutput.actions.length"),"Empty reply guard");
        check(worker.contains("Cloudflare Workers AI ücretsiz Neuron kotası"),"Accurate quota errors");
        check(worker.contains("validateVisualEvidence"),"Consent-based bounded image validation");
        check(worker.contains("toChatCompletionsTool"),"Existing approval-only CAD tools");
        check(actions.contains("default: gemini"),"Gemini remains default in production");
        check(actions.contains("Preflight Cloudflare Qwen before changing the production Worker"),
              "Cannot silently replace production before successful inference");
        check(actions.contains("secrets.CLOUDFLARE_AI_API_TOKEN"),"Dedicated AI secret");
        check(actions.contains("AI_PROVIDER = \"cloudflare\""),"Production opt-in mode");
        check(android.contains("public final String provider,model;"),"Android receives provider name");
        check(!android.contains("CLOUDFLARE_AI_API_TOKEN"),"Never bundle API secret in APK");
        System.out.println("MusaAiCloudflareQwenContractTest OK");
    }
}
