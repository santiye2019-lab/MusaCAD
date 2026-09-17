import com.musa.cad.LicenseToken;
import com.musa.cad.TrialToken;
import java.nio.charset.StandardCharsets;
import java.security.*;
import java.util.Base64;

public final class TrialTokenTest {
    public static void main(String[] args)throws Exception{
        KeyPairGenerator g=KeyPairGenerator.getInstance("RSA");g.initialize(2048);
        KeyPair trial=g.generateKeyPair(),paid=g.generateKeyPair();
        String trialPub=pem(trial.getPublic().getEncoded()),paidPub=pem(paid.getPublic().getEncoded());
        String id="MC-12345678-90ABCDEF-12345678";long now=1_800_000_000_000L,exp=now+86_400_000L;

        String payload=TrialToken.payload(id,exp);
        Signature signer=Signature.getInstance("SHA256withRSA");signer.initSign(trial.getPrivate());signer.update(payload.getBytes(StandardCharsets.UTF_8));
        String token=TrialToken.encode(payload,signer.sign());

        require(TrialToken.verify(token,id,now,trialPub).valid,"valid trial");
        require(!TrialToken.verify(token,id,now,paidPub).valid,"wrong key rejected");
        require(!LicenseToken.verify(token,id,now,paidPub).valid,"paid verifier rejects MT1");
        require(!TrialToken.verify(token,"MC-00000000-00000000-00000000",now,trialPub).valid,"wrong device");
        require(!TrialToken.verify(token,id,exp+1,trialPub).valid,"expired");

        String paidPayload=LicenseToken.payload(id,0L);signer.initSign(paid.getPrivate());signer.update(paidPayload.getBytes(StandardCharsets.UTF_8));
        String paidToken=LicenseToken.encode(paidPayload,signer.sign());
        require(LicenseToken.verify(paidToken,id,now,paidPub).valid,"paid valid");
        require(!TrialToken.verify(paidToken,id,now,trialPub).valid,"trial verifier rejects MC1");
        System.out.println("TrialTokenTest OK");
    }
    private static String pem(byte[] der){
        String b64=Base64.getMimeEncoder(64,new byte[]{'\n'}).encodeToString(der);
        return "-----BEGIN PUBLIC KEY-----\n"+b64+"\n-----END PUBLIC KEY-----\n";
    }
    private static void require(boolean ok,String name){if(!ok)throw new AssertionError(name);}
}
