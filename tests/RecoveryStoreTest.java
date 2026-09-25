import com.musa.cad.RecoveryStore;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

public class RecoveryStoreTest {
    public static void main(String[] args) throws Exception {
        File dir=Files.createTempDirectory("musacad-recovery-test").toFile();
        try{
            String id=RecoveryStore.idFor("content://drawings/plan.dxf");
            if(id.length()!=24)throw new AssertionError("stable id length");
            if(!id.equals(RecoveryStore.idFor("content://drawings/plan.dxf")))throw new AssertionError("stable id");

            RecoveryStore.write(dir,id,"Plan A.dxf","content://drawings/plan.dxf",101L,1000L,
                out->out.write("0\nSECTION\n0\nEOF\n".getBytes(StandardCharsets.UTF_8)));

            List<RecoveryStore.Record> records=RecoveryStore.list(dir);
            if(records.size()!=1)throw new AssertionError("record count");
            RecoveryStore.Record first=records.get(0);
            if(!"Plan A.dxf".equals(first.displayName))throw new AssertionError("display name");
            if(first.fingerprint!=101L||first.savedAtMs!=1000L)throw new AssertionError("metadata");
            if(!first.dxfFile.isFile()||first.dxfFile.length()==0L)throw new AssertionError("dxf file");

            RecoveryStore.Record byFile=RecoveryStore.findByFile(dir,first.dxfFile);
            if(byFile==null||!id.equals(byFile.id))throw new AssertionError("find by file");
            if(!id.equals(RecoveryStore.idFromRecoveryFile(dir,first.dxfFile)))throw new AssertionError("id from file");

            RecoveryStore.write(dir,id,"Plan A.dxf","content://drawings/plan.dxf",202L,2000L,
                out->out.write("0\nUPDATED\n0\nEOF\n".getBytes(StandardCharsets.UTF_8)));
            records=RecoveryStore.list(dir);
            if(records.size()!=1||records.get(0).fingerprint!=202L)throw new AssertionError("replacement");
            String body=new String(Files.readAllBytes(records.get(0).dxfFile.toPath()),StandardCharsets.UTF_8);
            if(!body.contains("UPDATED"))throw new AssertionError("updated content");

            String id2=RecoveryStore.idFor("content://drawings/second.dxf");
            RecoveryStore.write(dir,id2,"Second.dxf","content://drawings/second.dxf",303L,3000L,
                out->out.write("0\nSECOND\n0\nEOF\n".getBytes(StandardCharsets.UTF_8)));
            records=RecoveryStore.list(dir);
            if(records.size()!=2||!id2.equals(records.get(0).id))throw new AssertionError("newest first");

            RecoveryStore.prune(dir,1);
            records=RecoveryStore.list(dir);
            if(records.size()!=1||!id2.equals(records.get(0).id))throw new AssertionError("prune");

            RecoveryStore.delete(dir,id2);
            if(!RecoveryStore.list(dir).isEmpty())throw new AssertionError("delete");

            System.out.println("Recovery store cases passed");
        } finally {
            deleteTree(dir);
        }
    }

    private static void deleteTree(File f){
        if(f==null||!f.exists())return;
        File[] children=f.listFiles();
        if(children!=null)for(File c:children)deleteTree(c);
        f.delete();
    }
}
