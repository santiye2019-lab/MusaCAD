import com.musa.cad.DebugLicenseToken;

public class DebugLicenseTokenTest {
    public static void main(String[] args)throws Exception{
        long now=1_800_000_000_000L;
        String id="MC-TEST-DEVICE-01";
        String oneDay=DebugLicenseToken.issue(id,now+86_400_000L);
        if(!DebugLicenseToken.verify(oneDay,id,now))throw new AssertionError("valid");
        if(DebugLicenseToken.verify(oneDay,"MC-OTHER",now))throw new AssertionError("device");
        if(DebugLicenseToken.verify(oneDay,id,now+86_400_001L))throw new AssertionError("expiry");
        String perpetual=DebugLicenseToken.issue(id,0L);
        if(!DebugLicenseToken.verify(perpetual,id,now+9_999_999_999L))throw new AssertionError("perpetual");
        String tampered=oneDay.substring(0,oneDay.length()-1)+(oneDay.endsWith("A")?"B":"A");
        if(DebugLicenseToken.verify(tampered,id,now))throw new AssertionError("tamper");
        System.out.println("5 debug license token cases passed");
    }
}
