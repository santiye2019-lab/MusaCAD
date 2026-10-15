import com.musa.cad.NativeSceneFingerprint;
import java.security.MessageDigest;
import java.util.Arrays;

public final class NativeSceneFingerprintTest {
    private static void need(boolean pass,String what){
        if(!pass)throw new AssertionError(what);
    }
    private static void rejected(float[] words){
        try{
            NativeSceneFingerprint.sha256(words);
            throw new AssertionError("invalid stream unexpectedly hashed");
        }catch(IllegalArgumentException expected){}
    }
    public static void main(String[] args)throws Exception{
        // Verify the actual bytes being hashed (big-endian raw IEEE-754 bits).
        float[] simple={1.0f,-0.0f};
        byte[] expected=MessageDigest.getInstance("SHA-256").digest(
                new byte[]{0x3f,(byte)0x80,0,0,(byte)0x80,0,0,0});
        byte[] first=NativeSceneFingerprint.sha256(simple);
        need(Arrays.equals(first,expected),"SHA-256 exact raw float bit order");
        need(NativeSceneFingerprint.matches(first,
                NativeSceneFingerprint.sha256(simple.clone())),
                "unchanged page is reproducible");
        need(!NativeSceneFingerprint.matches(first,
                NativeSceneFingerprint.sha256(new float[]{1.0f,0.0f})),
                "positive/negative zero raw words differ");

        // Same record count, same external bounding box, different internal
        // geometry and color: old cursor/bounds/count parity missed this.
        float[] nativePage={
            6f,5f,-10f,-10f,10f,10f, /* v6 header stand-in */
            1f,0f,0f,10f,10f,        /* source line */
            1f,0f,10f,10f,0f,        /* internal line */
            0x00fffff0              /* color token */
        };
        byte[] original=NativeSceneFingerprint.sha256(nativePage);
        float[] changedLine=nativePage.clone();
        changedLine[12]=9f; // unchanged endpoints of first line / world bounds
        need(!NativeSceneFingerprint.matches(original,
                NativeSceneFingerprint.sha256(changedLine)),
                "interior geometry drift is detected");
        float[] changedColor=nativePage.clone();
        changedColor[changedColor.length-1]=1234f;
        need(!NativeSceneFingerprint.matches(original,
                NativeSceneFingerprint.sha256(changedColor)),
                "color drift is detected");
        need(!NativeSceneFingerprint.matches(original,new byte[31]),
                "truncated digest rejected");
        need(!NativeSceneFingerprint.matches(null,original),
                "missing expected source fingerprint rejected");
        rejected(null);
        rejected(new float[]{});

        // Multi-batch hash path must preserve all source bytes.
        float[] big=new float[32775];
        for(int i=0;i<big.length;i++)big[i]=i*0.03125f;
        byte[] bigBefore=NativeSceneFingerprint.sha256(big);
        need(NativeSceneFingerprint.matches(bigBefore,
                NativeSceneFingerprint.sha256(big.clone())),
                "streaming >8KB fingerprint is stable");
        big[32774]=big[32774]+1f;
        need(!NativeSceneFingerprint.matches(bigBefore,
                NativeSceneFingerprint.sha256(big)),
                "changed final word in a multi-buffer stream detected");
        System.out.println("Native DWG full page SHA-256: geometry, colors, raw bits and streaming PASS");
    }
}
