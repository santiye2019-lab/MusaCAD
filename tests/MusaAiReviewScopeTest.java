import com.musa.cad.*;
import java.util.*;

public final class MusaAiReviewScopeTest {
    private static void check(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    private static MusaAiReviewScope scope(String text,String system,MusaAiDiscipline discipline){
        MusaAiReviewScope s=MusaAiReviewScope.parse(text);
        check(s!=null,"not routed: "+text);
        check(system.equals(s.system),"wrong sub-system: "+text+" -> "+s.system);
        check(discipline==s.discipline,"wrong discipline: "+text+" -> "+s.discipline);
        return s;
    }
    public static void main(String[] args){
        scope("Gandalf, projeyi analiz et","ALL",MusaAiDiscipline.UNKNOWN);
        scope("Bu çizimi analiz et","ALL",MusaAiDiscipline.UNKNOWN);
        scope("Mekanik olarak analiz et","ALL",MusaAiDiscipline.MECHANICAL);
        scope("Sıhhi tesisat olarak analiz et","SANITARY",MusaAiDiscipline.MECHANICAL);
        scope("Pis su tesisatını analiz et","WASTEWATER",MusaAiDiscipline.MECHANICAL);
        scope("Havalandırma olarak analiz et","VENTILATION",MusaAiDiscipline.MECHANICAL);
        scope("Yangın tesisatını analiz et","FIRE",MusaAiDiscipline.FIRE_SAFETY);
        scope("Statik projeyi analiz et","ALL",MusaAiDiscipline.STRUCTURAL);
        scope("Elektrik projesini analiz et","ALL",MusaAiDiscipline.ELECTRICAL);
        scope("Mimari olarak analiz et","ALL",MusaAiDiscipline.ARCHITECTURAL);
        scope("Tüm disiplinlerde projeyi analiz et","ALL",MusaAiDiscipline.UNKNOWN);
        check(MusaAiReviewScope.offlineRequested("Projeyi çevrim dışı analiz et"),"offline intent");
        check(!MusaAiReviewScope.asksAnalysis("Raporu PDF olarak çıkar"),"PDF must not run review");
        check(!MusaAiReviewScope.asksAnalysis("Projeden keşif oluştur"),"BOQ must not run review");
        check(!MusaAiReviewScope.asksAnalysis("Ekrana sığdır"),"CAD command must not run review");

        MusaAiDrawingIndex index=new MusaAiDrawingIndex("Model",4,0,
            Arrays.asList("PIS_SU_DN100","HAVALANDIRMA","MIMARI"),
            Arrays.asList("PIS_SU_DN100","HAVALANDIRMA","MIMARI"),
            Arrays.asList(
                new MusaAiDrawingIndex.Item(10,"LINE","PIS_SU_DN100","",15d,Double.NaN),
                new MusaAiDrawingIndex.Item(11,"TEXT","PIS_SU_DN100","DN100",Double.NaN,Double.NaN),
                new MusaAiDrawingIndex.Item(20,"LINE","HAVALANDIRMA","",12d,Double.NaN),
                new MusaAiDrawingIndex.Item(30,"LINE","MIMARI","",99d,Double.NaN)),
            "m");
        MusaAiReviewScope waste=scope("Pis su tesisatını analiz et","WASTEWATER",MusaAiDiscipline.MECHANICAL);
        MusaAiDrawingIndex filtered=waste.filteredIndex(index);
        check(filtered.items().size()==2,"sub-system index must not include other trades");
        String report=waste.localEvidence(index,"tesisat.dwg");
        check(report.contains("Pis su: 15.00 m"),"source-linked pipe length should be reported");
        check(!report.contains("Havalandırma: 12.00 m"),"unrelated ventilation should be excluded");
        check(report.contains("İstenen kapsam: Pis su"),"report should name scope");
        System.out.println("MusaAiReviewScopeTest OK");
    }
}
