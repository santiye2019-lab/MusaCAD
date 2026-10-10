import com.musa.cad.MusaAiVisionTimeBudget;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class MusaAiVisionTimeBudgetTest {
    static void assertTrue(boolean value,String msg) {
        if(!value)throw new AssertionError(msg);
    }
    public static void main(String[] args)throws Exception {
        long model=MusaAiVisionTimeBudget.MODEL_READ_LIMIT_MS;
        long render=MusaAiVisionTimeBudget.RENDER_BATCH_LIMIT_MS;
        long notice=MusaAiVisionTimeBudget.IDLE_NOTICE_MS;
        long start=MusaAiVisionTimeBudget.SWEEP_START_DEADLINE_MS;
        long hard=MusaAiVisionTimeBudget.PANEL_HARD_DEADLINE_MS;
        assertTrue(notice>model,"Do not discard a normal 90-second Qwen response");
        assertTrue(notice>=model+render,"Idle progress waits for model AND render overhead");
        assertTrue(start>=3*model+3*render,"Allow three real multi-image groups");
        assertTrue(hard>=start+2*model+render,
            "Allow a last bounded call with one compact network retry to finish");
        assertTrue(MusaAiVisionTimeBudget.mayStartNextBatch(0),"Initial batch");
        assertTrue(MusaAiVisionTimeBudget.mayStartNextBatch(120000),"Next group after slow first call");
        assertTrue(MusaAiVisionTimeBudget.mayStartNextBatch(start-1),"Last safe start");
        assertTrue(!MusaAiVisionTimeBudget.mayStartNextBatch(start),"No work after sweep cut-off");
        assertTrue(!MusaAiVisionTimeBudget.mayRetryBatch(start),"No retry after cut-off");
        assertTrue(!MusaAiVisionTimeBudget.mayStartNextBatch(-1),"Reject invalid time");

        String panel=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MusaAiPanel.java"),StandardCharsets.UTF_8);
        assertTrue(panel.contains("timeoutHandler.postDelayed(timeout,MusaAiVisionTimeBudget.PANEL_HARD_DEADLINE_MS)"),
            "Hard deadline must stay fixed");
        assertTrue(panel.contains("timeoutHandler.postDelayed(idleNotice,MusaAiVisionTimeBudget.IDLE_NOTICE_MS)"),
            "Soft idle notice before hard timeout");
        assertTrue(panel.contains("timeoutHandler.removeCallbacks(idleNotice)"),
            "Do not keep old idle callbacks after final result");
        assertTrue(!panel.contains("timeoutHandler.postDelayed(timeout,75_000L)"),
            "No early terminal timer after only 75 seconds");
        String main=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        assertTrue(main.contains("MusaAiVisionTimeBudget.mayStartNextBatch("),
            "Sweep must use shared safe time budget");
        assertTrue(main.contains("MusaAiVisionTimeBudget.mayRetryBatch("),
            "Retry must use shared safe time budget");
        assertTrue(main.contains("acceptedTiles==total"),"Never invent 9/9");
        System.out.println("MusaAiVisionTimeBudgetTest OK");
    }
}
