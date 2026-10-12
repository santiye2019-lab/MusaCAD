import com.musa.cad.*;
import java.util.*;

public final class MusaAiVisualDisciplineTest {
    private static void equal(String actual,String expected){
        if(!Objects.equals(actual,expected))
            throw new AssertionError("expected "+expected+" but got "+actual);
    }
    private static void has(String actual,String expected){
        if(!actual.contains(expected))
            throw new AssertionError("missing: "+expected+" in "+actual);
    }
    private static void notHas(String actual,String unexpected){
        if(actual.contains(unexpected))
            throw new AssertionError("unexpected: "+unexpected+" in "+actual);
    }
    private static MusaAiDrawingIndex index(String... layers){
        return new MusaAiDrawingIndex("Model",25,0,
            Arrays.asList(layers),Arrays.asList(layers),
            Collections.emptyList(),"mm");
    }
    public static void main(String[]args){
        equal(MusaAiVisualDiscipline.resolve("all","projeyi görsel analiz et",
            "TESİSAT - SİLİVRİKAPI KAFE MUTFAK.dwg",index("M-KESIT")),
            "mechanical");
        equal(MusaAiVisualDiscipline.resolve("all","projeyi incele",
            "STATİK KALIP PLANI.dwg",index("MIM-KAPI")),"structural");
        equal(MusaAiVisualDiscipline.resolve("all","projeyi incele",
            "ELEKTRİK KUVVET PLANI.dwg",index("MIM-KAPI")),"electrical");
        equal(MusaAiVisualDiscipline.resolve("architectural","mimariyi incele",
            "TESİSAT MUTFak.dwg",index("MEK-TEMIZ")),"architectural");
        equal(MusaAiVisualDiscipline.resolve("all","tüm disiplinleri analiz et",
            "TESİSAT.dwg",index("MEK-BORU")),"all");
        equal(MusaAiVisualDiscipline.resolve("all","projeyi incele",
            "proje_01.dwg",index("MEK-BORU","MEK-HIDROFOR","MEK-SU","MIM-KAPI")),"mechanical");
        equal(MusaAiVisualDiscipline.resolve("all","projeyi incele",
            "proje_01.dwg",index("MEK-BORU","MIM-DUVAR","ELK-PANO")),"all");
        equal(MusaAiVisualDiscipline.resolve("all","projeyi incele",
            "proje_01.dwg",index("MEK-BORU")),"all");
        equal(MusaAiVisualDiscipline.localDiscipline("fire").name(),"FIRE_SAFETY");
        if(MusaAiVisualDiscipline.mechanicalScope("fire"))
            throw new AssertionError("Fire must use fire-safety checks");
        if(!MusaAiVisualDiscipline.mechanicalScope("wastewater"))
            throw new AssertionError("Wastewater should retain mechanical vector reader");

        String statik=MusaAiVisualDiscipline.visionInstruction("structural");
        has(statik,"kolon-kiriş-perde-döşeme-temel");
        has(statik,"yalnız bu disiplinin");
        notHas(statik,"hidrofor");
        has(MusaAiVisualDiscipline.visionInstruction("electrical"),"topraklama");
        has(MusaAiVisualDiscipline.visionInstruction("architectural"),"erişilebilirlik");
        has(MusaAiVisualDiscipline.visionInstruction("mechanical"),"sıhhi tesisat");
        has(MusaAiVisualDiscipline.visionInstruction("all"),"ayrı alt bölümlere");
        equal(MusaAiVisualDiscipline.heading("structural"),
            "STATİK GÖRSEL + YEREL MÜHENDİSLİK DENETİM RAPORU");

        MusaAiEngineeringSynthesis.Input d=new MusaAiEngineeringSynthesis.Input();
        d.project="statik-kalıp.dwg";d.layout="Model";d.scope="structural";
        d.discipline=MusaAiAnalysisIntent.label(d.scope);
        d.plannedTiles=1;d.reviewedTiles=1;
        d.visualGroups.add(new MusaAiEngineeringSynthesis.VisualGroup(
            "Qwen grup 1","• Statik aks/kiriş notu görünüyor",1,1,false));
        String report=MusaAiEngineeringSynthesis.compose(d);
        has(report,"GANDALF • STATİK GÖRSEL + YEREL MÜHENDİSLİK DENETİM RAPORU");
        has(report,"Bu disiplin için kontrol odağı");
        notHas(report,"DN/PN, montaj malzemesi");
        has(report,"DİSİPLİN KAPSAMI: Ana teknik sonuçlar Statik");
        d.scope="mechanical";d.discipline="Mekanik tesisat";
        report=MusaAiEngineeringSynthesis.compose(d);
        has(report,"MEKANİK TESİSAT GÖRSEL");
        has(report,"DN/PN, montaj malzemesi");
        System.out.println("MusaAiVisualDisciplineTest OK");
    }
}
