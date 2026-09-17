import com.musa.cad.LicenseToken;
import java.nio.charset.StandardCharsets;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.Signature;
import java.util.Base64;

public class LicenseTokenTest {
    public static void main(String[] args)throws Exception{
        KeyPairGenerator g=KeyPairGenerator.getInstance("RSA");g.initialize(2048);KeyPair pair=g.generateKeyPair();
        String pub=pem("PUBLIC KEY",pair.getPublic().getEncoded());
        String id="ABC-DEVICE-123";
        long now=1_800_000_000_000L;

        String valid=issue(pair,id,now+86_400_000L);
        ok(LicenseToken.verify(valid,id,now,pub).valid,"valid token");
        ok(!LicenseToken.verify(valid,"OTHER",now,pub).valid,"wrong device");
        ok(!LicenseToken.verify(valid,id,now+172_800_000L,pub).valid,"expired token");
        String perpetual=issue(pair,id,0L);
        ok(LicenseToken.verify(perpetual,id,now+999_999_999L,pub).perpetual(),"perpetual token");
        char last=valid.charAt(valid.length()-1);
        String tampered=valid.substring(0,valid.length()-1)+(last=='A'?'B':'A');
        ok(!LicenseToken.verify(tampered,id,now,pub).valid,"tampered token");
        System.out.println("Signed license token cases passed");
    }

    private static String issue(KeyPair pair,String id,long expiry)throws Exception{
        String payload=LicenseToken.payload(id,expiry);
        Signature s=Signature.getInstance("SHA256withRSA");s.initSign(pair.getPrivate());s.update(payload.getBytes(StandardCharsets.UTF_8));
        return LicenseToken.encode(payload,s.sign());
    }
    private static String pem(String type,byte[] der){return "-----BEGIN "+type+"-----\n"+Base64.getMimeEncoder(64,new byte[]{'\n'}).encodeToString(der)+"\n-----END "+type+"-----\n";}
    private static void ok(boolean value,String name){if(!value)throw new AssertionError(name);}
}
