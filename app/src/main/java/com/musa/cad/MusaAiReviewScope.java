package com.musa.cad;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Natural-language scope for an engineering review. The scope affects the
 * review questions, not the CAD file itself; no geometry is changed.
 */
public final class MusaAiReviewScope {
    public final MusaAiDiscipline discipline;
    public final String system;
    public final String label;

    private MusaAiReviewScope(MusaAiDiscipline discipline,String system,String label){
        this.discipline=discipline;this.system=system;this.label=label;
    }

    public static boolean asksAnalysis(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty()||q.startsWith("mekai ")||q.startsWith("gmekai ")||
            q.startsWith("statikai ")||q.startsWith("gstatikai ")||
            q.startsWith("gstatik ")||q.startsWith("ai ")||
            q.contains("raporu pdf")||q.contains("raporu word")||
            q.contains("projeden kesif")||q.contains("metraj cikar"))return false;
        boolean action=has(q,"analiz","incele");
        boolean subject=has(q,"proje","cizim","pafta"," olarak ",
            "sihhi tesisat","pis su tesisat","temiz su tesisat","havalandirma",
            "yangin tesisat","klima tesisat","elektrik tesisat");
        return action&&subject;
    }

    public static boolean offlineRequested(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return has(q,"yerel","cevrim disi","cevrimdisi","internetsiz","offline");
    }

    public static MusaAiReviewScope parse(String raw){
        if(!asksAnalysis(raw))return null;
        String q=" "+MusaAiDrawingIndex.normalize(raw)+" ";
        if(has(q,"tam proje","tum disiplin","disiplinler arasi","hepsini analiz"))
            return new MusaAiReviewScope(MusaAiDiscipline.UNKNOWN,"ALL","Tüm disiplinler");
        if(has(q,"pis su","atik su","atiksu","foseptik"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"WASTEWATER","Pis su tesisatı");
        if(has(q,"temiz su","sicak su","soguk su"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"WATER","Temiz / sıcak su tesisatı");
        if(has(q,"sihhi tesisat","sihhi olarak","sihhi proje"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"SANITARY","Sıhhi tesisat");
        if(has(q,"havalandirma","hava kanali","spiro","kanal kesit"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"VENTILATION","Havalandırma");
        if(has(q,"isitma","kalorifer","radyator"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"HEATING","Isıtma");
        if(has(q,"klima","sogutma","vrf","iklimlendirme"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"COOLING","Klima / soğutma");
        if(has(q,"dogalgaz","dogal gaz"))
            return new MusaAiReviewScope(MusaAiDiscipline.MECHANICAL,"GAS","Doğalgaz");
        if(has(q,"yangin tesisat","yangin pomp","sprinkler","hidrant"))
            return new MusaAiReviewScope(MusaAiDiscipline.FIRE_SAFETY,"FIRE","Yangın tesisatı");
        MusaAiDiscipline d=MusaAiDiscipline.fromQuery(raw);
        return new MusaAiReviewScope(d,"ALL",d==MusaAiDiscipline.UNKNOWN?"Tüm disiplinler":d.label);
    }

    /** Local evidence is an intentionally incomplete, source-linked review, not a visual diagnosis. */
    public String localEvidence(MusaAiDrawingIndex index,String fileName){
        if(index==null)return "Vektör çizim verisi henüz hazır değil.";
        if(discipline==MusaAiDiscipline.MECHANICAL||"FIRE".equals(system)){
            MusaAiDrawingIndex scoped=filteredIndex(index);
            return "İstenen kapsam: "+label+"\n"+
                MusaAiEngineeringReview.analyze(scoped,fileName).text;
        }
        if(discipline!=MusaAiDiscipline.UNKNOWN){
            return "İstenen kapsam: "+label+"\n"+
                MusaAiDisciplineAnalyzer.analyzeDiscipline(index,discipline).text;
        }
        return MusaAiDisciplineAnalyzer.analyzeAll(index).text;
    }

    /**
     * A bounded local filter. Only positively associated entities are included;
     * unlike the cloud context, the index is not used to claim a complete survey.
     */
    public MusaAiDrawingIndex filteredIndex(MusaAiDrawingIndex index){
        if(index==null||"ALL".equals(system))return index;
        String[] terms=termsFor(system);
        if(terms.length==0)return index;
        List<MusaAiDrawingIndex.Item> matched=new ArrayList<>();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=" "+MusaAiDrawingIndex.normalize(item.layer+" "+item.text)+" ";
            if(has(hay,terms))matched.add(item);
        }
        return new MusaAiDrawingIndex(index.layout,index.entityCount,index.oleObjectCount,
            index.allLayers,index.visibleLayers,matched,index.unitName,index.oleItems());
    }

    private static String[] termsFor(String system){
        switch(system){
            case "SANITARY": return new String[]{"sihhi","pis su","atiksu","atik su","temiz su",
                "sicak su","soguk su","lavabo","klozet","rezervuar","dus","hidrofor","boyler","wc"};
            case "WASTEWATER": return new String[]{"pis su","atiksu","atik su","kanalizasyon","pissu","foseptik"};
            case "WATER": return new String[]{"temiz su","sicak su","soguk su","hidrofor","boyler","kullanma suyu"};
            case "VENTILATION": return new String[]{"havalandirma","hava kanali","kanal kesit","spiro","egzoz","taze hava","fan"};
            case "HEATING": return new String[]{"isitma","kalorifer","radyator","kazan","yerden isitma"};
            case "COOLING": return new String[]{"klima","sogutma","vrf","chiller","fancoil","fan coil"};
            case "GAS": return new String[]{"dogalgaz","dogal gaz","gaz tesisat","regulator","solenoid"};
            case "FIRE": return new String[]{"yangin","sprinkler","hidrant","jokey pompa","yangin pomp","itfaiye"};
            default: return new String[0];
        }
    }

    private static boolean has(String q,String... terms){
        for(String term:terms)if(q.contains(term))return true;
        return false;
    }
}
