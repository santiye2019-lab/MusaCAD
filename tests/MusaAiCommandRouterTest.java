import com.musa.cad.MusaAiCommandRouter;

public final class MusaAiCommandRouterTest {
    private static void expect(String phrase,String command){
        MusaAiCommandRouter.Match m=MusaAiCommandRouter.route(phrase);
        if(!m.matched)throw new AssertionError("not matched: "+phrase);
        if(!command.equals(m.command))throw new AssertionError(phrase+": "+m.command+" != "+command);
    }
    private static void none(String phrase){
        if(MusaAiCommandRouter.route(phrase).matched)throw new AssertionError("unexpected command match: "+phrase);
    }

    public static void main(String[]args){
        expect("Ekrana sığdır","ZE");
        expect("tüm çizimi göster","ZE");
        expect("3 boyuta geç","3D");
        expect("2D görünüme dön","2D");
        expect("katmanları aç","LA");
        expect("seçili nesne özelliklerini göster","PR");
        expect("mesafe ölç","DI");
        expect("alanı ölç","AA");
        expect("açı ölç","ANG");
        expect("nokta koordinatını göster","ID");
        expect("yay uzunluğunu ölç","ARCLEN");
        expect("nesne seç","SELECT");
        expect("seçili nesneyi taşı","MOVE");
        expect("nesneyi kopyala","COPY");
        expect("nesneyi sil","ERASE");
        expect("nesneyi döndür","ROTATE");
        expect("nesneyi ölçekle","SCALE");
        expect("aynala","MIRROR");
        expect("offset al","OFFSET");
        expect("dizi oluştur","ARRAY");
        expect("patlat","EXPLODE");
        expect("çizgiyi kes","TRIM");
        expect("çizgiyi uzat","EXTEND");
        expect("köşe yuvarla","FILLET");
        expect("pah kır","CHAMFER");
        expect("nesneyi kır","BREAK");
        expect("çizgileri birleştir","JOIN");
        expect("nesneyi esnet","STRETCH");
        expect("tarama yap","HATCH");
        expect("blok oluştur","BLOCK");
        expect("blok yerleştir","INSERT");
        expect("eşit parçalara böl","DIVIDE");
        expect("revizyon bulutu çiz","REVCLOUD");
        expect("oklu açıklama ekle","MLEADER");
        expect("çizgi çiz","LINE");
        expect("polyline çiz","PLINE");
        expect("daire çiz","CIRCLE");
        expect("yay çiz","ARC");
        expect("elips çiz","ELLIPSE");
        expect("nokta koy","POINT");
        expect("sonsuz çizgi çiz","XLINE");
        expect("dikdörtgen çiz","RECTANG");
        expect("metin ekle","TEXT");
        expect("doğrusal ölçülendir","DLI");
        expect("hizalı ölçülendir","DAL");
        expect("açısal ölçülendir","DAN");
        expect("yarıçap ölçülendir","DRA");
        expect("çap ölçülendir","DDI");
        expect("geri al","UNDO");
        expect("yeniden yap","REDO");
        expect("projeyi kaydet","SAVE");
        none("bu çizimde kaç lavabo var");
        none("metraj çıkar");

        System.out.println("MusaAiCommandRouterTest OK");
    }
}
