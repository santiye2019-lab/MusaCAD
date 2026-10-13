import com.musa.cad.Cad3dRecoveryJournal;
import com.musa.cad.Cad3dSaveCheckpoint;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

/** Regression: UI snapshot is immutable, FIFO I/O survives rotation/discard races. */
public final class Cad3dRecoveryAsyncTest {
    private static void ok(boolean value,String explanation){
        if(!value)throw new AssertionError(explanation);
    }
    public static void main(String[] args)throws Exception{
        File dir=Files.createTempDirectory("musacad-recovery-async-").toFile();
        File source=new File(dir,"project.dxf");
        Files.write(source.toPath(),"0\nEOF\n".getBytes(StandardCharsets.UTF_8));
        float[] original={1,2,3,4,5,6};
        float[] current=original.clone();
        int[] sourceLines={2,4},slots={0,0};
        Set<Integer> edited=new HashSet<>(Arrays.asList(0));
        Cad3dSaveCheckpoint checkpoint=new Cad3dSaveCheckpoint(original);
        String fingerprint=Cad3dRecoveryJournal.fingerprint(source,original,sourceLines,slots);
        File destination=Cad3dRecoveryJournal.pathFor(new File(dir,"app-private"),source);

        current[0]=42;
        Cad3dRecoveryJournal.Snapshot captured=Cad3dRecoveryJournal.capture(
            original,current,sourceLines,slots,checkpoint,edited);
        // Simulate resumed UI editing and saved state changing before worker starts.
        current[0]=200;checkpoint.markSaved(current);edited.clear();

        ExecutorService queue=Executors.newSingleThreadExecutor();
        try{
            Future<?> first=queue.submit(()->{
                try{Cad3dRecoveryJournal.persist(destination,fingerprint,2,captured);}
                catch(Exception e){throw new RuntimeException(e);}
            });
            // Replacement Activity must read after previous Activity checkpoint write.
            Future<Cad3dRecoveryJournal.Snapshot> read=queue.submit(()->{
                try{return Cad3dRecoveryJournal.load(
                    destination,fingerprint,original,sourceLines,slots);}
                catch(Exception e){throw new RuntimeException(e);}
            });
            first.get();
            Cad3dRecoveryJournal.Snapshot restored=read.get();
            ok(restored!=null,"Queued load must find prior completed write");
            float[] model=original.clone();restored.restoreCurrent(model);
            ok(model[0]==42,"Queued file must contain captured immutable coordinates");
            ok(Arrays.equals(restored.savedGeometry(original),original),
                "Captured saved checkpoint must not change with later user edits");

            Future<?> delete=queue.submit(()->{
                if(destination.exists()&&!destination.delete())
                    throw new AssertionError("Failed to discard journal");
            });
            Future<Cad3dRecoveryJournal.Snapshot> absent=queue.submit(()->{
                try{return Cad3dRecoveryJournal.load(
                    destination,fingerprint,original,sourceLines,slots);}
                catch(Exception e){throw new RuntimeException(e);}
            });
            delete.get();
            ok(absent.get()==null,"Ordered discard cannot be undone by a stale writer");

            // Later edit after a discard must be intentionally saved as a new journal.
            edited.add(0);
            Cad3dRecoveryJournal.Snapshot newer=Cad3dRecoveryJournal.capture(
                original,current,sourceLines,slots,checkpoint,edited);
            Future<?> second=queue.submit(()->{
                try{Cad3dRecoveryJournal.persist(destination,fingerprint,2,newer);}
                catch(Exception e){throw new RuntimeException(e);}
            });
            second.get();
            restored=Cad3dRecoveryJournal.load(
                destination,fingerprint,original,sourceLines,slots);
            model=original.clone();restored.restoreCurrent(model);
            ok(model[0]==200,"Newer queued save must persist new session edits");
            // Large edit sets should not require sorting/extra cloning on the UI.
            // A deliberately reverse-order capture must round-trip in sorted disk order
            // without mixing the coordinate triples of adjacent vertices.
            float[] openedFour={0,1,2,3,4,5,6,7,8,9,10,11};
            float[] changedFour=openedFour.clone();
            for(int v=0;v<4;v++)changedFour[v*3]=100+v;
            int[] linesFour={2,4,6,8},slotsFour={0,0,0,0};
            Set<Integer> reverse=new LinkedHashSet<>(Arrays.asList(3,2,1,0));
            Cad3dRecoveryJournal.Snapshot unsorted=Cad3dRecoveryJournal.capture(
                openedFour,changedFour,linesFour,slotsFour,
                new Cad3dSaveCheckpoint(openedFour),reverse);
            ok(Arrays.equals(unsorted.indices(),new int[]{3,2,1,0}),
                "UI capture must avoid sorting touched vertices");
            changedFour[9]=-999; // After capture, editor can keep moving vertex 3.
            String fourFingerprint=Cad3dRecoveryJournal.fingerprint(
                source,openedFour,linesFour,slotsFour);
            File fourJournal=new File(dir,"unordered-vertices.m3dr");
            Future<?> saveUnsorted=queue.submit(()->{
                try{Cad3dRecoveryJournal.persist(fourJournal,fourFingerprint,4,unsorted);}
                catch(Exception e){throw new RuntimeException(e);}
            });
            saveUnsorted.get();
            Cad3dRecoveryJournal.Snapshot ordered=Cad3dRecoveryJournal.load(
                fourJournal,fourFingerprint,openedFour,linesFour,slotsFour);
            ok(Arrays.equals(ordered.indices(),new int[]{0,1,2,3}),
                "Disk format must retain increasing vertex order for safe recovery");
            float[] restoredFour=openedFour.clone();
            ordered.restoreCurrent(restoredFour);
            for(int v=0;v<4;v++)
                ok(restoredFour[v*3]==100+v,
                    "Sorted persistence must preserve each vertex's own XYZ");
            ok(Arrays.equals(ordered.savedGeometry(openedFour),openedFour),
                "Reordered checkpoint coordinates must remain aligned");
        }finally{queue.shutdown();}
        System.out.println("3D async recovery: immutable capture, unordered vertex save, serialized I/O passed");
    }
}
