import com.musa.cad.LicenseToken;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/** End-to-end check: displayed MusaCAD device ID -> offline generator -> app token verifier. */
public final class LicenseGeneratorRoundTripTest {
    public static void main(String[] args)throws Exception{
        Path dir=Files.createTempDirectory("musacad-license-roundtrip-");
        Path privateKey=dir.resolve("private.pem");
        Path publicKey=dir.resolve("public.pem");
        Path tokenFile=dir.resolve("license.txt");
        String deviceId="MC-12345678-90ABCDEF-12345678";

        try{
            LicenseGenerator.main(new String[]{"keygen",privateKey.toString(),publicKey.toString()});
            LicenseGenerator.main(new String[]{"issue",privateKey.toString(),deviceId,"perpetual",tokenFile.toString()});

            String token=Files.readString(tokenFile,StandardCharsets.UTF_8).trim();
            String publicPem=Files.readString(publicKey,StandardCharsets.US_ASCII);

            LicenseToken.Result ok=LicenseToken.verify(token,deviceId,System.currentTimeMillis(),publicPem);
            require(ok.valid,"generated token must verify for the displayed device ID");
            require(ok.perpetual(),"perpetual generator option must create a perpetual license");

            LicenseToken.Result wrong=LicenseToken.verify(token,"MC-AAAAAAAA-BBBBBBBB-CCCCCCCC",System.currentTimeMillis(),publicPem);
            require(!wrong.valid,"device-bound license must fail on another phone ID");

            System.out.println("License generator round trip passed");
        }finally{
            Files.deleteIfExists(tokenFile);
            Files.deleteIfExists(publicKey);
            Files.deleteIfExists(privateKey);
            Files.deleteIfExists(dir);
        }
    }

    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
}
