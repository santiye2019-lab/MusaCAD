import com.musa.cad.*;
import java.util.*;

public final class CadBlockLibraryTest {
    private static void require(boolean value,String message){if(!value)throw new AssertionError(message);}

    public static void main(String[]args){
        require(CadBlockLibrary.all().size()>=20,"library must include a useful starter set");
        require(CadBlockLibrary.categories().contains("Mimari"),"architecture category");
        require(CadBlockLibrary.categories().contains("Mekanik"),"mechanical category");
        require(CadBlockLibrary.categories().contains("Elektrik"),"electrical category");
        require(CadBlockLibrary.categories().contains("Proje"),"project category");

        List<CadBlockLibrary.Entry> pumps=CadBlockLibrary.filter("Mekanik","pompa");
        require(pumps.size()==1&&"mec_pump".equals(pumps.get(0).id),"pump search");
        CadBlock.Definition pump=pumps.get(0).definition(Collections.emptyMap());
        require(pump.members.size()>=4,"pump geometry");

        CadBlockLibrary.Entry panel=CadBlockLibrary.findById("elec_panel");
        require(panel!=null&&panel.attributeDefaults.containsKey("PANO"),"panel attribute");
        Map<String,String> first=new LinkedHashMap<>();first.put("PANO","P-01");
        Map<String,String> second=new LinkedHashMap<>();second.put("PANO","P-02");
        CadBlock.Definition p1=panel.definition(first),p2=panel.definition(second);
        require(!p1.name.equals(p2.name),"attribute variants need distinct block definitions");
        boolean found=false;
        for(CadEdit edit:p2.members)if("P-02".equals(edit.text))found=true;
        require(found,"attribute text must be embedded into definition");

        require(CadBlockLibrary.filter(null,"sprinkler yangın").size()==1,"multi-token Turkish search");
        require(CadBlockLibrary.findById("missing")==null,"unknown id");
        System.out.println("CadBlockLibraryTest OK");
    }
}
