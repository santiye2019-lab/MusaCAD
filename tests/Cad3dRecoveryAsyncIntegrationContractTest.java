import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** Enforces one FIFO I/O executor for old/new 3D editor instances. */
public final class Cad3dRecoveryAsyncIntegrationContractTest {
    private static void ok(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    public static void main(String[] args)throws Exception{
        String activity=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/Mesh3dActivity.java"),StandardCharsets.UTF_8);
        int pool=activity.indexOf("private static final ExecutorService RECOVERY_IO=");
        int load=activity.indexOf("RECOVERY_IO.execute(()->{",pool+1);
        int delete=activity.indexOf("private void delete3dRecovery(){",load);
        int deleteQueue=activity.indexOf("RECOVERY_IO.execute(()->{",delete);
        int stop=activity.indexOf("@Override protected void onStop(){",deleteQueue);
        int capture=activity.indexOf("Cad3dRecoveryJournal.capture(",stop);
        int stopQueue=activity.indexOf("RECOVERY_IO.execute(()->{",capture);
        int write=activity.indexOf("Cad3dRecoveryJournal.persist(",stopQueue);
        ok(pool>=0&&load>pool&&delete>load&&deleteQueue>delete&&
           stop>deleteQueue&&capture>stop&&stopQueue>capture&&write>stopQueue,
           "3D mesh recovery load, deletion and writes must use one ordered queue");
        ok(activity.contains("Executors.newSingleThreadExecutor("),
           "Only a serialized background executor guarantees rotation ordering");
        ok(activity.contains("final Cad3dRecoveryJournal.Snapshot snapshot="),
           "UI snapshots are immutable before background persistence starts");
        ok(!activity.contains("Cad3dRecoveryJournal.save(recoveryFile,"),
           "Do not serialize/fsync journal directly in Activity.onStop");
        ok(activity.contains("if(recoveryFile!=null") == false ||
           !activity.contains("!recoveryFile.delete()"),
           "Delete must never race on main thread with earlier queued save");
        System.out.println("3D recovery async Android contract: no main thread fsync and ordered operations passed");
    }
}
