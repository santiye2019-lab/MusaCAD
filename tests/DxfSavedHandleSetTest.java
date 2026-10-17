import com.musa.cad.DxfSavedHandleSet;
import java.io.IOException;

/** Exercises the same native C engine as Android through test-only Linux JNI. */
public final class DxfSavedHandleSetTest {
    private static void rejected(DxfSavedHandleSet set,String raw,String word)
            throws Exception {
        try {
            set.reserve(raw);
            throw new AssertionError("Handle accepted: "+raw);
        }catch(IOException expected) {
            if(!expected.getMessage().contains(word))throw expected;
        }
    }
    public static void main(String[] args)throws Exception {
        DxfSavedHandleSet set=new DxfSavedHandleSet(280);
        try {
            set.reserve(" 000a ");
            rejected(set,"a","yinelendi");
            rejected(set,"A","yinelendi");
            rejected(set,"GG","Invalid");
            rejected(set,"0","Invalid");
            set.reserve("FFFFFFFFFFFFFFFF");
            rejected(set,"ffffffffffffffff","yinelendi");
            for(int i=1;i<279;i++)set.reserve(Integer.toHexString(65536+i));
            rejected(set,"10001","yinelendi");
            rejected(set,"BCAD000","sınırı");
        }finally {set.close();}
        rejected(set,"14","kapatıldı");
        set.close(); // idempotent cleanup
        System.out.println("PASS: bounded shared-native DXF handle uniqueness via JNI");
    }
}
