import com.musa.cad.DebugLicenseToken;

/**
 * Compatibility check for the previously delivered MusaCAD-Lisans-TEST.apk.
 * That generator issues MCT1 tokens bound to the exact full device/license ID.
 */
public final class DebugLicenseCompatibilityTest {
    public static void main(String[] args)throws Exception{
        String fullId="MC-597F8945-86D3A1C2-11223344";
        long expiry=System.currentTimeMillis()+86_400_000L;

        String token=DebugLicenseToken.issue(fullId,expiry);
        require(token.startsWith("MCT1."),"legacy TEST generator prefix");
        require(DebugLicenseToken.verify(token,fullId,System.currentTimeMillis()),"legacy TEST token must verify for exact full device ID");
        require(!DebugLicenseToken.verify(token,"MC-597F8945-86",System.currentTimeMillis()),"truncated on-screen ID must never verify");
        require(!DebugLicenseToken.verify(token,"MC-AAAAAAAA-BBBBBBBB-CCCCCCCC",System.currentTimeMillis()),"token must remain device-bound");

        String perpetual=DebugLicenseToken.issue(fullId,0L);
        require(DebugLicenseToken.verify(perpetual,fullId,System.currentTimeMillis()),"perpetual TEST token must verify");
        System.out.println("MusaCAD-Lisans-TEST.apk compatibility passed");
    }

    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
}
