package com.musa.cad;

import java.util.Locale;

/**
 * Human-readable evidence register for a CAD review. A title near a plan is
 * NOT proof of its actual sheet boundary, full visual inspection or correctness.
 * This register remains useful even when the cloud provider returns HTTP 429.
 */
public final class MusaAiPaftaCoverage {
    public static String build(MusaAiDrawingIndex index,MusaAiViewCatalog.Result catalog,
                               int visualTilesReviewed,int visualTilesPlanned,
                               int closeupsReviewed,int closeupsPlanned,String interruption){
        StringBuilder out=new StringBuilder("\n\nPAFTA / GORUNUM KAPSAM VE KANIT CETVELI");
        if(index==null||catalog==null){
            return out.append("\nCAD vektör indeksi veya görünüm envanteri yok.").toString();
        }
        out.append("\nAktif DWG düzeni: ").append(index.layout.isEmpty()?"Model":index.layout);
        out.append("\nÖrneklenen CAD öğesi: ").append(index.items().size());
        out.append(" / çizimde bildirilen toplam: ").append(index.entityCount);
        if(index.items().size()<index.entityCount)
            out.append(" (vektör veri örneklemi; tam sayısal denetim değildir)");
        out.append("\nDWG yazılarından bulunan görünüm başlığı adayı: ")
            .append(catalog.views.size());
        out.append("\nAI yanıtı doğrulanan genel görüntü bölgesi: ")
            .append(Math.max(0,visualTilesReviewed)).append("/")
            .append(Math.max(0,visualTilesPlanned));
        out.append("\nAI yanıtı alınan başlık çevresi yakın-planı: ")
            .append(Math.max(0,closeupsReviewed)).append("/")
            .append(Math.max(0,closeupsPlanned));
        if(!interruption.isEmpty())out.append("\nKesinti / sınırlama: ").append(interruption);
        if(catalog.views.isEmpty()){
            out.append("\nGörünüm başlığı güvenilir şekilde tespit edilmedi; paftalar atlanmış olabilir.");
        }else{
            int i=0;
            for(MusaAiViewCatalog.View view:catalog.views){
                if(++i>30){out.append("\nEk başlık adayları liste sınırı dışında.");break;}
                out.append("\n").append(i).append(". ").append(view.kind)
                    .append(" • ").append(view.title);
                if(view.sourceId>=0)out.append(" [DWG kaynak ").append(view.sourceId).append("]");
                if(view.positioned())
                    out.append(" [başlık X=").append(number(view.x))
                        .append(", Y=").append(number(view.y)).append("]");
                else out.append(" [başlık konumu bilinmiyor]");
                out.append("\n   Kanıt: yalnız yazı/vektör başlığı. Pafta sınırı ve bu paftanın")
                    .append(" görsel kapsamı ayrı doğrulanmadı.");
            }
        }
        if(catalog.truncated)
            out.append("\nUYARI: Başlık/kot araması örnekleme sınırına ulaştı; envanter eksik olabilir.");
        out.append("\nSONUÇ: Genel görsel bölgelerin tamamlanması, her kat/kesit/vaziyet")
            .append(" paftasının tüm detaylarının incelendiği anlamına gelmez.");
        out.append(" Kaynak ID ile ilişkilendirilemeyen çıkarımlar doğrulanmış bulgu değildir.");
        return out.toString();
    }
    private static String number(double n){
        return Double.isFinite(n)?String.format(Locale.ROOT,"%.2f",n):"?";
    }
    private MusaAiPaftaCoverage(){}
}
