package com.musa.cad;

import java.text.Normalizer;
import java.util.Locale;

/** Deterministic Turkish natural-language bridge to the existing MusaCAD CAD command engine. */
public final class MusaAiCommandRouter {
    public static final class Match {
        public final boolean matched;
        public final String command;
        public final String description;
        private Match(boolean matched,String command,String description){
            this.matched=matched;this.command=command;this.description=description;
        }
        public static Match none(){return new Match(false,"","");}
        public static Match of(String command,String description){return new Match(true,command,description);}
    }

    public static Match route(String raw){
        String q=normalize(raw);
        if(q.isEmpty())return Match.none();

        // View/navigation
        if(any(q,"ekrana sigdir","cizimi sigdir","tum cizimi goster","tam cizimi goster","zoom extents","tumunu goster"))
            return m("ZE","Çizimi ekrana sığdır");
        if(any(q,"3d gorunum","3d ac","3 boyuta gec","uc boyut","3 boyut","3d ye gec"))
            return m("3D","3D görünümü aç");
        if(any(q,"2d gorunum","2d ac","2 boyuta gec","iki boyut","2d ye gec"))
            return m("2D","2D görünüme dön");
        if(any(q,"kaydir","pan yap","pan modu","cizimi surukle"))
            return m("PAN","Kaydırma moduna geç");

        // Information/measurement
        if(any(q,"katmanlari goster","katmanlari ac","layerlari goster","layer ac","katman yoneticisi"))
            return m("LA","Katmanları aç");
        if(any(q,"ozellikleri goster","nesne ozellikleri","secili nesne ozellikleri","properties"))
            return m("PR","Seçili nesnenin özelliklerini göster");
        if(any(q,"mesafe olc","uzunluk olc","iki nokta arasi","distance olc"))
            return m("DI","Mesafe ölç");
        if(any(q,"alan olc","alani olc","metrekare olc","area olc"))
            return m("AA","Alan ölç");
        if(any(q,"aci olc","aci hesapla","angle olc"))
            return m("ANG","Açı ölç");
        if(any(q,"koordinat goster","nokta koordinati","koordinatini goster"))
            return m("ID","Nokta koordinatını göster");
        if(any(q,"yay uzunlugu","yay uzunlugunu olc","arc length"))
            return m("ARCLEN","Yay uzunluğunu ölç");

        // Editing
        if(any(q,"nesne sec","secim yap","secme modu","select"))
            return m("SELECT","Nesne seç");
        if(any(q,"secili nesneyi tasi","nesneyi tasi","tasi"))
            return m("MOVE","Seçili nesneyi taşı");
        if(any(q,"secili nesneyi kopyala","nesneyi kopyala","kopyala"))
            return m("COPY","Seçili nesneyi kopyala");
        if(any(q,"secili nesneyi sil","nesneyi sil","sil"))
            return m("ERASE","Seçili nesneyi sil");
        if(any(q,"secili nesneyi dondur","nesneyi dondur","dondur"))
            return m("ROTATE","Seçili nesneyi döndür");
        if(any(q,"nesneyi olcekle","olcekle"))
            return m("SCALE","Seçili nesneyi ölçekle");
        if(any(q,"aynala","ayna al","mirror"))
            return m("MIRROR","Seçili nesneyi aynala");
        if(any(q,"offset al","paralel kopya","ofset al"))
            return m("OFFSET","Offset oluştur");
        if(any(q,"dizi olustur","array yap","cogalt"))
            return m("ARRAY","Dizi oluştur");
        if(any(q,"patlat","explode"))
            return m("EXPLODE","Nesneyi patlat");
        if(any(q,"trim yap","kirp","kesme yap","cizgiyi kes"))
            return m("TRIM","Trim işlemini başlat");
        if(any(q,"cizgiyi uzat","sinira uzat","extend"))
            return m("EXTEND","Extend işlemini başlat");
        if(any(q,"kose yuvarla","fillet","radyus ver"))
            return m("FILLET","Fillet işlemini başlat");
        if(any(q,"pah kir","chamfer","pah yap"))
            return m("CHAMFER","Chamfer işlemini başlat");
        if(any(q,"nesneyi kir","break yap"))
            return m("BREAK","Break işlemini başlat");
        if(any(q,"birleştir","join yap","cizgileri birlestir"))
            return m("JOIN","Nesneleri birleştir");
        if(any(q,"nesneyi esnet","stretch yap","esnet"))
            return m("STRETCH","Stretch işlemini başlat");
        if(any(q,"tarama yap","hatch yap","tarama ekle"))
            return m("HATCH","Hatch işlemini başlat");
        if(any(q,"blok olustur","block olustur"))
            return m("BLOCK","Blok oluştur");
        if(any(q,"blok ekle","blok yerlestir","insert block","insert blok"))
            return m("INSERT","Blok yerleştir");
        if(any(q,"bolumle","divide yap","esit parcalara bol"))
            return m("DIVIDE","Divide işlemini başlat");
        if(any(q,"revizyon bulutu","revcloud","bulut ciz"))
            return m("REVCLOUD","Revizyon bulutu çiz");
        if(any(q,"oklu aciklama","multileader","leader ekle"))
            return m("MLEADER","Multileader ekle");

        // Drawing
        if(any(q,"cizgi ciz","line ciz","duz cizgi"))
            return m("LINE","Çizgi çiz");
        if(any(q,"polyline ciz","poliline ciz","coklu cizgi ciz"))
            return m("PLINE","Polyline çiz");
        if(any(q,"daire ciz","cember ciz"))
            return m("CIRCLE","Daire çiz");
        if(any(q,"yay ciz","arc ciz"))
            return m("ARC","Yay çiz");
        if(any(q,"elips ciz"))
            return m("ELLIPSE","Elips çiz");
        if(any(q,"nokta koy","nokta ciz","point ekle"))
            return m("POINT","Nokta ekle");
        if(any(q,"sonsuz cizgi","xline ciz"))
            return m("XLINE","XLine çiz");
        if(any(q,"dikdortgen ciz","rectangle ciz"))
            return m("RECTANG","Dikdörtgen çiz");
        if(any(q,"yazi ekle","metin ekle","text ekle"))
            return m("TEXT","Metin ekle");

        // Dimensions
        if(any(q,"dogrusal olculendir","lineer olculendir","dimlinear"))
            return m("DLI","Doğrusal ölçülendirme");
        if(any(q,"hizali olculendir","aligned olculendir","dimaligned"))
            return m("DAL","Hizalı ölçülendirme");
        if(any(q,"acisal olculendir","dimangular"))
            return m("DAN","Açısal ölçülendirme");
        if(any(q,"yaricap olculendir","dimradius"))
            return m("DRA","Yarıçap ölçülendirme");
        if(any(q,"cap olculendir","dimdiameter"))
            return m("DDI","Çap ölçülendirme");

        // Session
        if(any(q,"geri al","son islemi geri al","undo"))
            return m("UNDO","Son işlemi geri al");
        if(any(q,"yeniden yap","ileri al","redo"))
            return m("REDO","Geri alınan işlemi yeniden uygula");
        if(any(q,"kaydet","projeyi kaydet","cizimi kaydet"))
            return m("SAVE","Çizimi kaydet");
        if(any(q,"komutlari goster","yardim goster","cad yardim"))
            return m("HELP","CAD komut yardımını aç");

        return Match.none();
    }

    private static Match m(String command,String description){return Match.of(command,description);}

    private static boolean any(String q,String... phrases){
        for(String p:phrases)if(q.equals(p)||q.contains(p))return true;
        return false;
    }

    static String normalize(String raw){
        if(raw==null)return "";
        String s=raw.trim().toLowerCase(new Locale("tr","TR"))
            .replace('ı','i').replace('ğ','g').replace('ü','u').replace('ş','s').replace('ö','o').replace('ç','c');
        s=Normalizer.normalize(s,Normalizer.Form.NFD).replaceAll("\\p{M}+","");
        return s.replaceAll("[^a-z0-9]+"," ").trim().replaceAll("\\s+"," ");
    }

    private MusaAiCommandRouter(){}
}
