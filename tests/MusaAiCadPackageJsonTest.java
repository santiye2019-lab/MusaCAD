import com.musa.cad.*;
import java.util.*;

public final class MusaAiCadPackageJsonTest {
    private static MusaAiDrawingIndex index(String layer,int count){
        ArrayList<MusaAiDrawingIndex.Item> items=new ArrayList<>();
        for(int i=0;i<count;i++)
            items.add(new MusaAiDrawingIndex.Item(i+1,"LINE",layer,"TEST "+i,1,Double.NaN,false,false,"LINE|0,0;100000,0|c=0",i,i));
        return new MusaAiDrawingIndex("Model",count,0,Arrays.asList(layer),Arrays.asList(layer),items,"mm");
    }
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    public static void main(String[]args){
        ArrayList<MusaAiProjectPackage.Drawing> drawings=new ArrayList<>();
        drawings.add(new MusaAiProjectPackage.Drawing("mimari.dwg",index("MIM_KAPI",350),null));
        drawings.add(new MusaAiProjectPackage.Drawing("statik.dwg",index("S_KIRIS",10),null));
        drawings.add(new MusaAiProjectPackage.Drawing("mekanik.dwg",index("MEK_BORU",10),null));
        drawings.add(new MusaAiProjectPackage.Drawing("elektrik.dwg",index("E_PANO",10),null));
        drawings.add(new MusaAiProjectPackage.Drawing("peyzaj.dwg",index("PEYZAJ",10),null));
        String json=MusaAiCadPackageJson.build(drawings,"yerel paket özeti");
        has(json,"\"schema\":\"musacad-cad-package/v1\"");
        has(json,"\"drawingCount\":4");
        has(json,"\"fileName\":\"mimari.dwg\"");
        has(json,"\"detectedDiscipline\":\"ARCHITECTURAL\"");
        has(json,"\"packageEditToolsAllowed\":false");
        has(json,"\"rawDrawingIncluded\":false");
        has(json,"\"localAuditSummary\":\"yerel paket özeti\"");
        has(json,"\"itemsIncluded\":300");
        if(json.contains("peyzaj.dwg"))throw new AssertionError("drawing limit ignored");
        if(json.contains("\"sourceId\":301"))throw new AssertionError("per-drawing item limit ignored");
        if(json.contains("geometryKey"))throw new AssertionError("internal geometry key leaked");
        System.out.println("MusaAiCadPackageJsonTest OK");
    }
}
