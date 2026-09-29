package com.musa.cad;

import java.util.*;

/** Detailed discipline and full-project report composer using MusaCAD's BOQ engine. */
public final class MusaAiDetailedReport {
    public enum Level { SUMMARY,TECHNICAL,FULL }

    public static final class Result {
        public final boolean matched;
        public final String title,text;
        public final MusaAiDiscipline discipline;
        public final List<Integer> sourceIds;
        public final int findingCount;
        Result(boolean matched,String title,String text,MusaAiDiscipline discipline,Collection<Integer>ids,int findingCount){
            this.matched=matched;this.title=title==null?"":title;this.text=text==null?"":text;this.discipline=discipline;
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids));this.findingCount=Math.max(0,findingCount);
        }
        public static Result none(){return new Result(false,"","",MusaAiDiscipline.UNKNOWN,Collections.emptyList(),0);}
    }

    public static boolean asksDetailedReport(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return false;
        if(q.contains("tam proje denetim raporu")||q.contains("tum disiplin raporu")||q.contains("detayli proje raporu"))return true;
        MusaAiDiscipline d=MusaAiDiscipline.fromQuery(raw);
        return d!=MusaAiDiscipline.UNKNOWN&&q.contains("rapor")&&
            (q.contains("detay")||q.contains("teknik")||q.contains("inceleme")||q.contains("kontrol")||
             q.contains("statik")||q.contains("mimari")||q.contains("mekanik")||q.contains("elektrik")||
             q.contains("peyzaj")||q.contains("altyapi")||q.contains("asansor")||q.contains("yangin"));
    }

    public static Level levelFrom(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.contains("ozet"))return Level.SUMMARY;
        if(q.contains("tam")||q.contains("detay"))return Level.FULL;
        return Level.TECHNICAL;
    }

    public static Result generate(MusaAiDrawingIndex index,String drawingName,String raw,MusaAiBoq.Model loadedBoq){
        if(index==null)return Result.none();
        String q=MusaAiDrawingIndex.normalize(raw);
        boolean all=q.contains("tam proje")||q.contains("tum disiplin")||q.contains("detayli proje");
        Level level=levelFrom(raw);
        if(all)return generateAll(index,drawingName,loadedBoq,level);
        MusaAiDiscipline d=MusaAiDiscipline.fromQuery(raw);
        if(d==MusaAiDiscipline.UNKNOWN)return Result.none();
        return generateDiscipline(index,drawingName,d,loadedBoq,level);
    }

    public static Result generateDiscipline(MusaAiDrawingIndex index,String drawingName,MusaAiDiscipline d,
                                            MusaAiBoq.Model loadedBoq,Level level){
        MusaAiDisciplineAnalyzer.Result analysis=MusaAiDisciplineAnalyzer.analyzeDiscipline(index,d);
        LinkedHashSet<Integer>ids=new LinkedHashSet<>(analysis.sourceIds);
        String title=d==MusaAiDiscipline.STRUCTURAL?"Statik Proje İnceleme Raporu":d.label+" Proje İnceleme Raporu";
        StringBuilder out=new StringBuilder();
        header(out,title,drawingName,index,level);

        out.append("\n\n1. İNCELEME KAPSAMI");
        out.append("\n• Disiplin: ").append(d.label);
        out.append("\n• İnceleme seviyesi: ").append(levelLabel(level));
        out.append("\n• Kaynak: görünür DWG/DXF vektör geometrisi, katmanlar, metinler ve ölçü metadatası.");
        if(d==MusaAiDiscipline.STRUCTURAL)
            out.append("\n• Bu statik rapor çizim koordinasyonu ve proje verisi ön kontrolüdür. Hesap modeli/raporu olmadan taşıma gücü veya deprem performansı onayı üretmez.");

        out.append("\n\n2. PROJE / ÇİZİM ÖZETİ");
        out.append("\n• Nesne: ").append(index.entityCount);
        out.append("\n• AI indekslenen görünür nesne: ").append(index.items().size());
        out.append("\n• Katman: ").append(index.allLayers.size()).append(" • görünür: ").append(index.visibleLayers.size());
        out.append("\n• Çizim birimi: ").append(index.unitName.isEmpty()?"belirsiz":index.unitName);

        out.append("\n\n3. DİSİPLİN KONTROL BULGULARI");
        appendFindings(out,analysis,level);

        if(d==MusaAiDiscipline.STRUCTURAL){
            StructuralSummary structural=structuralSummary(index);
            ids.addAll(structural.sourceIds);
            out.append("\n\n3A. STATİK TEKNİK VERİ ÖZETİ");
            appendStructuralSummary(out,structural,level);
        }

        MusaAiBoq.Model projectAll=MusaAiBoq.generate(index,blank(drawingName,"Aktif proje")+" • otomatik proje metrajı");
        MusaAiBoq.Model project=filter(projectAll,d);
        MusaAiBoq.Model loaded=filter(loadedBoq,d);

        out.append("\n\n4. PROJEDEN ÜRETİLEN METRAJ / TASLAK KEŞİF");
        appendBoq(out,project,level);

        out.append("\n\n5. PROJE–KEŞİF UYGUNLUĞU");
        if(loadedBoq==null){
            out.append("\n• Bu projeye yüklenmiş keşif bulunmuyor. “Keşif yükle” komutuyla XLSX/CSV keşif bağlanabilir.");
        }else if(loaded==null||loaded.rows.isEmpty()){
            out.append("\n• Yüklenen keşifte ").append(d.label).append(" olarak sınıflandırılabilen satır bulunamadı. Disiplin adları/poz açıklamaları kontrol edilmelidir.");
        }else{
            MusaAiBoq.Comparison cmp=MusaAiBoq.compare(loaded,project);
            appendComparison(out,cmp,level);
        }

        if(level!=Level.SUMMARY){
            out.append("\n\n6. KOORDİNASYON / ÇAPRAZ KONTROL");
            appendCoordinationScope(out,d);
        }

        out.append("\n\n7. SONUÇ");
        out.append("\n• Otomatik bulgu/inceleme adayı: ").append(analysis.findings.size());
        out.append("\n• Çizimden üretilen keşif taslaktır; şartname, mahal listesi, detay ve sözleşme kapsamı ayrıca doğrulanmalıdır.");
        out.append("\n• Rapor proje müellifi, kontrol teşkilatı veya yetkili mühendis onayının yerine geçmez.");
        if(d==MusaAiDiscipline.STRUCTURAL)
            out.append("\n• Statik hesap uygunluğu için hesap raporu/modeli, yük kabulleri, malzeme sınıfları ve deprem parametreleri ayrıca karşılaştırılmalıdır.");

        return new Result(true,title,out.toString(),d,ids,analysis.findings.size());
    }

    public static Result generateAll(MusaAiDrawingIndex index,String drawingName,MusaAiBoq.Model loadedBoq,Level level){
        String title="Tam Proje Denetim ve Uygunluk Raporu";
        StringBuilder out=new StringBuilder();header(out,title,drawingName,index,level);
        MusaAiDisciplineAnalyzer.Result all=MusaAiDisciplineAnalyzer.analyzeAll(index);
        LinkedHashSet<Integer>ids=new LinkedHashSet<>(all.sourceIds);

        out.append("\n\n1. YÖNETİCİ ÖZETİ");
        out.append("\n• İncelenen disiplin: ").append(MusaAiDiscipline.engineering().size());
        out.append("\n• Otomatik bulgu/inceleme adayı: ").append(all.findings.size());
        out.append("\n• Çizimde ilişkilendirilebilir bulgu nesnesi: ").append(ids.size());

        out.append("\n\n2. DİSİPLİN BAZLI BULGULAR");
        for(MusaAiDiscipline d:MusaAiDiscipline.engineering()){
            MusaAiDisciplineAnalyzer.Result r=MusaAiDisciplineAnalyzer.analyzeDiscipline(index,d);
            out.append("\n\n").append(d.label.toUpperCase(new Locale("tr","TR"))).append(" • ").append(r.findings.size()).append(" bulgu");
            if(level!=Level.SUMMARY){
                for(MusaAiDisciplineAnalyzer.Finding f:r.findings)
                    out.append("\n- [").append(f.id).append("] ").append(f.severity).append(" • ").append(f.title).append(" — ").append(f.detail);
            }
        }

        MusaAiBoq.Model project=MusaAiBoq.generate(index,blank(drawingName,"Aktif proje")+" • otomatik proje metrajı");
        out.append("\n\n3. PROJEDEN TASLAK KEŞİF");
        appendBoq(out,project,level);

        out.append("\n\n4. PROJE–KEŞİF KARŞILAŞTIRMASI");
        if(loadedBoq==null)out.append("\n• Yüklenmiş keşif yok.");
        else appendComparison(out,MusaAiBoq.compare(loadedBoq,project),level);

        out.append("\n\n5. DİSİPLİNLER ARASI KOORDİNASYON");
        out.append("\n• Mimari ↔ Statik: açıklıklar, şaftlar, rezervasyonlar, kotlar.");
        out.append("\n• Statik ↔ Mekanik/Elektrik: taşıyıcı eleman geçişleri, delik ve rezervasyonlar.");
        out.append("\n• Mekanik ↔ Elektrik: cihaz beslemeleri, pano/yük, kablo tavası ve bakım alanları.");
        out.append("\n• Peyzaj ↔ Altyapı: drenaj, sulama, rögar, hat güzergâhları ve ağaç kök bölgeleri.");
        out.append("\n• Asansör ↔ Mimari/Statik/Elektrik/Yangın: kuyu, kapı, boşluklar, besleme ve yangın senaryosu.");
        out.append("\n• Yangın ↔ tüm disiplinler: kaçış, sprinkler/hidrant, algılama, duman kontrolü ve itfaiye erişimi.");

        out.append("\n\n6. SONUÇ VE SINIRLAR");
        out.append("\n• MusaCAD AI görünür proje verisinden ön kontrol, koordinasyon ve taslak keşif karşılaştırması üretir.");
        out.append("\n• Yönetmelik, statik güvenlik, nihai keşif/hakediş veya resmi proje onayı yetkili kişilerce doğrulanmalıdır.");

        return new Result(true,title,out.toString(),MusaAiDiscipline.UNKNOWN,ids,all.findings.size());
    }

    private static void appendFindings(StringBuilder out,MusaAiDisciplineAnalyzer.Result analysis,Level level){
        if(analysis.findings.isEmpty()){out.append("\n• Otomatik taramada bulgu üretilmedi.");return;}
        int max=level==Level.SUMMARY?8:(level==Level.TECHNICAL?30:100),n=0;
        for(MusaAiDisciplineAnalyzer.Finding f:analysis.findings){
            if(n++>=max){out.append("\n• … kalan bulgular tam rapor seviyesinde listelenebilir.");break;}
            out.append("\n\n").append(n).append(". [").append(f.id).append("] ").append(f.severity).append(" • ").append(f.title);
            out.append("\nTespit: ").append(f.detail);
            if(!f.suggestion.isEmpty())out.append("\nÖneri: ").append(f.suggestion);
            if(!f.sourceIds.isEmpty())out.append("\nÇizimde ilişkilendirilen nesne: ").append(f.sourceIds.size());
        }
    }

    private static void appendBoq(StringBuilder out,MusaAiBoq.Model model,Level level){
        if(model==null||model.rows.isEmpty()){out.append("\n• Bu disiplin için otomatik metraj satırı üretilemedi.");return;}
        out.append("\n• Kalem: ").append(model.rows.size());
        int max=level==Level.FULL?80:(level==Level.TECHNICAL?30:10),n=0;
        for(MusaAiBoq.Row row:model.rows){
            if(n++>=max){out.append("\n• … kalan ").append(model.rows.size()-max).append(" kalem rapor ekine bırakıldı.");break;}
            out.append("\n• ").append(row.label()).append(" — ").append(number(row.quantity)).append(" ").append(row.unit);
        }
        if(!model.warnings.isEmpty()&&level!=Level.SUMMARY)
            for(String warning:model.warnings)out.append("\nUyarı: ").append(warning);
    }

    private static void appendComparison(StringBuilder out,MusaAiBoq.Comparison cmp,Level level){
        if(cmp==null){out.append("\n• Karşılaştırma üretilemedi.");return;}
        out.append("\n• Eşleştirilen: ").append(cmp.compared);
        out.append("\n• Fark/birim incelemesi: ").append(cmp.different);
        out.append("\n• Keşifte olup eşleşmeyen: ").append(cmp.unmatchedBoq);
        out.append("\n• Projede olup keşifle eşleşmeyen: ").append(cmp.projectOnly);
        if(level!=Level.SUMMARY)out.append("\n\n").append(cmp.text);
    }

    private static MusaAiBoq.Model filter(MusaAiBoq.Model model,MusaAiDiscipline d){
        if(model==null)return null;
        ArrayList<MusaAiBoq.Row> rows=new ArrayList<>();
        for(MusaAiBoq.Row row:model.rows){
            MusaAiDiscipline found=MusaAiDiscipline.classify(row.code+" "+row.description+" "+row.source);
            if(found==d)rows.add(row);
        }
        return MusaAiBoq.of(model.name+" • "+d.label,rows);
    }

    private static void header(StringBuilder out,String title,String drawingName,MusaAiDrawingIndex index,Level level){
        out.append("MUSACAD AI\n").append(title.toUpperCase(new Locale("tr","TR")));
        out.append("\n========================================");
        out.append("\nProje/Dosya: ").append(blank(drawingName,"Aktif çizim"));
        out.append("\nLayout: ").append(blank(index.layout,"aktif layout"));
        out.append("\nRapor seviyesi: ").append(levelLabel(level));
        out.append("\nAI rapor tipi: otomatik proje/keşif ön kontrolü");
    }

    private static void appendCoordinationScope(StringBuilder out,MusaAiDiscipline d){
        switch(d){
            case STRUCTURAL:out.append("\n• Mimari aks/boşluklar, mekanik-elektrik rezervasyonları, asansör kuyusu ve temel/şaft ilişkisi.");break;
            case MECHANICAL:out.append("\n• Mimari şaft/asma tavan, statik kiriş/geçişler, elektrik cihaz beslemeleri ve yangın senaryosu.");break;
            case ELECTRICAL:out.append("\n• Mekanik ekipman beslemeleri, mimari aydınlatma/mahal, yangın algılama ve kablo güzergâhları.");break;
            case ARCHITECTURAL:out.append("\n• Statik taşıyıcı sistem, tesisat şaftları, kaçış/yangın kapıları, asansör ve erişilebilirlik.");break;
            case LANDSCAPE:out.append("\n• Altyapı hatları, drenaj/sulama, aydınlatma, yaya erişimi ve bitki kök bölgeleri.");break;
            case INFRASTRUCTURE:out.append("\n• Mimari/peyzaj kotları, kurum bağlantıları, kazı-dolgu ve diğer yeraltı hatları.");break;
            case ELEVATOR:out.append("\n• Kuyu ve kapı ölçüleri, statik boşluklar, elektrik beslemesi, havalandırma ve yangın senaryosu.");break;
            case FIRE_SAFETY:out.append("\n• Kaçış, kapılar, sprinkler/hidrant, algılama, duman kontrolü, basınçlandırma ve itfaiye erişimi.");break;
            default:out.append("\n• İlgili disiplinlerle koordinasyon kontrolü.");break;
        }
    }

    private static final class StructuralSummary {
        int structuralItems,columns,beams,walls,slabs,foundations,stairs,reservations,detailRefs;
        final LinkedHashSet<String> sections=new LinkedHashSet<>();
        final LinkedHashSet<String> materials=new LinkedHashSet<>();
        final LinkedHashSet<String> rebars=new LinkedHashSet<>();
        final LinkedHashSet<String> levels=new LinkedHashSet<>();
        final LinkedHashSet<String> axes=new LinkedHashSet<>();
        final LinkedHashSet<Integer> sourceIds=new LinkedHashSet<>();
        final ArrayList<String> missing=new ArrayList<>();
    }

    private static StructuralSummary structuralSummary(MusaAiDrawingIndex index){
        StructuralSummary r=new StructuralSummary();
        if(index==null)return r;
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String raw=(item.layer+" "+item.text).trim();
            String q=MusaAiDrawingIndex.normalize(raw);
            boolean structural=MusaAiDiscipline.classify(raw)==MusaAiDiscipline.STRUCTURAL||
                has(q,"kolon","kiris","perde","doseme","temel","radye","kazik","donati","betonarme","rezervasyon","tasiyici");
            if(!structural)continue;
            r.structuralItems++;
            if(item.sourceId>=0)r.sourceIds.add(item.sourceId);

            if(has(q,"temel","radye","kazik","papuc","foundation","pile"))r.foundations++;
            else if(has(q,"kolon","column"))r.columns++;
            else if(has(q,"kiris","beam"))r.beams++;
            else if(has(q,"perde","shear wall"))r.walls++;
            else if(has(q,"doseme","slab"))r.slabs++;
            else if(has(q,"merdiven","stair"))r.stairs++;

            if(has(q,"rezervasyon","bosluk","delik","gecis","opening","sleeve"))r.reservations++;
            if(has(q,"detay","kesit","section","detail"))r.detailRefs++;

            collectMatches(raw,STRUCT_SECTION,r.sections,12);
            collectMatches(raw,STRUCT_CONCRETE,r.materials,10);
            collectMatches(raw,STRUCT_REBAR_CLASS,r.materials,10);
            collectMatches(raw,STRUCT_REBAR_DIAMETER,r.rebars,12);
            if(has(q,"kot","seviye","elevation","level"))collectMatches(raw,STRUCT_LEVEL,r.levels,12);
            if(has(q,"aks","axis","grid"))addSample(r.axes,item.text.isEmpty()?item.layer:item.text,12);
        }
        if(r.columns+r.beams+r.walls+r.slabs+r.foundations==0)
            r.missing.add("Taşıyıcı eleman türleri (kolon/kiriş/perde/döşeme/temel) otomatik ayrıştırılamadı.");
        if(r.sections.isEmpty())
            r.missing.add("Kesit/ebat bilgileri otomatik okunamadı; kolon-kiriş-perde ölçüleri pafta/lejanda ayrıca doğrulanmalı.");
        if(r.materials.isEmpty())
            r.missing.add("Beton veya donatı çeliği sınıfı otomatik okunamadı.");
        if(r.rebars.isEmpty())
            r.missing.add("Donatı çapı/aralığına ilişkin okunabilir metin ipucu bulunamadı.");
        if(r.axes.isEmpty())
            r.missing.add("Aks/grid bilgisi otomatik okunamadı.");
        if(r.levels.isEmpty())
            r.missing.add("Kot/seviye bilgisi otomatik okunamadı.");
        if(r.foundations==0)
            r.missing.add("Temel sistemi bu görünür paftadan güvenilir biçimde tanınamadı.");
        return r;
    }

    private static void appendStructuralSummary(StringBuilder out,StructuralSummary r,Level level){
        out.append("\n• Statik olarak ilişkilendirilen görünür nesne: ").append(r.structuralItems);
        out.append("\n• Eleman envanteri: kolon ").append(r.columns)
           .append(" • kiriş ").append(r.beams)
           .append(" • perde ").append(r.walls)
           .append(" • döşeme ").append(r.slabs)
           .append(" • temel/kazık ").append(r.foundations);
        if(r.stairs>0)out.append(" • merdiven ").append(r.stairs);
        out.append("\n• Rezervasyon/delik/geçiş adayı: ").append(r.reservations);
        out.append(" • detay/kesit referansı: ").append(r.detailRefs);

        if(!r.sections.isEmpty())out.append("\n• Okunan kesit/ebat ipuçları: ").append(joinSamples(r.sections,level==Level.SUMMARY?5:12));
        if(!r.materials.isEmpty())out.append("\n• Okunan malzeme sınıfı ipuçları: ").append(joinSamples(r.materials,level==Level.SUMMARY?4:10));
        if(!r.rebars.isEmpty())out.append("\n• Okunan donatı çapı ipuçları: ").append(joinSamples(r.rebars,level==Level.SUMMARY?5:12));
        if(!r.axes.isEmpty())out.append("\n• Aks/grid ipuçları: ").append(joinSamples(r.axes,level==Level.SUMMARY?4:10));
        if(!r.levels.isEmpty())out.append("\n• Kot/seviye ipuçları: ").append(joinSamples(r.levels,level==Level.SUMMARY?4:10));

        out.append("\n\nEksik / doğrulanması gereken statik veri:");
        if(r.missing.isEmpty())out.append("\n• Otomatik metin taramasında temel veri başlıklarının tümüne ilişkin en az bir ipucu bulundu; bu durum hesap yeterliliği veya doğruluğu anlamına gelmez.");
        else{
            int max=level==Level.SUMMARY?4:r.missing.size();
            for(int i=0;i<Math.min(max,r.missing.size());i++)out.append("\n• ").append(r.missing.get(i));
            if(max<r.missing.size())out.append("\n• … ").append(r.missing.size()-max).append(" ek veri başlığı tam/teknik raporda listelenir.");
        }
        out.append("\n• Statik hesap modeli, yük kombinasyonları, deprem parametreleri, zemin verisi ve eleman kapasiteleri DWG görünür verisinden güvenilir biçimde türetilemez; bunlar ayrı hesap raporu/modeliyle karşılaştırılmalıdır.");
    }

    private static void collectMatches(String raw,java.util.regex.Pattern pattern,Set<String>out,int max){
        if(raw==null||raw.isEmpty()||out.size()>=max)return;
        java.util.regex.Matcher m=pattern.matcher(raw);
        while(m.find()&&out.size()<max){
            String v=m.group().replaceAll("\\s+","").toUpperCase(Locale.ROOT);
            if(!v.isEmpty())out.add(v);
        }
    }

    private static void addSample(Set<String>out,String raw,int max){
        if(out.size()>=max||raw==null)return;
        String v=raw.trim().replaceAll("\\s+"," ");
        if(v.length()>80)v=v.substring(0,80)+"…";
        if(!v.isEmpty())out.add(v);
    }

    private static String joinSamples(Collection<String>values,int max){
        StringBuilder out=new StringBuilder();int n=0;
        for(String v:values){
            if(n++>=max)break;
            if(out.length()>0)out.append(", ");
            out.append(v);
        }
        return out.toString();
    }

    private static boolean has(String q,String...terms){
        if(q==null||q.isEmpty())return false;
        for(String term:terms)if(q.contains(MusaAiDrawingIndex.normalize(term)))return true;
        return false;
    }

    private static final java.util.regex.Pattern STRUCT_SECTION=
        java.util.regex.Pattern.compile("(?i)(?<!\\d)\\d{2,4}\\s*[x×/]\\s*\\d{2,4}(?!\\d)");
    private static final java.util.regex.Pattern STRUCT_CONCRETE=
        java.util.regex.Pattern.compile("(?i)(?<![A-Z0-9])C\\s*\\d{2,3}(?![A-Z0-9])");
    private static final java.util.regex.Pattern STRUCT_REBAR_CLASS=
        java.util.regex.Pattern.compile("(?i)(?<![A-Z0-9])(?:B|S)\\s*\\d{3}[A-Z]?(?![A-Z0-9])");
    private static final java.util.regex.Pattern STRUCT_REBAR_DIAMETER=
        java.util.regex.Pattern.compile("(?i)(?:Ø|Φ|ø|\\bfi\\s*)\\s*\\d{1,2}");
    private static final java.util.regex.Pattern STRUCT_LEVEL=
        java.util.regex.Pattern.compile("[+-]?\\s*\\d{1,3}[\\.,]\\d{1,3}");

    private static String levelLabel(Level l){return l==Level.SUMMARY?"Özet":l==Level.FULL?"Tam Denetim":"Teknik";}
    private static String blank(String s,String f){return s==null||s.trim().isEmpty()?f:s.trim();}
    private static String number(double v){return Double.isFinite(v)?String.format(new Locale("tr","TR"),"%.3f",v).replaceAll("0+$","").replaceAll("[,.]$",""):"—";}
    private MusaAiDetailedReport(){}
}
