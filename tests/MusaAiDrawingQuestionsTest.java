import com.musa.cad.MusaAiDrawingIndex;
import com.musa.cad.MusaAiDrawingQuestions;
import java.util.*;

public final class MusaAiDrawingQuestionsTest {
    private static void expectContains(MusaAiDrawingIndex index,String q,String...parts){
        MusaAiDrawingQuestions.Answer a=MusaAiDrawingQuestions.answer(index,q);
        if(!a.matched)throw new AssertionError("not matched: "+q);
        for(String p:parts)if(!a.text.contains(p))throw new AssertionError(q+" missing: "+p+"\n"+a.text);
    }
    private static void none(MusaAiDrawingIndex index,String q){
        if(MusaAiDrawingQuestions.answer(index,q).matched)throw new AssertionError("unexpected match: "+q);
    }

    public static void main(String[]args){
        List<MusaAiDrawingIndex.Item>items=Arrays.asList(
            new MusaAiDrawingIndex.Item("LINE","PIS_SU","Ø100 PİS SU"),
            new MusaAiDrawingIndex.Item("LINE","PIS_SU",""),
            new MusaAiDrawingIndex.Item("CIRCLE","SIHHI_CIHAZ","LAVABO"),
            new MusaAiDrawingIndex.Item("CIRCLE","SIHHI_CIHAZ","LAVABO"),
            new MusaAiDrawingIndex.Item("TEXT","NOTLAR","Lavabo sıcak su bağlantısı"),
            new MusaAiDrawingIndex.Item("MTEXT","NOTLAR","Pis su kolon detayı"),
            new MusaAiDrawingIndex.Item("LWPOLYLINE","DUVAR",""),
            new MusaAiDrawingIndex.Item("POLYLINE","DUVAR",""),
            new MusaAiDrawingIndex.Item("HATCH","DOLGU","")
        );
        MusaAiDrawingIndex index=new MusaAiDrawingIndex(
            "Model",9,1,
            Arrays.asList("PIS_SU","SIHHI_CIHAZ","NOTLAR","DUVAR","DOLGU"),
            Arrays.asList("PIS_SU","SIHHI_CIHAZ","NOTLAR","DUVAR","DOLGU"),
            items
        );

        expectContains(index,"Bu çizimde neler var?","Çizim özeti","Nesne: 9","Katman: 5");
        expectContains(index,"hangi katmanlar var","PIS_SU","SIHHI_CIHAZ");
        expectContains(index,"kaç daire var","Daire adedi: 2");
        expectContains(index,"kaç polyline var","Polyline adedi: 2");
        expectContains(index,"kaç yazı var","Metin adedi: 5");
        expectContains(index,"lavabo kelimesi kaç yerde geçiyor","“lavabo”","3 kez");
        expectContains(index,"pis su geçen katmanları göster","PIS_SU","NOTLAR");
        expectContains(index,"çizimde lavabo var mı","eşleşmesi bulundu");
        expectContains(index,"hangi yazılar var","LAVABO","Pis su kolon detayı");
        none(index,"sprinkler metrajı çıkar");

        System.out.println("MusaAiDrawingQuestionsTest OK");
    }
}
