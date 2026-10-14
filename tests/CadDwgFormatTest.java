import com.musa.cad.CadDwgFormat;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public final class CadDwgFormatTest {
    private static void need(boolean ok,String reason){
        if(!ok)throw new AssertionError(reason);
    }
    private static void expect(String signature,String family,boolean modern){
        byte[] header=(signature+"    ").getBytes(StandardCharsets.US_ASCII);
        CadDwgFormat.Info info=CadDwgFormat.identify(header);
        need(info.supportedSignature,"recognized DWG format "+signature);
        need(info.signature.equals(signature),"exact signature");
        need(info.formatFamily.equals(family),"actual DWG format family");
        need(info.modern2018Family==modern,"modern DWG family match");
    }
    public static void main(String[] args)throws Exception{
        expect("AC1009","R12",false);
        expect("AC1012","R13",false);
        expect("AC1014","R14",false);
        expect("AC1015","2000",false);
        expect("AC1018","2004",false);
        expect("AC1021","2007",false);
        expect("AC1024","2010",false);
        expect("AC1027","2013",false);
        expect("AC1032","2018–2026",true);
        CadDwgFormat.Info modern=CadDwgFormat.identify("AC1032".getBytes(StandardCharsets.US_ASCII));
        need(modern.label().contains("2018–2026"),
                "modern family is NOT a separate DWG 2026 magic");
        need(!CadDwgFormat.identify("AC2026".getBytes(StandardCharsets.US_ASCII)).supportedSignature,
                "unknown fabricated AC2026 signature cannot pass");
        need(!CadDwgFormat.identify(new byte[]{0,1,2,3,4,5}).supportedSignature,
                "binary garbage is not a DWG version");

        File f=File.createTempFile("musacad-real-format-", ".dwg");
        try{
            Files.write(f.toPath(),"AC1032data".getBytes(StandardCharsets.US_ASCII));
            need(CadDwgFormat.identify(f).modern2018Family,
                    "read exact six-byte modern DWG header from a file");
            Files.write(f.toPath(),"AC10".getBytes(StandardCharsets.US_ASCII));
            boolean blocked=false;
            try{CadDwgFormat.identify(f);}
            catch(IOException expected){blocked=true;}
            need(blocked,"short DWG header must fail rather than guess");
        }finally{Files.deleteIfExists(f.toPath());}

        try{CadDwgFormat.identify(new byte[]{'A','C'});}
        catch(IllegalArgumentException expected){
            System.out.println("DWG AC1032 and legacy magic detection / malformed input PASS");
            return;
        }
        throw new AssertionError("short in-memory DWG signature accepted");
    }
}
