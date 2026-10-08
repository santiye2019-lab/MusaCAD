import java.nio.file.*;

public final class MusaAiDisciplineExpertContractTest {
    private static String read(String p)throws Exception{return Files.readString(Path.of(p));}
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args)throws Exception{
        String main=read("app/src/main/java/com/musa/cad/MainActivity.java");
        String policy=read("app/src/main/java/com/musa/cad/MusaAiCloudPolicy.java");
        String service=read("app/src/main/java/com/musa/cad/MusaAiCloudService.java");
        String worker=read("server/ai-worker/src/index.js");
        String panel=read("app/src/main/java/com/musa/cad/MusaAiPanel.java");
        has(main,"MusaAiDisciplineExpert.analyze");
        has(main,"MusaAiDisciplineExpert.isHelpCommand");
        has(policy,"MusaAiDisciplineExpert.isCloudExpertCommand");
        has(service,"MusaAiDisciplineExpert.cloudProfile");
        has(worker,"structural_openings");
        has(worker,"electrical_power");
        has(worker,"fire_safety_full");
        has(worker,"disciplineExpertInstructions");
        has(panel,"Gandalf\'a yazın veya sesli komut verin");
        has(panel,"host.onPrompt(prompt,previousTurns.toString(),requestReply)");
        System.out.println("MusaAiDisciplineExpertContractTest OK");
    }
}
