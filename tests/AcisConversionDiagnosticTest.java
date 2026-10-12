import com.musa.cad.AcisConversionDiagnostic;
import com.musa.cad.DxfAcisCoverage;
import java.util.List;

public final class AcisConversionDiagnosticTest {
    private static void check(boolean valid,String message){
        if(!valid)throw new AssertionError(message);
    }
    public static void main(String[] args){
        DxfAcisCoverage empty=new DxfAcisCoverage();
        check(AcisConversionDiagnostic.status(0,0,empty.snapshot()).isEmpty(),
                "Ordinary drawings must not show ACIS warnings");

        DxfAcisCoverage fullyTagged=new DxfAcisCoverage();
        List<String> sat=List.of("8","3D","1","700 0 1 0","3","SAT-DATA");
        fullyTagged.accept("3DSOLID",sat,0,sat.size());
        String present=AcisConversionDiagnostic.status(1,0,fullyTagged.snapshot());
        check(present.contains("DWG 1 / DXF 1"),"Matching entity counts must be visible");
        check(present.contains("geometri doğrulanmadı"),
                "Presence of ACIS tags is not geometric fidelity proof");
        check(!present.contains("nesne sayıları farklı"),"No false mismatch for equal counts");

        DxfAcisCoverage missing=new DxfAcisCoverage();
        List<String> noBody=List.of("5","A1","70","2");
        missing.accept("REGION",noBody,0,noBody.size());
        String loss=AcisConversionDiagnostic.status(3,2,missing.snapshot());
        check(loss.contains("DWG 3 / DXF 1"),"Source and DXF counts must stay distinct");
        check(loss.contains("nesne sayıları farklı"),"Unexplained count deficit must be visible");
        check(loss.contains("gövde verisi eksik 1"),"Missing SAT/SAB data must be visible");
        check(loss.contains("harici AcDs 2"),"External geometry must not be called resolved");

        DxfAcisCoverage corrupted=new DxfAcisCoverage();
        List<String> badHex=List.of("310","41Z1");
        corrupted.accept("BODY",badHex,0,badHex.size());
        String bad=AcisConversionDiagnostic.status(0,0,corrupted.snapshot());
        check(bad.contains("bozuk SAB parçası 1"),"Invalid SAB must be reported");
        check(bad.contains("gövde verisi eksik 1"),"Invalid payload must not be trusted");

        String sourceOnly=AcisConversionDiagnostic.status(1,1,empty.snapshot());
        check(sourceOnly.contains("DWG 1 / DXF 0"),"Dropped ACIS records must remain visible");
        check(sourceOnly.contains("harici AcDs 1"),"Dropped external ACIS must be identified");

        check(AcisConversionDiagnostic.status(-1,-4,null).isEmpty(),
                "Invalid unavailable counters must not invent an ACIS warning");
        System.out.println("PASS: ACIS source, DXF payload, AcDs and geometric uncertainty stay distinct");
    }
}
