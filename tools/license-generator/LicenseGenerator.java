import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.spec.PKCS8EncodedKeySpec;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Base64;
import com.musa.cad.LicenseToken;

/** Simple offline MusaCAD license generator. Keep the private key outside the repository. */
public final class LicenseGenerator {
    public static void main(String[] args)throws Exception{
        if(args.length==0){usage();System.exit(2);}
        if("keygen".equalsIgnoreCase(args[0])){
            if(args.length!=3){usage();System.exit(2);}
            keygen(Path.of(args[1]),Path.of(args[2]));return;
        }
        if("issue".equalsIgnoreCase(args[0])){
            if(args.length<4||args.length>5){usage();System.exit(2);}
            Path privateKey=Path.of(args[1]);
            String installationId=args[2].trim();
            String term=args[3].trim();
            long expiresAt=0L;
            if(!"perpetual".equalsIgnoreCase(term)){
                long days=Long.parseLong(term);
                if(days<1||days>3650)throw new IllegalArgumentException("days must be 1..3650 or perpetual");
                expiresAt=Instant.now().plus(days,ChronoUnit.DAYS).toEpochMilli();
            }
            String token=issue(privateKey,installationId,expiresAt);
            if(args.length==5)Files.writeString(Path.of(args[4]),token+System.lineSeparator(),StandardCharsets.UTF_8);
            System.out.println(token);
            return;
        }
        usage();System.exit(2);
    }

    private static String issue(Path privateKeyPath,String installationId,long expiresAt)throws Exception{
        String payload=LicenseToken.payload(installationId,expiresAt);
        Signature signer=Signature.getInstance("SHA256withRSA");
        signer.initSign(readPrivateKey(privateKeyPath));
        signer.update(payload.getBytes(StandardCharsets.UTF_8));
        return LicenseToken.encode(payload,signer.sign());
    }

    private static void keygen(Path privateOut,Path publicOut)throws Exception{
        KeyPairGenerator generator=KeyPairGenerator.getInstance("RSA");
        generator.initialize(2048);
        KeyPair pair=generator.generateKeyPair();
        Files.writeString(privateOut,pem("PRIVATE KEY",pair.getPrivate().getEncoded()),StandardCharsets.US_ASCII);
        Files.writeString(publicOut,pem("PUBLIC KEY",pair.getPublic().getEncoded()),StandardCharsets.US_ASCII);
        System.out.println("Keys created. Never commit or share the private key.");
    }

    private static PrivateKey readPrivateKey(Path path)throws Exception{
        String pem=Files.readString(path,StandardCharsets.US_ASCII);
        String b64=pem.replace("-----BEGIN PRIVATE KEY-----","").replace("-----END PRIVATE KEY-----","").replaceAll("\\s","");
        return KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(Base64.getDecoder().decode(b64)));
    }

    private static String pem(String type,byte[] der){
        String b64=Base64.getMimeEncoder(64,new byte[]{'\n'}).encodeToString(der);
        return "-----BEGIN "+type+"-----\n"+b64+"\n-----END "+type+"-----\n";
    }

    private static void usage(){
        System.out.println("MusaCAD License Generator\n"+
            "  keygen <private.pem> <public.pem>\n"+
            "  issue <private.pem> <installation-id> <days|perpetual> [output.txt]");
    }
}
