package com.musa.cad;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Stage 2 of the Qwen-first visual workflow.
 *
 * The phone compiles an engineering report ONLY after one or more actual
 * successful image requests. Its CAD evidence is an independent cross-check:
 * an existing sourceId does not prove the model's pipe or equipment diagnosis.
 * A 9/9 network response is not a complete engineering approval.
 *
 * Pure Java, offline, deterministic and without additional paid model calls.
 */
public final class MusaAiEngineeringSynthesis {
    private static final Pattern SOURCE=Pattern.compile(
        "(?iu)(?:\\bkaynak\\s*(?:id\\s*)?[:#-]?\\s*|\\bsource\\s*id\\s*[:#-]?\\s*)(\\d{1,9})");
    private static final int MAX_SUMMARY_LINES_PER_GROUP=22;
    private static final int MAX_LOCAL_ITEMS=20;
    private static final int MAX_CAD_SOURCE_MATCHES=24;

    public static final class VisualGroup {
        public final String name;
        public final String response;
        public final int firstRegion, lastRegion;
        public final boolean focused;
        public VisualGroup(String name,String response,int firstRegion,int lastRegion,
                           boolean focused) {
            this.name=clean(name);
            this.response=response==null?"":response.trim();
            this.firstRegion=Math.max(0,firstRegion);
            this.lastRegion=Math.max(this.firstRegion,lastRegion);
            this.focused=focused;
        }
    }

    public static final class Input {
        public String project="",layout="",discipline="";
        public int reviewedTiles, plannedTiles=9, reviewedCloseups, plannedCloseups;
        public String visualIssue="", closeupIssue="", viewInventory="", coverageRegister="";
        public String localVectorEvidence="", quantityEvidence="", priceCandidates="";
        public int sampledCadItems, declaredCadItems;
        public String drawingUnit="";
        public boolean includeTakeoff;
        public final List<VisualGroup> visualGroups=new ArrayList<>();
        /** Only source identities which appeared in the local CAD index. */
        public final Map<Integer,String> sourceDescriptions=new LinkedHashMap<>();

        public void addCadSources(MusaAiDrawingIndex index) {
            if(index==null)return;
            sampledCadItems=index.items().size();
            declaredCadItems=index.entityCount;
            drawingUnit=clean(index.unitName);
            for(MusaAiDrawingIndex.Item item:index.items()) {
                if(item.sourceId<0||sourceDescriptions.containsKey(item.sourceId))continue;
                StringBuilder label=new StringBuilder(clean(item.type));
                if(!clean(item.layer).isEmpty())
                    label.append(" • katman ").append(clean(item.layer));
                if(!clean(item.text).isEmpty())
                    label.append(" • etiket ").append(limit(clean(item.text),115));
                sourceDescriptions.put(item.sourceId,limit(label.toString(),180));
            }
        }
    }

    public static String compose(Input data) {
        if(data==null)throw new IllegalArgumentException("Sentez verisi yok");
        int total=Math.max(1,data.plannedTiles);
        int accepted=Math.max(0,Math.min(total,data.reviewedTiles));
        StringBuilder out=new StringBuilder("GANDALF • GÖRSEL + YEREL MÜHENDİSLİK DENETİM RAPORU");
        out.append("\nProje: ").append(clean(data.project));
        out.append("\nDüzen: ").append(clean(data.layout));
        out.append("\nDisiplin: ").append(clean(data.discipline));
        out.append("\nİşlem sırası: 1) Qwen çevrim içi görsel inceleme → ")
           .append("2) telefonda DWG kanıtlarıyla yerel çapraz kontrol → ")
           .append("3) mühendislik raporu.");
        out.append("\nGörsel AI yanıtı alınan bölge: ").append(accepted)
            .append("/").append(total);
        out.append("\nBaşlık çevresi yakın-plan adayları: ")
            .append(Math.max(0,data.reviewedCloseups)).append("/")
            .append(Math.max(0,data.plannedCloseups));
        if(data.visualGroups.isEmpty()||accepted==0) {
            out.append("\n\nGÖRSEL ANALİZ TAMAMLANMADI");
            out.append("\nQwen'den bu çizime ait doğrulanabilir görsel grup yanıtı alınmadı.");
            out.append("\nYerel inceleme görsel inceleme yerine geçirilmedi; ")
                .append("teknik tespit veya onaylı keşif oluşturulmadı.");
            if(!clean(data.visualIssue).isEmpty())
                out.append("\nKesinti nedeni: ").append(clean(data.visualIssue));
            out.append("\nTekrar denemeden önce HTTPS/AI oturumu veya model hatasını ")
                .append("kontrol edin. Çizim değiştirilmedi.");
            return out.toString();
        }
        boolean allTiles=accepted==total&&clean(data.visualIssue).isEmpty();
        out.append("\nDenetim seviyesi: ")
            .append(allTiles?"Planlanan 9 görsel bölgeye AI yanıtı alındı; mühendis onayı değildir."
                            :"KISMİ; eksik bölgeler hakkında teknik çıkarım yapılmadı.");
        out.append("\nDWG kanıt örneklemi: ").append(Math.max(0,data.sampledCadItems))
            .append("/").append(Math.max(0,data.declaredCadItems))
            .append(" nesne; örneklem dışındaki veriler yerel olarak doğrulanmadı.");
        out.append("\nÖlçü birimi: ")
            .append(clean(data.drawingUnit).isEmpty()?"belirsiz":clean(data.drawingUnit));
        if(!clean(data.visualIssue).isEmpty())
            out.append("\nGörsel sınırlama: ").append(clean(data.visualIssue));
        if(!clean(data.closeupIssue).isEmpty())
            out.append("\nYakın-plan sınırlaması: ").append(clean(data.closeupIssue));

        out.append("\n\n1. YÖNETİCİ ÖZETİ");
        out.append("\n• Çevrim içi görsel analiz yanıtları önce toplandı; ")
            .append("yerel denetim ve rapor birleştirmesi bunlardan sonra yapıldı.");
        out.append("\n• Modelin görsel yorumu, DWG vektör etiketi, kaynak ID'si veya ")
            .append("ölçü doğrulanmadıkça kesin mühendislik tespiti olarak kullanılmaz.");
        out.append("\n• Semboller yalnız çizgi benzerliğinden boru/cihaz olarak ")
            .append("kesinleştirilmez; ilgili pafta ve proje hesaplarıyla kontrol gerekir.");
        if(!allTiles)out.append("\n• Taranmayan bölgeler kapsam dışıdır; ")
            .append("uygunluk veya eksiksizlik kararı verilmez.");

        out.append("\n\n2. QWEN GÖRSEL BULGULARI • KONUMLU VE SADELEŞTİRİLMİŞ");
        Set<String> already=new LinkedHashSet<>();
        Set<Integer> qwenSourceIds=new LinkedHashSet<>();
        int usefulLines=0;
        for(VisualGroup group:data.visualGroups) {
            if(group==null||group.response.isEmpty())continue;
            out.append("\n\n").append(group.name);
            if(group.firstRegion>0)out.append(" [")
                .append(group.focused?"yakın görünüm ":"görüntü bölgesi ")
                .append(group.firstRegion);
            if(group.lastRegion>group.firstRegion)
                out.append("–").append(group.lastRegion);
            if(group.firstRegion>0)out.append("]");
            // A successful group request proves the group was processed, not
            // that every component/line was understood correctly.
            out.append("\nKanıt statüsü: Qwen görsel gözlemi; DWG ve saha teyidi gerekebilir.");
            int shown=0,omitted=0;
            for(String row:group.response.split("\\r?\\n")) {
                String line=stripFormatting(row);
                if(line.isEmpty()||isTemplateNoise(line))continue;
                String key=normalize(line);
                if(!already.add(key))continue;
                Matcher matcher=SOURCE.matcher(line);
                while(matcher.find()) {
                    try {qwenSourceIds.add(Integer.parseInt(matcher.group(1)));}
                    catch(NumberFormatException ignored) {}
                }
                if(shown++<MAX_SUMMARY_LINES_PER_GROUP) {
                    out.append("\n• ").append(limit(line,650));
                    usefulLines++;
                }else omitted++;
            }
            if(shown==0)out.append("\n• Bu gruptan ayırt edilebilir teknik bulgu metni alınamadı.");
            if(omitted>0)out.append("\n• Tekrarsız ilave kayıt: ").append(omitted)
                .append(" (ham Qwen yanıtı bu kısa rapora eklenmedi).");
        }
        if(usefulLines==0)
            out.append("\nQwen'in grup yanıtları teknik bulgu içermedi; ")
               .append("görsel uygunluk kararı verilemez.");

        out.append("\n\n3. YEREL DWG ÇAPRAZ KONTROLÜ • QWEN'DEN SONRA");
        if(qwenSourceIds.isEmpty()) {
            out.append("\nQwen bulgularında kaynak ID belirtilmedi. ")
                .append("İmaj yorumu ile CAD nesnesi birebir bağlanamadı; ")
                .append("bölge/grup referansları mühendis kontrolü için korunur.");
        } else {
            int displayed=0;
            for(Integer id:qwenSourceIds){
                if(displayed++>=MAX_CAD_SOURCE_MATCHES)break;
                String description=data.sourceDescriptions.get(id);
                if(description==null)
                    out.append("\n• Kaynak ").append(id)
                       .append(": DWG örnekleminde bulunamadı; görsel iddia doğrulanmadı.");
                else
                    out.append("\n• Kaynak ").append(id)
                       .append(": DWG nesne kimliği mevcut (").append(description)
                       .append("). Bu eşleşme teknik sınıflandırmanın doğrulandığı anlamına gelmez.");
            }
            if(qwenSourceIds.size()>MAX_CAD_SOURCE_MATCHES)
                out.append("\n• Diğer kaynak kimlikleri: ")
                    .append(qwenSourceIds.size()-MAX_CAD_SOURCE_MATCHES)
                    .append(" (özet dışında).");
        }
        out.append("\nBağımsız yerel çizim kanıtları (yalnız CAD metni/vektörü):");
        appendLocalBullets(out,data.localVectorEvidence);
        if(!clean(data.viewInventory).isEmpty()) {
            out.append("\n\n4. KAT, KESİT, VAZİYET VE KOT KONTROL GÜNDEMİ");
            out.append("\nBaşlık ve kot bilgileri çizim metninden üretilmiştir; ")
               .append("başlık bulunması görünümün görsel olarak incelendiğini kanıtlamaz.");
            appendViewInventory(out,data.viewInventory);
        }

        out.append("\n\n5. MÜHENDİSLİK DEĞERLENDİRMESİ VE SINIRLAR");
        out.append("\n• Qwen'in teknik bulguları yalnız gerçekten yanıt alınan bölgelere aittir.");
        out.append("\n• Yerel analiz ölçü, kaynak ID, blok/katman ve etiketleri ")
            .append("bağımsız olarak destekleyebilir; otomatik olarak mevzuata uygunluk onayı vermez.");
        out.append("\n• DN/PN, montaj malzemesi, kolon düşeyliği, hidrolik hesap, ")
            .append("sızdırmazlık, tesisat güzergâhı ve plan/kesit çelişkileri ")
            .append("ortak referans ve hesapla teyit edilmelidir.");
        out.append("\n• DWG üzerinde değişiklik yapılmadı.");

        if(data.includeTakeoff) {
            out.append("\n\n6. DOĞRULANABİLİR METRAJ VE ÇŞİDB POZ ÖN KONTROLÜ");
            out.append("\nBu bölüm Qwen görsel incelemesinden SONRA yerel olarak üretildi; ")
                .append("onaylı keşif değildir.");
            if(!clean(data.quantityEvidence).isEmpty())
                out.append("\n").append(clean(data.quantityEvidence));
            else out.append("\nBirim/ölçek veya teknik eşleşme doğrulanmadı; güvenilir metraj yok.");
            if(!clean(data.priceCandidates).isEmpty())
                out.append("\n\nResmî poz eşleştirme adayları (kesin poz değildir):\n")
                   .append(clean(data.priceCandidates));
        }else {
            out.append("\n\n6. METRAJ / KEŞİF");
            out.append("\nKullanıcı bu çalışmada metraj/keşif istemedi; ")
                .append("yerel keşif hesaplaması yapılmadı.");
        }
        if(!clean(data.coverageRegister).isEmpty())
            out.append("\n\n7. GÖRÜNÜM VE KAYNAK KAPSAM CETVELİ")
               .append(clean(data.coverageRegister));
        out.append("\n\nSONUÇ: Bu rapor Qwen görsel gözlemleri ile telefonun ")
           .append("DWG kanıtlarının kaynakları karıştırılmadan derlenmiş ön denetimidir.")
           .append(" Teknik imalat kararları, kesin keşif ve mevzuat uygunluğu ")
           .append("sorumlu mühendis tarafından doğrulanmalıdır.");
        return out.toString();
    }

    private static void appendLocalBullets(StringBuilder out,String local) {
        if(clean(local).isEmpty()) {
            out.append("\n• Yerel vektör kanıtı okunamadı.");return;
        }
        int count=0;Set<String> seen=new LinkedHashSet<>();
        for(String row:local.split("\\r?\\n")) {
            String line=stripFormatting(row);
            if(line.isEmpty()||line.startsWith("GANDALF")||
               line.startsWith("Proje:")||line.startsWith("Layout:")||
               line.startsWith("ÖNEMLİ SINIRLAR")||line.startsWith("Kapsam:")||
               line.startsWith("NOT:"))continue;
            // Include evidence lines and existing warning lines, not pages of
            // generic checklist or duplicated text. Never invent CAD values.
            boolean hasEvidence=row.trim().startsWith("•")||
                line.startsWith("OKUNAN ")||line.startsWith("ÇİZİMDEN ")||
                line.startsWith("MÜHENDİSLİK KONTROL")||
                line.startsWith("PAFTA KONUM");
            if(!hasEvidence||!seen.add(normalize(line)))continue;
            if(count++>=MAX_LOCAL_ITEMS)break;
            out.append("\n• ").append(limit(line,450));
        }
        if(count==0)out.append("\n• Yerel vektör örnekleminde kaynaklı teknik tespit yok.");
        if(count>=MAX_LOCAL_ITEMS)out.append("\n• Ek yerel kanıtlar özet dışında bırakıldı.");
    }

    private static void appendViewInventory(StringBuilder out,String viewInventory) {
        int count=0;Set<String> seen=new LinkedHashSet<>();
        for(String line:viewInventory.split("\\r?\\n")) {
            String row=stripFormatting(line);
            if(row.isEmpty()||!seen.add(normalize(row)))continue;
            if(count++>=14)break;
            out.append("\n• ").append(limit(row,300));
        }
        if(count>=14)out.append("\n• Tekrarlanan ve diğer başlık/kot adayları özet dışındadır.");
    }

    private static boolean isTemplateNoise(String value) {
        String n=normalize(value);
        if(n.isEmpty())return true;
        return n.equals("merhaba")||n.equals("tesekkurler")||
            n.startsWith("size nasil yardimci olabilirim")||
            n.startsWith("raporu pdf olarak")||
            n.startsWith("raporu word olarak")||
            n.startsWith("lutfen daha fazla bilgi")||
            n.startsWith("sonucu pdf olarak");
    }

    private static String stripFormatting(String value) {
        if(value==null)return "";
        return clean(value).replaceFirst("^(?:[-*•]+\\s*|\\d+[.)]\\s*)","")
                           .replace("**","").trim();
    }

    private static String normalize(String value){
        String lower=clean(value).toLowerCase(new Locale("tr","TR"));
        return Normalizer.normalize(lower,Normalizer.Form.NFD)
            .replaceAll("\\p{M}+","").replace('ı','i')
            .replaceAll("\\s+"," ").trim();
    }

    private static String clean(String input) {
        return input==null?"":input.trim();
    }

    private static String limit(String input,int max) {
        return input.length()<=max?input:input.substring(0,max-1)+"…";
    }

    private MusaAiEngineeringSynthesis(){}
}
