import com.musa.cad.ShortLicenseCode;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Offline 12-character MusaCAD license issuer for direct APK / institutional sales.
 * Input is the 12-character Serial displayed by MusaCAD.
 */
public final class ShortLicenseGenerator {
    public static String issue(String serial,String term,long nowMs)throws Exception{
        int days=parseDays(term);
        return ShortLicenseCode.issue(serial,days,nowMs);
    }

    private static int parseDays(String term){
        String value=term==null?"":term.trim();
        if("perpetual".equalsIgnoreCase(value)||"suresiz".equalsIgnoreCase(value)||"süresiz".equalsIgnoreCase(value))return 0;
        int days=Integer.parseInt(value);
        if(days<1||days>3650)throw new IllegalArgumentException("days must be 1..3650 or perpetual");
        return days;
    }

    public static void main(String[]args)throws Exception{
        if(args.length<3||args.length>4||!"issue".equalsIgnoreCase(args[0])){
            usage();
            System.exit(2);
        }
        String code=issue(args[1],args[2],System.currentTimeMillis());
        if(args.length==4)Files.writeString(Path.of(args[3]),code+System.lineSeparator(),StandardCharsets.UTF_8);
        System.out.println(code);
    }

    private static void usage(){
        System.out.println("MusaCAD 12-Character Offline License Generator\n"+
            "  issue <12-character-serial> <days|perpetual> [output.txt]");
    }

    private ShortLicenseGenerator(){}
}
