import com.musa.cad.MusaAiEngineeringSynthesis;

public final class MusaAiEngineeringSynthesisTest {
    static void yes(boolean value,String why) {
        if(!value)throw new AssertionError(why);
    }
    static void contains(String out,String s) {
        yes(out.contains(s),"Missing: "+s);
    }
    static void notContains(String out,String s) {
        yes(!out.contains(s),"Unexpected: "+s);
    }
    static int count(String text,String needle) {
        int total=0,at=0;
        while((at=text.indexOf(needle,at))>=0) {
            total++;at+=needle.length();
        }
        return total;
    }
    static MusaAiEngineeringSynthesis.Input input() {
        MusaAiEngineeringSynthesis.Input d=new MusaAiEngineeringSynthesis.Input();
        d.project="Silivrikapı Kafe Mutfak";
        d.layout="Model";d.discipline="Mekanik tesisat";
        d.plannedTiles=9;d.reviewedTiles=4;
        d.sampledCadItems=5000;d.declaredCadItems=27631;
        d.viewInventory="ZEMİN KAT UYGULAMA PLANI\n+18.50\n+18.50";
        d.coverageRegister="\nAI yanıtı doğrulanan genel görüntü bölgesi: 4/9";
        d.localVectorEvidence="GANDALF • MÜHENDİSLİK PAFTA ÖN İNCELEMESİ\n"+
            "OKUNAN EKİPMAN / SEMBOL AÇIKLAMALARI\n"+
            "• Hidrofor [kaynak 184] — Q/H bu etikette okunamadı\n"+
            "• Klozet [kaynak 200] — blok adı okundu\n"+
            "ÖNEMLİ SINIRLAR: uygunluk belgesi değildir.";
        d.sourceDescriptions.put(184,"INSERT • katman MEK-S-HIDROFOR");
        d.visualGroups.add(new MusaAiEngineeringSynthesis.VisualGroup(
            "Qwen genel görsel grup 1/3",
            "• Görsel gözlem: hidrofor bağlantı etiketi [kaynak 184] üzerinde Q/H okunamıyor.\n"+
            "• Görsel gözlem: hidrofor bağlantı etiketi [kaynak 184] üzerinde Q/H okunamıyor.\n"+
            "• Bir armatür simgesi olabilir, ancak boru çapı okunamadı [kaynak 99999].",
            1,4,false));
        return d;
    }
    public static void main(String[]args) {
        MusaAiEngineeringSynthesis.Input d=input();
        d.visualIssue="İkinci görsel grup yanıt vermedi.";
        String out=MusaAiEngineeringSynthesis.compose(d);
        contains(out,"GANDALF • GÖRSEL + YEREL MÜHENDİSLİK DENETİM RAPORU");
        contains(out,"Görsel AI yanıtı alınan bölge: 4/9");
        contains(out,"KISMİ");
        contains(out,"Qwen genel görsel grup 1/3");
        contains(out,"DWG nesne kimliği mevcut");
        contains(out,"Kaynak 99999: DWG örnekleminde bulunamadı");
        contains(out,"YEREL DWG ÇAPRAZ KONTROLÜ • QWEN'DEN SONRA");
        contains(out,"Hidrofor [kaynak 184]");
        contains(out,"Ölçü birimi: belirsiz");
        contains(out,"AI yanıtı doğrulanan genel görüntü bölgesi: 4/9");
        contains(out,"Kullanıcı bu çalışmada metraj/keşif istemedi");
        notContains(out,"MALZEME METRAJ / ÇŞİDB POZ KEŞİF ÖN RAPORU");
        yes(count(out,"Görsel gözlem: hidrofor bağlantı etiketi") == 1,
            "Repeat technical evidence only once in summary");
        yes(out.indexOf("Qwen genel görsel") <
            out.indexOf("YEREL DWG ÇAPRAZ KONTROLÜ"),
            "Qwen must be first in the final report");

        MusaAiEngineeringSynthesis.Input full=input();
        full.reviewedTiles=9;full.visualGroups.add(
            new MusaAiEngineeringSynthesis.VisualGroup(
                "Qwen genel görsel grup 2/3",
                "• Bölge 5: temiz su hattı adayı, çap etiketi okunamadı",5,8,false));
        full.visualGroups.add(
            new MusaAiEngineeringSynthesis.VisualGroup(
                "Qwen genel görsel grup 3/3",
                "• Bölge 9: kot etiketi görünür; referans noktası doğrulanmalı",9,9,false));
        full.includeTakeoff=true;
        full.drawingUnit="";
        full.quantityEvidence="Çizim birimi doğrulanamadı; boru metrajı üretilmedi.";
        full.priceCandidates="2025 kitabından eşleşen doğrulanmış poz bulunamadı.";
        String fullReport=MusaAiEngineeringSynthesis.compose(full);
        contains(fullReport,"9/9");
        contains(fullReport,"mühendis onayı değildir");
        contains(fullReport,"6. DOĞRULANABİLİR METRAJ");
        contains(fullReport,"Çizim birimi doğrulanamadı");
        contains(fullReport,"2025 kitabından eşleşen");
        notContains(fullReport,"0/9");

        MusaAiEngineeringSynthesis.Input noVisual=input();
        noVisual.reviewedTiles=0;noVisual.visualGroups.clear();
        noVisual.visualIssue="Worker DNS çözümlenemedi";
        noVisual.includeTakeoff=true;
        noVisual.quantityEvidence="Sahte imalat metrajı 103 m";
        String missing=MusaAiEngineeringSynthesis.compose(noVisual);
        contains(missing,"GÖRSEL ANALİZ TAMAMLANMADI");
        contains(missing,"0/9");
        contains(missing,"Worker DNS çözümlenemedi");
        notContains(missing,"Sahte imalat");
        notContains(missing,"Hidrofor [kaynak 184]");
        notContains(missing,"YEREL DWG ÇAPRAZ KONTROLÜ");
        System.out.println("MusaAiEngineeringSynthesisTest OK");
    }
}
