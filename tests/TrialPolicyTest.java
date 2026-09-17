import com.musa.cad.TrialPolicy;

public class TrialPolicyTest {
    private static void ok(boolean value,String message){if(!value)throw new AssertionError(message);}
    public static void main(String[] args){
        long start=1_000_000L;
        ok(TrialPolicy.isActive(start,start,start),"trial must start active");
        ok(TrialPolicy.isActive(start,start,start+23L*60L*60L*1000L+59L*60L*1000L),"trial must remain active before 24h");
        ok(!TrialPolicy.isActive(start,start,start+TrialPolicy.TRIAL_DURATION_MS),"trial must expire at 24h");
        ok(!TrialPolicy.isActive(start,start+60L*60L*1000L,start-10L*60L*1000L),"large clock rollback must fail closed");
        ok(TrialPolicy.remainingMs(start,start,start)==TrialPolicy.TRIAL_DURATION_MS,"full duration remaining at start");
        ok(TrialPolicy.remainingMs(0,0,start)==0,"unused trial has no active remaining time");
        System.out.println("One-day trial policy cases passed");
    }
}
