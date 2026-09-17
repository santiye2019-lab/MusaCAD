import com.musa.cad.SignedTrialPolicy;
import com.musa.cad.TrialPolicy;

public final class SignedTrialPolicyTest {
    public static void main(String[] args){
        long now=1_800_000_000_000L;
        require(SignedTrialPolicy.validWindow(true,now+TrialPolicy.TRIAL_DURATION_MS,now),"24h token");
        require(!SignedTrialPolicy.validWindow(true,0L,now),"perpetual rejected");
        require(!SignedTrialPolicy.validWindow(true,now-1L,now),"expired rejected");
        require(!SignedTrialPolicy.validWindow(false,now+TrialPolicy.TRIAL_DURATION_MS,now),"bad signature rejected");
        require(!SignedTrialPolicy.validWindow(true,now+TrialPolicy.TRIAL_DURATION_MS+SignedTrialPolicy.SERVER_CLOCK_TOLERANCE_MS+1L,now),"too-long token rejected");
        require(SignedTrialPolicy.clockOk(now,now),"normal clock");
        require(!SignedTrialPolicy.clockOk(now+TrialPolicy.CLOCK_ROLLBACK_TOLERANCE_MS+1L,now),"rollback rejected");
        require(SignedTrialPolicy.remainingMs(true,now+1000L,now,now)==1000L,"remaining");
        System.out.println("SignedTrialPolicyTest OK");
    }
    private static void require(boolean ok,String name){if(!ok)throw new AssertionError(name);}
}
