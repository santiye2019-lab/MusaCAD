package com.musa.cad;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.*;

/**
 * Structural calculation/report reader and conservative DWG cross-check.
 *
 * This class compares explicit text/data cues only. It does not reproduce
 * structural analysis, member capacity, seismic performance or code approval.
 */
public final class MusaAiStructuralCalc {
    private static final int MAX_TEXT=2_000_000;
    private static final Pattern SECTION=Pattern.compile("(?i)(?<!\\d)(\\d{2,4})\\s*[x×/]\\s*(\\d{2,4})(?!\\d)");
    private static final Pattern CONCRETE=Pattern.compile("(?i)(?<![A-Z0-9])C\\s*(\\d{2,3})(?![A-Z0-9])");
    private static final Pattern REBAR=Pattern.compile("(?i)(?<![A-Z0-9])([BS])\\s*(\\d{3})([A-Z])?(?![A-Z0-9])");
    private static final Pattern TAG_SECTION=Pattern.compile(
        "(?iu)\\b([A-ZÇĞİÖŞÜ]{1,5}\\s*[-_]?\\s*\\d{1,4})\\b[^\\n\\r]{0,28}?(\\d{2,4}\\s*[x×/]\\s*\\d{2,4})");
    private static final Pattern REBAR_DIAMETER=Pattern.compile("(?iu)(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})");
    private static final Pattern LEVEL=Pattern.compile("[+-]?\\s*\\d{1,3}[\\.,]\\d{1,3}");

    public static final class Model {
        public final String name;
        public final int textLength;
        public final Set<String> concreteGrades,rebarGrades,sections,rebarDiameters,foundationTypes;
        public final Map<String,String> taggedSections;
        public final List<String> seismicCues,loadCues,warnings;

        Model(String name,int textLength,Collection<String>concreteGrades,Collection<String>rebarGrades,
              Collection<String>sections,Collection<String>rebarDiameters,Collection<String>foundationTypes,
              Map<String,String>taggedSections,Collection<String>seismicCues,Collection<String>loadCues,
              Collection<String>warnings){
            this.name=clean(name);
            this.textLength=Math.max(0,textLength);
            this.concreteGrades=immutableSet(concreteGrades);
            this.rebarGrades=immutableSet(rebarGrades);
            this.sections=immutableSet(sections);
            this.rebarDiameters=immutableSet(rebarDiameters);
            this.foundationTypes=immutableSet(foundationTypes);
            this.taggedSections=Collections.unmodifiableMap(new LinkedHashMap<>(taggedSections));
            this.seismicCues=Collections.unmodifiableList(new ArrayList<>(seismicCues));
            this.loadCues=Collections.unmodifiableList(new ArrayList<>(loadCues));
            this.warnings=Collections.unmodifiableList(new ArrayList<>(warnings));
        }

        public boolean isEmpty(){
            return concreteGrades.isEmpty()&&rebarGrades.isEmpty()&&sections.isEmpty()&&
                taggedSections.isEmpty()&&seismicCues.isEmpty()&&loadCues.isEmpty();
        }
    }

    public static final class Comparison {
        public final boolean matched;
        public final String text;
        public final int sameTaggedSections,differentTaggedSections,reportOnlyTags,drawingOnlyTags;
        public final List<Integer> sourceIds;

        Comparison(boolean matched,String text,int sameTaggedSections,int differentTaggedSections,
                   int reportOnlyTags,int drawingOnlyTags,Collection<Integer>sourceIds){
            this.matched=matched;
            this.text=text==null?"":text;
            this.sameTaggedSections=Math.max(0,sameTaggedSections);
            this.differentTaggedSections=Math.max(0,differentTaggedSections);
            this.reportOnlyTags=Math.max(0,reportOnlyTags);
            this.drawingOnlyTags=Math.max(0,drawingOnlyTags);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Comparison none(){return new Comparison(false,"",0,0,0,0,Collections.emptyList());}
    }

    private static final class DrawingSnapshot {
        final LinkedHashSet<String> concrete=new LinkedHashSet<>();
        final LinkedHashSet<String> rebar=new LinkedHashSet<>();
        final LinkedHashSet<String> sections=new LinkedHashSet<>();
        final LinkedHashSet<String> foundations=new LinkedHashSet<>();
        final LinkedHashMap<String,String> tagged=new LinkedHashMap<>();
        final LinkedHashMap<String,Integer> tagSource=new LinkedHashMap<>();
    }

    public static Model parse(String name,String text){
        String raw=text==null?"":text;
        if(raw.length()>MAX_TEXT)raw=raw.substring(0,MAX_TEXT);
        LinkedHashSet<String> concrete=new LinkedHashSet<>(),rebar=new LinkedHashSet<>(),
            sections=new LinkedHashSet<>(),diameters=new LinkedHashSet<>(),foundations=new LinkedHashSet<>();
        LinkedHashMap<String,String>tagged=new LinkedHashMap<>();
        ArrayList<String> seismic=new ArrayList<>(),loads=new ArrayList<>(),warnings=new ArrayList<>();

        collectConcrete(raw,concrete);
        collectRebar(raw,rebar);
        collectSections(raw,sections);
        collectDiameters(raw,diameters);
        collectTagged(raw,tagged);
        collectFoundationTypes(raw,foundations);
        collectCues(raw,seismic,loads);

        if(concrete.isEmpty())warnings.add("Beton sınıfı metinden güvenilir biçimde çıkarılamadı.");
        if(rebar.isEmpty())warnings.add("Donatı çeliği sınıfı metinden güvenilir biçimde çıkarılamadı.");
        if(sections.isEmpty())warnings.add("Kesit/ebat ifadesi bulunamadı.");
        if(tagged.isEmpty())warnings.add("Eleman etiketi ile kesit birlikte eşleştirilemedi; kesit karşılaştırması kısmi olacaktır.");
        if(seismic.isEmpty())warnings.add("Deprem/zemin parametrelerine ilişkin okunabilir metin ipucu bulunamadı.");
        if(raw.trim().isEmpty())warnings.add("Hesap raporundan okunabilir metin elde edilemedi.");

        return new Model(name,raw.length(),concrete,rebar,sections,diameters,foundations,tagged,seismic,loads,warnings);
    }

    public static Comparison compare(MusaAiDrawingIndex index,Model report){
        if(index==null||report==null)return Comparison.none();
        DrawingSnapshot drawing=snapshot(index);
        StringBuilder out=new StringBuilder("STATİK HESAP RAPORU ↔ DWG PROJE KARŞILAŞTIRMASI");
        out.append("\nRapor: ").append(report.name.isEmpty()?"Yüklenen hesap raporu":report.name);
        out.append("\nKarşılaştırma türü: metin/etiket/kesit ve açık proje verisi ön kontrolü");

        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        int same=0,different=0,reportOnly=0,drawingOnly=0;
        ArrayList<String> mismatchLines=new ArrayList<>();

        for(Map.Entry<String,String> e:report.taggedSections.entrySet()){
            String tag=e.getKey(),reportSection=e.getValue(),drawingSection=drawing.tagged.get(tag);
            if(drawingSection==null){reportOnly++;continue;}
            if(reportSection.equals(drawingSection))same++;
            else{
                different++;
                Integer id=drawing.tagSource.get(tag);if(id!=null&&id>=0)ids.add(id);
                if(mismatchLines.size()<30)mismatchLines.add(tag+": rapor "+reportSection+" • DWG "+drawingSection);
            }
        }
        for(String tag:drawing.tagged.keySet())if(!report.taggedSections.containsKey(tag))drawingOnly++;

        out.append("\n\n1. ELEMAN ETİKETİ / KESİT");
        out.append("\n• Aynı etiket ve aynı kesit: ").append(same);
        out.append("\n• Aynı etiket fakat farklı kesit: ").append(different);
        out.append("\n• Hesap raporunda olup DWG'de eşleşmeyen etiket: ").append(reportOnly);
        out.append("\n• DWG'de olup hesap raporunda eşleşmeyen etiket: ").append(drawingOnly);
        for(String line:mismatchLines)out.append("\n• YÜKSEK • ").append(line);
        if(report.taggedSections.isEmpty()||drawing.tagged.isEmpty())
            out.append("\n• Etiketli kesit eşleştirmesi için taraflardan birinde yeterli K1/S1/P1 benzeri etiket + ebat verisi bulunamadı.");

        out.append("\n\n2. MALZEME SINIFLARI");
        appendSetComparison(out,"Beton",report.concreteGrades,drawing.concrete,true);
        appendSetComparison(out,"Donatı çeliği",report.rebarGrades,drawing.rebar,true);

        out.append("\n\n3. KESİT KÜMESİ");
        appendSetComparison(out,"Kesit/ebat",report.sections,drawing.sections,false);

        out.append("\n\n4. TEMEL SİSTEMİ");
        appendSetComparison(out,"Temel tipi",report.foundationTypes,drawing.foundations,false);

        out.append("\n\n5. HESAP RAPORUNDAN OKUNAN TASARIM İPUÇLARI");
        appendSamples(out,"Deprem / zemin",report.seismicCues,10);
        appendSamples(out,"Yük",report.loadCues,8);

        out.append("\n\n6. İNCELEME NOTLARI");
        for(String w:report.warnings)out.append("\n• ").append(w);
        if(different>0)
            out.append("\n• Aynı eleman etiketinde farklı kesit tespitleri proje müellifi tarafından hesap modeli ve son revizyon paftası üzerinden doğrulanmalıdır.");
        if(reportOnly+drawingOnly>0)
            out.append("\n• Tek tarafta görünen etiketler doğrudan eksik eleman kabul edilmez; kat, pafta, isimlendirme ve rapor kapsamı farklı olabilir.");
        out.append("\n• Bu karşılaştırma statik analiz motoru değildir; iç kuvvet, kapasite, düzensizlik, deplasman, performans veya yönetmelik uygunluğu hesabı yapmaz.");

        return new Comparison(true,out.toString(),same,different,reportOnly,drawingOnly,ids);
    }

    public static String summary(Model model){
        if(model==null)return "Statik hesap raporu yüklenmedi.";
        StringBuilder out=new StringBuilder("STATİK HESAP RAPORU ÖZETİ");
        out.append("\n• Dosya: ").append(model.name.isEmpty()?"—":model.name);
        out.append("\n• Okunan metin: ").append(model.textLength).append(" karakter");
        out.append("\n• Beton sınıfları: ").append(join(model.concreteGrades,8));
        out.append("\n• Donatı sınıfları: ").append(join(model.rebarGrades,8));
        out.append("\n• Kesit/ebat: ").append(model.sections.size()).append(" farklı değer");
        out.append("\n• Etiketli kesit: ").append(model.taggedSections.size());
        out.append("\n• Temel tipi: ").append(join(model.foundationTypes,6));
        if(!model.seismicCues.isEmpty())out.append("\n• Deprem/zemin ipucu: ").append(model.seismicCues.get(0));
        if(!model.warnings.isEmpty())out.append("\n• Uyarı: ").append(model.warnings.get(0));
        return out.toString();
    }

    public static boolean isEngineeringTextExtension(String name){
        String ext=extension(name);
        return ext.equals("e2k")||ext.equals("s2k")||ext.equals("f2k")||ext.equals("xml")||
            ext.equals("dat")||ext.equals("out")||ext.equals("rep")||ext.equals("rpt");
    }

    public static String readText(InputStream input)throws IOException{
        if(input==null)throw new IOException("Dosya açılamadı");
        ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buf=new byte[32*1024];int n,total=0;
        while((n=input.read(buf))!=-1){
            int keep=Math.min(n,MAX_TEXT-total);
            if(keep>0)out.write(buf,0,keep);
            total+=keep;if(total>=MAX_TEXT)break;
        }
        byte[] b=out.toByteArray();
        if(b.length>=2&&b[0]==(byte)0xFF&&b[1]==(byte)0xFE)return new String(b,2,b.length-2,StandardCharsets.UTF_16LE);
        if(b.length>=2&&b[0]==(byte)0xFE&&b[1]==(byte)0xFF)return new String(b,2,b.length-2,StandardCharsets.UTF_16BE);
        int off=b.length>=3&&b[0]==(byte)0xEF&&b[1]==(byte)0xBB&&b[2]==(byte)0xBF?3:0;
        return new String(b,off,b.length-off,StandardCharsets.UTF_8);
    }

    private static DrawingSnapshot snapshot(MusaAiDrawingIndex index){
        DrawingSnapshot out=new DrawingSnapshot();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String raw=(item.layer+" "+item.text).trim();
            String norm=MusaAiDrawingIndex.normalize(raw);
            boolean structural=MusaAiDiscipline.classify(raw)==MusaAiDiscipline.STRUCTURAL||
                has(norm,"kolon","kiris","perde","doseme","temel","radye","kazik","donati","betonarme","tasiyici");
            if(!structural)continue;
            collectConcrete(raw,out.concrete);
            collectRebar(raw,out.rebar);
            collectSections(raw,out.sections);
            collectFoundationTypes(raw,out.foundations);
            LinkedHashMap<String,String> found=new LinkedHashMap<>();
            collectTagged(raw,found);
            for(Map.Entry<String,String>e:found.entrySet()){
                out.tagged.putIfAbsent(e.getKey(),e.getValue());
                if(item.sourceId>=0)out.tagSource.putIfAbsent(e.getKey(),item.sourceId);
            }
        }
        return out;
    }

    private static void appendSetComparison(StringBuilder out,String label,Set<String> report,Set<String> drawing,boolean highIfDisjoint){
        out.append("\n• ").append(label).append(" • rapor: ").append(join(report,12)).append(" • DWG: ").append(join(drawing,12));
        if(report.isEmpty()||drawing.isEmpty()){
            out.append("\n  - Taraflardan birinde veri yok; otomatik uyum kararı verilmedi.");
            return;
        }
        LinkedHashSet<String>common=new LinkedHashSet<>(report);common.retainAll(drawing);
        LinkedHashSet<String>onlyReport=new LinkedHashSet<>(report);onlyReport.removeAll(drawing);
        LinkedHashSet<String>onlyDrawing=new LinkedHashSet<>(drawing);onlyDrawing.removeAll(report);
        if(common.isEmpty()){
            out.append("\n  - ").append(highIfDisjoint?"YÜKSEK":"ORTA").append(" • Ortak değer bulunmadı; revizyon ve kapsam kontrolü gerekli.");
        }else{
            out.append("\n  - Ortak: ").append(join(common,12));
            if(!onlyReport.isEmpty())out.append("\n  - Yalnız rapor: ").append(join(onlyReport,12));
            if(!onlyDrawing.isEmpty())out.append("\n  - Yalnız DWG: ").append(join(onlyDrawing,12));
        }
    }

    private static void appendSamples(StringBuilder out,String label,List<String>values,int max){
        out.append("\n• ").append(label).append(":");
        if(values.isEmpty()){out.append(" okunabilir ipucu yok.");return;}
        for(int i=0;i<Math.min(max,values.size());i++)out.append("\n  - ").append(values.get(i));
    }

    private static void collectConcrete(String raw,Set<String>out){
        Matcher m=CONCRETE.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<30)out.add("C"+m.group(1));
    }
    private static void collectRebar(String raw,Set<String>out){
        Matcher m=REBAR.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<30)out.add(m.group(1).toUpperCase(Locale.ROOT)+m.group(2)+(m.group(3)==null?"":m.group(3).toUpperCase(Locale.ROOT)));
    }
    private static void collectSections(String raw,Set<String>out){
        Matcher m=SECTION.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<250)out.add(section(m.group(1),m.group(2)));
    }
    private static void collectDiameters(String raw,Set<String>out){
        Matcher m=REBAR_DIAMETER.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<60)out.add("Ø"+m.group(1));
    }
    private static void collectTagged(String raw,Map<String,String>out){
        Matcher m=TAG_SECTION.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<500){
            Matcher s=SECTION.matcher(m.group(2));
            if(!s.find())continue;
            String tag=canonicalTag(m.group(1));
            if(tag.matches("C\\d{2,3}")||tag.matches("[BS]\\d{3}[A-Z]?"))continue;
            out.putIfAbsent(tag,section(s.group(1),s.group(2)));
        }
    }
    private static void collectFoundationTypes(String raw,Set<String>out){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(has(q,"radye","raft"))out.add("RADYE");
        if(has(q,"fore kazik","kazik temel","pile foundation","bored pile"))out.add("KAZIK");
        if(has(q,"tekil temel","isolated footing","münferit temel","munferit temel"))out.add("TEKİL");
        if(has(q,"surekli temel","mütemadi temel","mutemadi temel","strip footing"))out.add("SÜREKLİ");
        if(has(q,"birlesik temel","combined footing"))out.add("BİRLEŞİK");
    }

    private static void collectCues(String raw,List<String>seismic,List<String>loads){
        if(raw==null)return;
        for(String line:raw.replace('\r','\n').split("\n")){
            String t=line.trim().replaceAll("\\s+"," ");
            if(t.isEmpty())continue;if(t.length()>180)t=t.substring(0,180)+"…";
            String q=MusaAiDrawingIndex.normalize(t);
            if(seismic.size()<24&&has(q,"sds","sd1","dts","bys","zemin sinifi","deprem","spektrum","bina onem","tasarim spektrumu","ra r","dayanim fazlaligi"))
                addUnique(seismic,t);
            if(loads.size()<20&&has(q,"hareketli yuk","sabit yuk","kar yuku","ruzgar yuku","duvar yuku","live load","dead load","snow load","wind load"))
                addUnique(loads,t);
        }
    }

    private static void addUnique(List<String>out,String value){if(!out.contains(value))out.add(value);}
    private static String section(String a,String b){return Integer.parseInt(a)+"x"+Integer.parseInt(b);}
    private static String canonicalTag(String raw){return raw==null?"":raw.toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");}
    private static String join(Collection<String>values,int max){
        if(values==null||values.isEmpty())return "—";StringBuilder out=new StringBuilder();int n=0;
        for(String v:values){if(n++>=max){out.append(", …");break;}if(out.length()>0)out.append(", ");out.append(v);}
        return out.toString();
    }
    private static boolean has(String q,String...terms){for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;}
    private static Set<String>immutableSet(Collection<String>source){return Collections.unmodifiableSet(new LinkedHashSet<>(source));}
    private static String extension(String name){if(name==null)return "";int dot=name.lastIndexOf('.');return dot>=0&&dot+1<name.length()?name.substring(dot+1).toLowerCase(Locale.ROOT):"";}
    private static String clean(String value){return value==null?"":value.trim();}
    private MusaAiStructuralCalc(){}
}
