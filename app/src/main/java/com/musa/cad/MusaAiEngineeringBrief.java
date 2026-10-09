package com.musa.cad;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;

/** Evidence-first report: only actionable engineering observations reach the main output. */
public final class MusaAiEngineeringBrief {
    private final LinkedHashSet<String> findings=new LinkedHashSet<>();
    private final List<String> unstructured=new ArrayList<>();
    public void addRegion(String region,String response){
        String label=region==null?"Bölge":region.trim();
        if(response==null||response.isEmpty())return;
        int accepted=0;
        for(String line:response.split("\\r?\\n")){
            String value=line.trim().replaceFirst("^[•*\\-\\d.)\\s]+","");
            if(value.isEmpty())continue;
            String upper=value.toUpperCase(Locale.ROOT);
            if(!upper.startsWith("BULGU |")&&!upper.startsWith("KONTROL |"))continue;
            // A model's unsupported generic claim is not promoted to a finding.
            if(value.length()<24||!upper.contains("KANIT:")||!upper.contains("İŞLEM:"))continue;
            if(findings.size()>=45)break;
            findings.add(label+" • "+value);
            accepted++;
        }
        if(accepted==0&&unstructured.size()<3)
            unstructured.add(label+": "+response.trim().replaceAll("\\s+"," ").substring(
                0,Math.min(450,response.trim().replaceAll("\\s+"," ").length())));
    }
    public String build(String project,String layout,String provider,int completed,int total,
                        int focused,int eligible,String limitation,String localEvidence,
                        String estimate,String bookHint){
        StringBuilder out=new StringBuilder("MUSACAD • MÜHENDİSLİK KONTROL RAPORU");
        out.append("\nProje: ").append(project==null?"":project);
        out.append("\nPafta/Layout: ").append(layout==null?"":layout);
        out.append("\nAI motoru: ").append(provider==null?"Belirsiz":provider);
        out.append("\nGörsel kanıt: ").append(completed).append("/").append(total)
           .append(" bölge • Yakın inceleme: ").append(focused).append("/").append(eligible);
        out.append("\n\n1. MÜHENDİSLİK BULGULARI / YAPILACAK İŞLER");
        if(findings.isEmpty()){
            out.append("\n• Görsel modelden kaynak ve işlem bilgisi taşıyan doğrulanabilir bir teknik bulgu alınamadı.");
            out.append("\n• Bu sonuç, projede hata olmadığı anlamına GELMEZ.");
        }else{
            int n=0;for(String line:findings)out.append("\n").append(++n).append(". ").append(line);
        }
        if(!unstructured.isEmpty()){
            out.append("\n\n2. DOĞRULANAMAYAN MODEL YANITLARI (HÜKÜM DEĞİL)");
            for(String line:unstructured)out.append("\n• ").append(line);
        }
        out.append("\n\n3. KAPSAM / EKSİK KANIT");
        if(completed<total||limitation!=null&&!limitation.isEmpty())
            out.append("\n• Görsel inceleme KISMİ. ")
               .append(limitation==null?"Eksik bölgeler ayrıca incelenmelidir.":limitation);
        else out.append("\n• Planlanan 3×3 bölgeler yanıtlandı; bu durum tüm tesisat detaylarının doğrulandığı anlamına gelmez.");
        String verifiedLocal=technicalLocalHighlights(localEvidence);
        if(!verifiedLocal.isEmpty())
            out.append("\n• Yerel DWG kanıtları (görsel AI değil):\n")
               .append(verifiedLocal);
        if(estimate!=null&&!estimate.trim().isEmpty())
            out.append("\n\n4. METRAJ / KEŞİF ÖN KONTROLÜ\n").append(compact(estimate,2200));
        if(bookHint!=null&&!bookHint.trim().isEmpty())
            out.append("\n\n5. RESMÎ POZ KAYNAĞI\n").append(compact(bookHint,1200));
        out.append("\n\nNot: Çap, uzunluk, debi, kot ve poz kodu yalnız açıkça okunmuş CAD/pafta kanıtıyla doğrulanır; belirsiz ölçü uydurulmaz.");
        return out.toString();
    }
    /** Keep equipment/diameter/flow labels, never boilerplate CAD entity counts. */
    private static String technicalLocalHighlights(String raw){
        if(raw==null||raw.isEmpty())return "";
        StringBuilder out=new StringBuilder();
        int accepted=0;
        for(String line:raw.split("\\r?\\n")){
            String value=line.trim();
            if(!value.startsWith("• ")||value.length()>320)continue;
            String upper=value.toUpperCase(Locale.ROOT);
            if(!(upper.contains("DN")||upper.contains("Ø")||upper.contains("POMPA")||
                upper.contains("HAT")||upper.contains("KANAL")||upper.contains("M³")||
                upper.contains("DEBİ")||upper.contains("KAYNAK ")||
                upper.contains("DOĞRULANA")||upper.contains("Q/H")))continue;
            if(accepted++>=10)break;
            if(out.length()>0)out.append("\\n");
            out.append(value);
        }
        return out.toString();
    }
    private static String compact(String input,int max){
        String value=input.trim();
        return value.length()<=max?value:value.substring(0,max)+"… (devamı ayrı inceleme gerektirir)";
    }
    private MusaAiEngineeringBrief(boolean unused){}
    public MusaAiEngineeringBrief(){}
}
