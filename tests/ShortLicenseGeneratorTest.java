import com.musa.cad.ShortLicenseCode;

public final class ShortLicenseGeneratorTest {
    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
    public static void main(String[]args)throws Exception{
        long day=86_400_000L;
        long now=20_000L*day;
        String serial="597F894586D3";

        String yearly=ShortLicenseGenerator.issue(serial,"365",now);
        require(yearly.length()==12,"yearly code length");
        require(ShortLicenseCode.verify(yearly,serial,now),"yearly code must verify offline");
        require(!ShortLicenseCode.verify(yearly,"AAAAAAAAAAAA",now),"wrong Serial must fail");

        String perpetual=ShortLicenseGenerator.issue(serial,"perpetual",now);
        require(ShortLicenseCode.verify(perpetual,serial,now+5000L*day),"perpetual code must remain valid");

        System.out.println("Offline 12-character Serial license generator passed");
    }
}
