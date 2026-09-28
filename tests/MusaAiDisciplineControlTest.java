import com.musa.cad.*;
import java.util.*;

public final class MusaAiDisciplineControlTest {
    private static void require(boolean v,String m){if(!v)throw new AssertionError(m);}
    private static void has(String s,String n,String m){if(s==null||!s.contains(n))throw new AssertionError(m+": "+s);}

    private static MusaAiDrawingIndex index(){
        List<MusaAiDrawingIndex.Item>items=Arrays.asList(
            new MusaAiDrawingIndex.Item(1,"TEXT","A-MIMARI","Mahal OFİS KAPI PENCERE KOT +0.00",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(2,"DIMENSION","A-MIMARI","Kesit A-A",5,Double.NaN),
            new MusaAiDrawingIndex.Item(3,"TEXT","S-STATIK","AKS A-1 KOLON K1 KİRİŞ K101 DÖŞEME",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(4,"TEXT","S-TEMEL","RADYE TEMEL DONATI B420 C30/37",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(5,"TEXT","E-ELEKTRIK","MDB PANO DEVRE KABLO N2XH AYDINLATMA PRİZ TOPRAKLAMA",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(6,"TEXT","M-MEKANIK","PİS SU DN100 POMPA 5.5 kW",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(7,"TEXT","F-YANGIN","YANGIN İHBAR DUMAN DETEKTÖR SPRINKLER HİDRANT ACİL ÇIKIŞ YANGIN KAPISI",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(8,"TEXT","C-ALTYAPI","ALTYAPI RÖGAR TABAN KOTU YOL PARSEL KANALİZASYON",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(9,"TEXT","L-PEYZAJ","PEYZAJ AĞAÇ BİTKİ ÇİM SULAMA SERT ZEMİN BİTKİ LİSTESİ",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(10,"TEXT","V-ASANSOR","ASANSÖR KUYUSU KABİN KAT KAPISI PIT KAPASİTE 1000 KG HIZ 1.6 m/s",Double.NaN,Double.NaN),
            new MusaAiDrawingIndex.Item(11,"LINE","S-STATIK","EKSİK REVİZE",0,Double.NaN)
        );
        return new MusaAiDrawingIndex("Model",items.size(),0,
            Arrays.asList("A-MIMARI","S-STATIK","S-TEMEL","E-ELEKTRIK","M-MEKANIK","F-YANGIN","C-ALTYAPI","L-PEYZAJ","V-ASANSOR"),
            Arrays.asList("A-MIMARI","S-STATIK","S-TEMEL","E-ELEKTRIK","M-MEKANIK","F-YANGIN","C-ALTYAPI","L-PEYZAJ","V-ASANSOR"),
            items,"m");
    }

    public static void main(String[]args){
        MusaAiDisciplineControl.Result full=MusaAiDisciplineControl.analyze(index(),"AI_DISIPLIN_KONTROL");
        require(full.matched,"full command matched");
        require(full.detected.containsAll(EnumSet.allOf(MusaAiDisciplineControl.Discipline.class)),"all disciplines detected");
        has(full.text,"Mimari ↔ Statik","architecture/structural coordination");
        has(full.text,"Statik ↔ MEP","structural/MEP coordination");
        has(full.text,"Asansör ↔ Mimari/Statik","elevator coordination");
        has(full.text,"Altyapı ↔ Peyzaj/Mimari","infrastructure coordination");
        require(full.findingCount>=2,"structural issue candidates");

        MusaAiDisciplineControl.Result statik=MusaAiDisciplineControl.analyze(index(),"Statik projeyi kontrol et");
        require(statik.matched,"natural structural command");
        has(statik.text,"Statik / taşıyıcı sistem","structural label");
        require(MusaAiDisciplineControl.requestedDisciplines("AI_STATIK_KONTROL").contains(MusaAiDisciplineControl.Discipline.STRUCTURAL),"structural request");

        require(MusaAiDisciplineControl.classifyText("ASANSÖR KUYUSU KABİN").contains(MusaAiDisciplineControl.Discipline.ELEVATOR),"elevator classification");
        require(!MusaAiDisciplineControl.classifyText("SU KUYUSU").contains(MusaAiDisciplineControl.Discipline.ELEVATOR),"well must not become elevator");

        MusaAiDisciplineControl.Result mechanicalFire=MusaAiDisciplineControl.analyze(index(),"Sprinkler mekanik kontrol");
        require(!mechanicalFire.matched,"mechanical fire request remains for mechanical engine");

        System.out.println("MusaAiDisciplineControlTest OK");
    }
}
