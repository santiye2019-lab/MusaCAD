import com.musa.cad.*;
import java.util.*;

public final class MusaAiProjectPacketTest {
    private static void has(String s,String n){if(!s.contains(n))throw new AssertionError("missing "+n);}
    private static void no(String s,String n){if(s.contains(n))throw new AssertionError("must not contain "+n);}
    public static void main(String[]args){
        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",4,0,
            Arrays.asList("M_PIS_SU","M_SPRINKLER"),
            Arrays.asList("M_PIS_SU"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(1,"LWPOLYLINE","M_PIS_SU","Ø100 PİS SU",12.5,Double.NaN,true,false,"g1",10,20),
                new MusaAiDrawingIndex.Item(2,"TEXT","M_SPRINKLER","SPRINKLER KONTROL",Double.NaN,Double.NaN)
            ),"m");

        MusaAiProjectPacket p=MusaAiProjectPacket.build(index,"/storage/emulated/0/Projeler/Test Projesi.dwg");
        has(p.json,"\"schema\":\"musacad.project.v1\"");
        has(p.json,"\"file_name\":\"Test Projesi.dwg\"");
        has(p.json,"M_PIS_SU");
        has(p.json,"Ø100 PİS SU");
        has(p.json,"\"center_x\":10");
        has(p.json,"\"closed\":false");
        has(p.json,"mechanical_summary");
        no(p.json,"/storage/emulated");
        if(p.includedItems!=2||p.totalItems!=2||p.truncated)throw new AssertionError("packet counts");

        ArrayList<MusaAiDrawingIndex.Item>many=new ArrayList<>();
        for(int i=0;i<MusaAiProjectPacket.MAX_ITEMS+3;i++)many.add(new MusaAiDrawingIndex.Item(i,"LINE","L","",1,Double.NaN));
        MusaAiDrawingIndex big=new MusaAiDrawingIndex("Model",many.size(),0,Arrays.asList("L"),Arrays.asList("L"),many,"mm");
        MusaAiProjectPacket bp=MusaAiProjectPacket.build(big,"big.dwg");
        if(!bp.truncated||bp.includedItems!=MusaAiProjectPacket.MAX_ITEMS)throw new AssertionError("truncation");

        System.out.println("MusaAiProjectPacketTest OK");
    }
}
