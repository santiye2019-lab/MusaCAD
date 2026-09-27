import com.musa.cad.PlayBillingPolicy;

public final class PlayBillingPolicyTest {
    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
    public static void main(String[]args){
        require(PlayBillingPolicy.isYearlyBillingPeriod("P1Y"),"P1Y must be accepted");
        require(PlayBillingPolicy.isYearlyBillingPeriod(" p1y "),"yearly period should be normalized");
        require(!PlayBillingPolicy.isYearlyBillingPeriod("P1M"),"monthly offer must be rejected");
        require(!PlayBillingPolicy.isYearlyBillingPeriod("P6M"),"six-month offer must be rejected");
        require(!PlayBillingPolicy.isYearlyBillingPeriod(null),"null period must be rejected");
        System.out.println("Google Play yearly billing policy cases passed");
    }
}
