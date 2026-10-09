import com.musa.cad.MusaAiEngineeringBrief;
import com.musa.cad.MusaAiEngineLabel;

public final class MusaAiEngineeringBriefTest {
    private static void need(boolean condition,String message){if(!condition)throw new AssertionError(message);}
    public static void main(String[]args){
        need(MusaAiEngineLabel.display("cloudflare","@cf/qwen/test").contains("Qwen"),"Qwen provider label");
        need(MusaAiEngineLabel.display("gemini","gemini-test").contains("Gemini"),"Gemini provider label");
        need(MusaAiEngineLabel.display("","").contains("doğrulanmadı"),"missing provider must remain unknown");

        MusaAiEngineeringBrief brief=new MusaAiEngineeringBrief();
        brief.addRegion("Bölge 1","BULGU | ÖNCELİK: kritik | PAFTA: Zemin | TESPİT: kesit eksik | KANIT: sheet-tile-1, P1 | İŞLEM: yükleniciden kesit iste");
        brief.addRegion("Bölge 2","CAD varlık sayısı: 5000\\nGörsel çok güzel, genel öneri.");
        String report=brief.build("Proje A","Model","Qwen",4,9,1,5,
            "Diğer bölgeler yanıtlanmadı.","Yerel vektör kontrolü","Ön metraj doğrulanmalı","Poz kitabı bekleniyor");
        need(report.contains("BULGU | ÖNCELİK"),"actionable engineering issue");
        need(report.contains("KESİT")==false||report.contains("kesit eksik"),"verbatim evidence retained");
        need(report.contains("KANIT: sheet-tile-1"),"tile provenance");
        need(report.contains("4/9"),"partial 3x3 scan");
        need(report.contains("KISMİ"),"incomplete scan is never described as complete");
        need(report.contains("DOĞRULANAMAYAN"),"unstructured output must not masquerade as an issue");
        need(!report.contains("Toplam nesne:"),"CAD bookkeeping hidden");
        MusaAiEngineeringBrief empty=new MusaAiEngineeringBrief();
        String unknown=empty.build("A","Model","Bilinmiyor",0,9,0,0,"AI ağ hatası","","","");
        need(unknown.contains("hata olmadığı anlamına GELMEZ"),"lack of evidence is not approval");
        System.out.println("MusaAiEngineeringBriefTest OK");
    }
}
