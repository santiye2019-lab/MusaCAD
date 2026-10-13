import com.musa.cad.Cad3dRecoveryJournal;
import com.musa.cad.Cad3dSaveCheckpoint;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public final class Cad3dRecoveryJournalTest {
    private static void yes(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static void xyz(float[] values,int vertex,float x,float y,float z){
        int i=vertex*3;
        yes(values[i]==x&&values[i+1]==y&&values[i+2]==z,
            "Bad restored vertex "+vertex);
    }
    private static void fails(Throwing code,String message){
        try{code.run();}catch(IOException expected){return;}
        throw new AssertionError(message);
    }
    interface Throwing{void run()throws IOException;}
    public static void main(String[]args)throws Exception{
        File dir=Files.createTempDirectory("musacad-journal-").toFile();
        File drawing=new File(dir,"source.dxf");
        Files.write(drawing.toPath(),"0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n"
            .getBytes(StandardCharsets.UTF_8));
        float[] opened={1,2,3,4,5,6,7,8,9};
        int[] sourceLines={10,12,14},slots={0,0,0};
        String fingerprint=Cad3dRecoveryJournal.fingerprint(drawing,opened,sourceLines,slots);
        File folder=new File(dir,"private");
        File recovery=Cad3dRecoveryJournal.pathFor(folder,drawing);
        yes(recovery.getName().endsWith(".m3dr"),"Private recovery suffix");
        yes(Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots)==null,
            "No journal should be ignored");

        float[] current=opened.clone();
        current[0]=10;current[4]=55;
        Cad3dSaveCheckpoint checkpoint=new Cad3dSaveCheckpoint(opened);
        Set<Integer> touched=new HashSet<>(Arrays.asList(0,1));
        Cad3dRecoveryJournal.save(recovery,fingerprint,opened,current,sourceLines,slots,
            checkpoint,touched);
        yes(recovery.isFile(),"Journal must be committed");
        Cad3dRecoveryJournal.Snapshot restored=Cad3dRecoveryJournal.load(recovery,
            fingerprint,opened,sourceLines,slots);
        yes(restored.changedVertices()==2,"Sparse vertex count");
        float[] merged=opened.clone();
        restored.restoreCurrent(merged);
        xyz(merged,0,10,2,3);xyz(merged,1,4,55,6);xyz(merged,2,7,8,9);
        yes(Arrays.equals(restored.savedGeometry(opened),opened),
            "Checkpoint must remain original until export");

        // Saving the DXF copy then making a new edit preserves both checkpoints.
        checkpoint.markSaved(current);
        current[2]=33;
        Cad3dRecoveryJournal.save(recovery,fingerprint,opened,current,sourceLines,slots,
            checkpoint,touched);
        restored=Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots);
        float[] saved=restored.savedGeometry(opened);
        xyz(saved,0,10,2,3);xyz(saved,1,4,55,6);
        merged=opened.clone();restored.restoreCurrent(merged);
        xyz(merged,0,10,2,33);xyz(merged,1,4,55,6);
        yes(!Arrays.equals(merged,saved),"Unexported changes survive recovery");

        // Invalid source revision must never replay even if vertex indices still fit.
        File another=new File(dir,"another.dxf");
        Files.write(another.toPath(),Files.readAllBytes(drawing.toPath()));
        String foreign=Cad3dRecoveryJournal.fingerprint(another,opened,sourceLines,slots);
        yes(!fingerprint.equals(foreign),"Canonical source identity fingerprinted");
        fails(()->Cad3dRecoveryJournal.load(recovery,foreign,opened,sourceLines,slots),
            "Different source should fail");
        int[] differentSlots={1,0,0};
        yes(!fingerprint.equals(Cad3dRecoveryJournal.fingerprint(
            drawing,opened,sourceLines,differentSlots)),"Coordinate mapping fingerprinted");
        float[] changedModel=opened.clone();changedModel[6]=71;
        yes(!fingerprint.equals(Cad3dRecoveryJournal.fingerprint(
            drawing,changedModel,sourceLines,slots)),"Exact mesh data fingerprinted");

        // Attempting to save a noneditable source vertex must leave old journal untouched.
        int[] locked={-1,12,14};
        File previous=recovery;
        fails(()->Cad3dRecoveryJournal.save(previous,fingerprint,opened,current,locked,slots,
            checkpoint,touched),"Noneditable vertex must fail");
        yes(Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots)
            .changedVertices()==2,"Bad snapshot must not overwrite old valid journal");

        float[] invalid=current.clone();invalid[0]=Float.NaN;
        fails(()->Cad3dRecoveryJournal.save(recovery,fingerprint,opened,invalid,sourceLines,
            slots,checkpoint,touched),"NaN must be rejected");
        yes(Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots)
            .changedVertices()==2,"Invalid values must preserve previous journal");

        // A damaged file cannot restore partial coordinates (checksum checked first).
        try(RandomAccessFile file=new RandomAccessFile(recovery,"rw")){
            file.seek(file.length()-6);
            file.writeByte(0x7f);
        }
        fails(()->Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots),
            "Corrupted file must fail checksum validation");

        Cad3dRecoveryJournal.save(recovery,fingerprint,opened,current,sourceLines,slots,
            checkpoint,touched);
        try(RandomAccessFile file=new RandomAccessFile(recovery,"rw")){
            file.setLength(file.length()-2);
        }
        fails(()->Cad3dRecoveryJournal.load(recovery,fingerprint,opened,sourceLines,slots),
            "Truncated journal must not restore");

        yes(recovery.delete(),"Private recovery can be deleted on explicit discard");
        System.out.println("3D recovery journal: round trip, last export, source identity, invalid edits, CRC, truncated data passed");
    }
}
