import com.musa.cad.ShortLicenseCode;

public final class ShortLicenseCodeTest {
    public static void main(String[] args)throws Exception{
        String installation="MC-597F8945-86D3A1C2-11223344";
        String serial=ShortLicenseCode.serialFromInstallationId(installation);
        require(serial.length()==12,"Serial must be exactly 12 characters");
        require(serial.equals("597F894586D3"),"Serial derivation");

        long day=86_400_000L;
        long now=20_000L*day;
        String yearly=ShortLicenseCode.issue(serial,365,now);
        require(yearly.length()==12,"License code must be exactly 12 characters");
        require(ShortLicenseCode.verify(yearly,serial,now),"Issued code must verify");
        require(!ShortLicenseCode.verify(yearly,"AAAAAAAAAAAA",now),"Wrong Serial must fail");
        require(!ShortLicenseCode.verify(yearly,serial,now+366L*day),"Expired code must fail");

        String perpetual=ShortLicenseCode.issue(serial,0,now);
        require(perpetual.length()==12,"Perpetual code length");
        require(ShortLicenseCode.verify(perpetual,serial,now+5000L*day),"Perpetual code must remain valid");

        System.out.println("ShortLicenseCodeTest OK: serial="+serial+" code="+yearly);
    }

    private static void require(boolean value,String message){
        if(!value)throw new AssertionError(message);
    }
}
