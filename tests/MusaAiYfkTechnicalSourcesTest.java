import com.musa.cad.MusaAiYfkTechnicalSources;
import java.util.*;
public final class MusaAiYfkTechnicalSourcesTest {
    private static void ok(boolean test,String message){
        if(!test)throw new AssertionError(message);
    }
    public static void main(String[] args){
        ok(MusaAiYfkTechnicalSources.VOLUMES.length==3,"Three official 2026 analysis books");
        for(MusaAiYfkTechnicalSources.Volume v:MusaAiYfkTechnicalSources.VOLUMES){
            ok(MusaAiYfkTechnicalSources.isOfficial(v.url),"Pinned official analysis document");
        }
        ok(!MusaAiYfkTechnicalSources.isOfficial(
            "https://webdosya.csb.gov.tr.evil.example/v2/yfk/2026/01/book.pdf"),"Host spoof");
        ok(!MusaAiYfkTechnicalSources.isOfficial(
            "http://webdosya.csb.gov.tr/v2/yfk/2026/01/book.pdf"),"HTTPS only");
        ok(!MusaAiYfkTechnicalSources.isOfficial(
            "https://webdosya.csb.gov.tr/v2/yfk/2025/01/book.pdf"),"Edition isolation");
        ok(!MusaAiYfkTechnicalSources.isOfficial(
            "https://user@webdosya.csb.gov.tr/v2/yfk/2026/01/book.pdf"),"Reject userinfo");
        ok(MusaAiYfkTechnicalSources.wantsInstall("Mekanik analizleri indir"),
            "Explicit user installation");
        ok(MusaAiYfkTechnicalSources.wantsStatus("Mekanik analiz durumu"),
            "Status intent");
        ok(MusaAiYfkTechnicalSources.wantsLookup("Mekanik analizde PVC boru ara"),
            "Technical text lookup");
        ok(MusaAiYfkTechnicalSources.wantsMatch("Poz eşleştir"),
            "Candidate matching intent");
        ok(!MusaAiYfkTechnicalSources.wantsLookup("Projeyi analiz et"),
            "General analysis not hijacked");
        ok("pvc boru".equals(MusaAiYfkTechnicalSources.query(
            "Mekanik analizde PVC boru ara")),"Normalized free term");
        ok("25.100.1005".equals(MusaAiYfkTechnicalSources.query(
            "Mekanik analizde 25.100.1005 pozunu incele")),"Code exact lookup");
        List<String> codes=MusaAiYfkTechnicalSources.codesIn(
            "25.100.1005 25.100.1005 25.200.1234 15.100.2000");
        ok(codes.size()==2 && codes.get(0).equals("25.100.1005")
            && codes.get(1).equals("25.200.1234"),"Unique mechanical codes");
        ok(MusaAiYfkTechnicalSources.category("25.100.1005").equals("Sıhhi tesisat"),
            "Sanitary taxonomy");
        ok(MusaAiYfkTechnicalSources.category("25.705.1000").equals("Yangın tesisatı"),
            "Fire protection taxonomy");
        ok(MusaAiYfkTechnicalSources.category("25.455.1000").equals("Havalandırma ve klima"),
            "HVAC taxonomy");
        System.out.println("MusaAiYfkTechnicalSourcesTest OK");
    }
}
