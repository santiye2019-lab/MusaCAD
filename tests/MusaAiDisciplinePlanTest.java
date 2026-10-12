import com.musa.cad.*;
import java.util.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

/** Regression for natural Turkish engineering discipline and subsystem requests. */
public final class MusaAiDisciplinePlanTest {
    static void yes(boolean b,String why){if(!b)throw new AssertionError(why);}
    static void has(String text,String item){yes(text.contains(item),"Missing "+item);}
    static void scope(String sentence,String fallback,String scope,String... all){
        MusaAiDisciplinePlan.Plan plan=MusaAiDisciplinePlan.resolve(sentence,fallback);
        yes(scope.equals(plan.scope),sentence+" => "+plan.scope+" expected "+scope);
        for(String key:all)yes(plan.includes(key),sentence+" missing "+key);
    }
    static MusaAiDrawingIndex index(){
        List<MusaAiDrawingIndex.Item> items=Arrays.asList(
            new MusaAiDrawingIndex.Item(10,"TEXT","MEK-TEMIZ-SU","DN25"),
            new MusaAiDrawingIndex.Item(11,"TEXT","MEK-PIS-SU","DN100"),
            new MusaAiDrawingIndex.Item(12,"TEXT","ELK-ZAYIF-AKIM","kamera data"),
            new MusaAiDrawingIndex.Item(13,"INSERT","BMS-OTOMASYON","DDC"),
            new MusaAiDrawingIndex.Item(14,"LINE","ELK-KUVVETLI-AKIM","PANO"),
            new MusaAiDrawingIndex.Item(15,"LINE","STATIK-KOLON",""),
            new MusaAiDrawingIndex.Item(16,"TEXT","MEK-KLIMA","VRF")
        );
        return new MusaAiDrawingIndex("Model",7,0,
            Arrays.asList("MEK-TEMIZ-SU","MEK-PIS-SU","ELK-ZAYIF-AKIM",
                "BMS-OTOMASYON","ELK-KUVVETLI-AKIM","STATIK-KOLON","MEK-KLIMA"),
            Arrays.asList("MEK-TEMIZ-SU","MEK-PIS-SU","ELK-ZAYIF-AKIM",
                "BMS-OTOMASYON","ELK-KUVVETLI-AKIM","STATIK-KOLON","MEK-KLIMA"),
            items,"mm");
    }
    public static void main(String[]args)throws Exception{
        scope("Gandalf, mekanik tesisatı tüm alt sistemleriyle kontrol et",
            "all","mechanical","cleanwater","wastewater","heating",
            "ventilation","cooling","fire","automation");
        scope("Elektrik projesini bir inceleyiver","all","electrical",
            "lowcurrent","highcurrent");
        scope("Yalnız pis suya bak","mechanical","wastewater","wastewater");
        scope("Şu statik paftada ne eksik?","all","structural","structural");
        scope("Buradaki yangın algılama projesini analiz et","all","lowcurrent","lowcurrent");
        scope("BMS ve zayıf akım şemalarını karşılaştır","all","all","automation","lowcurrent");
        scope("Mimariyle statik arasındaki rezervasyonları kontrol et","all","all",
            "architectural","structural");
        scope("Temiz su, pis su ve kalorifer projelerini incele","all","all",
            "cleanwater","wastewater","heating");
        scope("Bu DWG'nin tüm disiplinlerini tek tek analiz et","mechanical","all",
            "architectural","structural","cleanwater","wastewater","heating",
            "ventilation","cooling","fire","automation","lowcurrent","highcurrent");
        scope("Havalandırma ve klima paftalarını denetle","all","all",
            "ventilation","cooling");
        scope("Otomasyon tesisatlarını bir gözden geçir","all","automation","automation");
        scope("Kuvvetli akım hatlarını ve panoları incele","all","highcurrent","highcurrent");

        MusaAiDisciplinePlan.Plan all=MusaAiDisciplinePlan.resolve(
            "Mimari, statik, mekanik tesisat, temiz su, pis su, kalorifer, havalandırma, klima, yangın, otomasyon, elektrik zayıf ve kuvvetli akımı incele","all");
        for(String s:new String[]{"architectural","structural","cleanwater",
            "wastewater","heating","ventilation","cooling","fire","automation",
            "lowcurrent","highcurrent"})
            yes(all.includes(s),"Missing user-required subsystem: "+s);
        has(all.visionInstruction(),"Her alt disiplin için AYRI başlık");
        has(all.register(),"Elektrik / Kuvvetli akım");
        has(all.register(),"Mekanik / Kalorifer");
        yes(all.localParents().contains(MusaAiDiscipline.ELECTRICAL),"electrical local review");
        yes(all.localParents().contains(MusaAiDiscipline.STRUCTURAL),"structural local review");
        yes(all.localParents().contains(MusaAiDiscipline.ARCHITECTURAL),"architectural local review");

        MusaAiDisciplinePlan.Evidence local=all.localEvidence(index());
        has(local.text,"MEK-TEMIZ-SU");
        has(local.text,"BMS-OTOMASYON");
        has(local.text,"ELK-ZAYIF-AKIM");
        yes(local.sourceIds.contains(10)&&local.sourceIds.contains(13),"CAD ids retained");
        has(local.text,"sistemin mevcut olmadığı sonucu çıkarılamaz");
        String main=Files.readString(Path.of(
            "app/src/main/java/com/musa/cad/MainActivity.java"),StandardCharsets.UTF_8);
        has(main,"MusaAiDisciplinePlan.resolve(raw,reportScope)");
        has(main,"plan.visionInstruction()");
        has(main,"plan.localEvidence(snapshot)");
        has(main,"MusaAiDisciplineAnalyzer.analyzeDiscipline(snapshot,parent)");
        has(main,"MusaAiEngineeringSynthesis.composeDetailed(synthesis)");
        yes(!MusaAiAnalysisIntent.isReview("Raporu PDF olarak çıkar"),
            "PDF export must not become review");
        yes(MusaAiAnalysisIntent.isReview("Bu projedeki otomasyon tesisatını incele"),
            "natural automation review");
        yes(MusaAiAnalysisIntent.isReview("Zayıf akım paftasına bak"),
            "natural low-current review");
        yes(MusaAiAnalysisIntent.isReview("Kuvvetli akım projesini analiz et"),
            "natural strong-current review");
        yes(!MusaAiDisciplinePlan.wantsMultiOpenFiles("Tüm disiplinleri incele"),
            "All disciplines in active project is not a multi-file project package");
        yes(MusaAiDisciplinePlan.wantsMultiOpenFiles("Tüm açık projeleri karşılaştır"),
            "Explicit multi-open-project intent is retained");
        has(main,"MusaAiDisciplinePlan.wantsMultiOpenFiles(raw)");
        MusaAiDisciplinePlan.Plan broad=MusaAiDisciplinePlan.resolve(
            "Bu projeyi incele","mechanical");
        MusaAiDisciplinePlan.Plan focused=MusaAiDisciplinePlan.focusOnDrawing(
            "Bu projeyi incele",broad,index());
        yes(focused.sections.size()<=3,"Generic mechanical requests must not list all 8 systems");
        yes(focused.includes("cleanwater")&&focused.includes("wastewater"),
            "DWG layer and label evidence should select plumbing disciplines");
        yes(!focused.includes("heating"),"Unseen heating must not dominate a kitchen drawing");
        MusaAiDisciplinePlan.Plan explicit=MusaAiDisciplinePlan.focusOnDrawing(
            "Mekanik tesisatı tüm alt sistemleriyle kontrol et",
            MusaAiDisciplinePlan.resolve("Mekanik tesisatı tüm alt sistemleriyle kontrol et","mechanical"),
            index());
        yes(explicit.includes("heating")&&explicit.includes("fire"),
            "Explicit all-subsystem instruction must not be narrowed");
        has(main,"MusaAiDisciplinePlan.focusOnDrawing(raw,reviewPlan,snapshot)");
        System.out.println("MusaAiDisciplinePlanTest OK");
    }
}
