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
    private static final Pattern TAG_TOKEN=Pattern.compile("(?iu)\\b([A-ZÇĞİÖŞÜ]{1,8}\\s*[-_]?\\s*\\d{1,4})\\b");
    private static final Pattern REBAR_DIAMETER=Pattern.compile("(?iu)(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})");
    private static final Pattern LONGITUDINAL_REBAR=Pattern.compile("(?iu)(?:(ALT\\s*DONATI|ÜST\\s*DONATI|UST\\s*DONATI|İLAVE|ILAVE|ADDITIONAL|MESNET|SUPPORT|GÖVDE|GOVDE|SIDE|DÜŞEY|DUSEY|VERTICAL|YATAY|HORIZONTAL|TOP|BOTTOM|BOT|ALT|ÜST|UST)\\s*[:=]?\\s*)?(\\d{1,3})\\s*(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})(?!\\s*[/@])");
    private static final Pattern STIRRUP_REBAR=Pattern.compile("(?iu)(ETR[İI]YE|STIRRUP)\\s*[:=]?\\s*(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})\\s*[/@]\\s*(\\d{1,3})(?:\\s*(?:MM|CM))?");
    private static final Pattern DISTRIBUTED_REBAR=Pattern.compile("(?iu)(ALT\\s*DONATI|ÜST\\s*DONATI|UST\\s*DONATI|DÜŞEY|DUSEY|VERTICAL|YATAY|HORIZONTAL|TOP|BOTTOM|BOT|ALT|ÜST|UST)\\s*[:=]?\\s*(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})\\s*[/@]\\s*(\\d{1,3})(?:\\s*(?:MM|CM))?");
    private static final String DETAIL_LENGTH="(\\d{1,4}(?:[\\.,]\\d+)?)\\s*(MM|CM|M|Ø|D|DB)";
    private static final Pattern LAP_SPLICE=Pattern.compile("(?iu)\\b(?:B[İI]ND[İI]RME(?:\\s+BOYU)?|LAP(?:\\s+SPLICE)?|EK\\s+BOYU)\\s*[:=]?\\s*"+DETAIL_LENGTH+"\\b");
    private static final Pattern ANCHORAGE=Pattern.compile("(?iu)\\b(?:ANKRAJ(?:\\s+BOYU)?|KENETLENME(?:\\s+BOYU)?|ANCHORAGE(?:\\s+LENGTH)?|DEVELOPMENT\\s+LENGTH|LD)\\s*[:=]?\\s*"+DETAIL_LENGTH+"\\b");
    private static final Pattern COVER=Pattern.compile("(?iu)\\b(?:PAS\\s+PAYI|BETON\\s+ÖRTÜSÜ|BETON\\s+ORTUSU|COVER)\\s*[:=]?\\s*"+DETAIL_LENGTH+"\\b");
    private static final Pattern CONFINEMENT=Pattern.compile("(?iu)\\b(?:SIKLAŞTIRMA(?:\\s+BÖLGESİ|\\s+BOLGESI|\\s+BOYU)?|SARILMA\\s+BÖLGESİ|SARILMA\\s+BOLGESI|CONFINEMENT(?:\\s+ZONE)?)\\s*[:=]?\\s*"+DETAIL_LENGTH+"\\b");
    private static final Pattern CONTINUITY=Pattern.compile("(?iu)\\b(?:(ALT\\s*DONATI|ÜST\\s*DONATI|UST\\s*DONATI|DÜŞEY\\s*DONATI|DUSEY\\s*DONATI|YATAY\\s*DONATI|ALT|ÜST|UST|DÜŞEY|DUSEY|YATAY)\\s+(?:DONATI\\s*)?|DONATI\\s+)(SÜREKLİ|SUREKLI|DEVAMLI|KESİNTİSİZ|KESINTISIZ|CONTINUOUS|KESİLİR|KESILIR|SONLANIR|DISCONTINUOUS)\\b");
    private static final String METRIC_LENGTH="(\\d{1,5}(?:[\\.,]\\d+)?)\\s*(MM|CM|M)";
    private static final Pattern FOUNDATION_THICKNESS=Pattern.compile("(?iu)\\b(?:RADYE|RAFT|TEMEL|FOUNDATION)\\s*(?:KALINLIĞI|KALINLIGI|KALINLIK|THICKNESS)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern PILE_DIAMETER=Pattern.compile("(?iu)\\b(?:KAZIK|PILE)\\s*(?:ÇAPI|CAPI|DIAMETER)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern PILE_LENGTH=Pattern.compile("(?iu)\\b(?:KAZIK|PILE)\\s*(?:BOYU|LENGTH)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern PILE_SPACING=Pattern.compile("(?iu)\\b(?:KAZIK|PILE)\\s*(?:AKS\\s+ARALIĞI|AKS\\s+ARALIGI|ARALIĞI|ARALIGI|SPACING)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern PILE_COUNT=Pattern.compile("(?iu)\\b(?:KAZIK\\s*(?:ADED[İI]|SAYISI)|PILE\\s*COUNT)\\s*[:=]?\\s*(\\d{1,5})\\b");
    private static final Pattern PILE_CAP_THICKNESS=Pattern.compile("(?iu)\\b(?:KAZIK\\s+BAŞLIĞI|KAZIK\\s+BASLIGI|PILE\\s+CAP)\\s*(?:KALINLIĞI|KALINLIGI|KALINLIK|THICKNESS)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern PUNCHING_REBAR=Pattern.compile("(?iu)\\b(?:ZIMBALAMA|PUNCHING)\\s*(?:DONATISI|DONATI|REINFORCEMENT)\\s*[:=]?\\s*(?:Ø|Φ|ø|\\bfi\\s*)\\s*(\\d{1,2})\\s*[/@]\\s*(\\d{1,3})\\b");
    private static final Pattern PUNCHING_PERIMETER=Pattern.compile("(?iu)\\b(?:ZIMBALAMA|PUNCHING)\\s*(?:ÇEVRESİ|CEVRESI|PERIMETER)\\s*[:=]?\\s*"+METRIC_LENGTH+"\\b");
    private static final Pattern OPENING_SIZE=Pattern.compile("(?iu)\\b(BOŞLUK|BOSLUK|REZERVASYON|ŞAFT|SAFT|OPENING|SLEEVE)\\s*(?:NO\\.?\\s*)?([A-ZÇĞİÖŞÜ]{1,3}\\d{1,4})?\\s*[:=]?\\s*(\\d{1,4}(?:[\\.,]\\d+)?)\\s*[x×/]\\s*(\\d{1,4}(?:[\\.,]\\d+)?)\\s*(MM|CM|M)\\b");
    private static final Pattern FLOOR_AFTER=Pattern.compile("(?iu)\\b(?:KAT|FLOOR|STOREY)\\s*[:=]?\\s*([+-]?\\d{1,2}|ZEM[İI]N|GROUND|BODRUM\\s*\\d{0,2}|BASEMENT\\s*\\d{0,2})\\b");
    private static final Pattern FLOOR_BEFORE=Pattern.compile("(?iu)\\b([+-]?\\d{1,2})\\s*\\.?\\s*(?:KAT|FLOOR|STOREY)\\b");
    private static final Pattern FLOOR_GROUND=Pattern.compile("(?iu)\\b(ZEM[İI]N|GROUND)\\s*(?:KAT|FLOOR)?\\b");
    private static final Pattern FLOOR_BASEMENT=Pattern.compile("(?iu)\\b(BODRUM|BASEMENT)\\s*(\\d{0,2})\\s*(?:KAT|FLOOR)?\\b");
    private static final Pattern AXIS=Pattern.compile("(?iu)\\b(?:AKS|AXIS|GRID)\\s*[:=]?\\s*([A-ZÇĞİÖŞÜ0-9]{1,4})(?:\\s*[-/]\\s*([A-ZÇĞİÖŞÜ0-9]{1,4}))?\\b");
    private static final Pattern ELEMENT_TAG=Pattern.compile("(?iu)(?:K|S|P|D|T|B|C|W|L|KIR|KOL|PER|BEAM|COL|WALL|SLAB|FOOT)\\d{1,4}[A-Z]?");

    public enum ElementStatus { MATCH, MISMATCH, REPORT_ONLY, DRAWING_ONLY, UNVERIFIED }
    public enum MemberType { BEAM, COLUMN, WALL, SLAB, FOUNDATION, UNKNOWN }

    public static final class Element {
        public final String tag,floor,axis,section,concreteGrade,rebarGrade;
        public final MemberType memberType;
        public final Set<String> rebarDiameters,longitudinalRebar,stirrups,distributedRebar,memberRebar,lapSplices,anchorage,cover,confinement,continuity,foundationDetails,punchingDetails,openings;
        public final int sourceId;

        Element(String tag,String floor,String axis,String section,String concreteGrade,String rebarGrade,MemberType memberType,
                Collection<String>diameters,Collection<String>longitudinalRebar,Collection<String>stirrups,
                Collection<String>distributedRebar,Collection<String>memberRebar,
                Collection<String>lapSplices,Collection<String>anchorage,Collection<String>cover,
                Collection<String>confinement,Collection<String>continuity,
                Collection<String>foundationDetails,Collection<String>punchingDetails,Collection<String>openings,int sourceId){
            this.tag=canonicalTag(tag);
            this.floor=clean(floor);
            this.axis=clean(axis);
            this.section=clean(section);
            this.concreteGrade=clean(concreteGrade);
            this.rebarGrade=clean(rebarGrade);
            this.memberType=memberType==null?MemberType.UNKNOWN:memberType;
            this.rebarDiameters=immutableSet(diameters);
            this.longitudinalRebar=immutableSet(longitudinalRebar);
            this.stirrups=immutableSet(stirrups);
            this.distributedRebar=immutableSet(distributedRebar);
            this.memberRebar=immutableSet(memberRebar);
            this.lapSplices=immutableSet(lapSplices);
            this.anchorage=immutableSet(anchorage);
            this.cover=immutableSet(cover);
            this.confinement=immutableSet(confinement);
            this.continuity=immutableSet(continuity);
            this.foundationDetails=immutableSet(foundationDetails);
            this.punchingDetails=immutableSet(punchingDetails);
            this.openings=immutableSet(openings);
            this.sourceId=sourceId;
        }

        public String location(){
            StringBuilder out=new StringBuilder();
            if(!floor.isEmpty())out.append(floor);
            if(!axis.isEmpty()){if(out.length()>0)out.append(" • ");out.append("AKS ").append(axis);}
            return out.length()==0?"KAT/AKS ?":out.toString();
        }
    }

    public static final class ElementCheck {
        public final ElementStatus status;
        public final Element report,drawing;
        public final List<String> differences;

        ElementCheck(ElementStatus status,Element report,Element drawing,Collection<String>differences){
            this.status=status;
            this.report=report;
            this.drawing=drawing;
            this.differences=Collections.unmodifiableList(new ArrayList<>(differences));
        }
    }

    public static final class Model {
        public final String name;
        public final int textLength;
        public final Set<String> concreteGrades,rebarGrades,sections,rebarDiameters,foundationTypes;
        public final Map<String,String> taggedSections,designParameters;
        public final List<String> seismicCues,loadCues,warnings;
        public final List<Element> elements;

        Model(String name,int textLength,Collection<String>concreteGrades,Collection<String>rebarGrades,
              Collection<String>sections,Collection<String>rebarDiameters,Collection<String>foundationTypes,
              Map<String,String>taggedSections,Map<String,String>designParameters,
              Collection<String>seismicCues,Collection<String>loadCues,Collection<String>warnings,
              Collection<Element>elements){
            this.name=clean(name);
            this.textLength=Math.max(0,textLength);
            this.concreteGrades=immutableSet(concreteGrades);
            this.rebarGrades=immutableSet(rebarGrades);
            this.sections=immutableSet(sections);
            this.rebarDiameters=immutableSet(rebarDiameters);
            this.foundationTypes=immutableSet(foundationTypes);
            this.taggedSections=Collections.unmodifiableMap(new LinkedHashMap<>(taggedSections));
            this.designParameters=Collections.unmodifiableMap(new LinkedHashMap<>(designParameters));
            this.seismicCues=Collections.unmodifiableList(new ArrayList<>(seismicCues));
            this.loadCues=Collections.unmodifiableList(new ArrayList<>(loadCues));
            this.warnings=Collections.unmodifiableList(new ArrayList<>(warnings));
            this.elements=Collections.unmodifiableList(new ArrayList<>(elements));
        }

        public boolean isEmpty(){
            return concreteGrades.isEmpty()&&rebarGrades.isEmpty()&&sections.isEmpty()&&
                taggedSections.isEmpty()&&elements.isEmpty()&&seismicCues.isEmpty()&&loadCues.isEmpty();
        }
    }

    public static final class Comparison {
        public final boolean matched;
        public final String text;
        public final int sameTaggedSections,differentTaggedSections,reportOnlyTags,drawingOnlyTags;
        public final int sameElements,differentElements,reportOnlyElements,drawingOnlyElements,unverifiedElements;
        public final List<ElementCheck> elementChecks;
        public final List<Integer> sourceIds;

        Comparison(boolean matched,String text,int sameTaggedSections,int differentTaggedSections,
                   int reportOnlyTags,int drawingOnlyTags,
                   int sameElements,int differentElements,int reportOnlyElements,int drawingOnlyElements,int unverifiedElements,
                   Collection<ElementCheck>elementChecks,Collection<Integer>sourceIds){
            this.matched=matched;
            this.text=text==null?"":text;
            this.sameTaggedSections=Math.max(0,sameTaggedSections);
            this.differentTaggedSections=Math.max(0,differentTaggedSections);
            this.reportOnlyTags=Math.max(0,reportOnlyTags);
            this.drawingOnlyTags=Math.max(0,drawingOnlyTags);
            this.sameElements=Math.max(0,sameElements);
            this.differentElements=Math.max(0,differentElements);
            this.reportOnlyElements=Math.max(0,reportOnlyElements);
            this.drawingOnlyElements=Math.max(0,drawingOnlyElements);
            this.unverifiedElements=Math.max(0,unverifiedElements);
            this.elementChecks=Collections.unmodifiableList(new ArrayList<>(elementChecks));
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds));
        }
        public static Comparison none(){
            return new Comparison(false,"",0,0,0,0,0,0,0,0,0,Collections.emptyList(),Collections.emptyList());
        }
    }

    private static final class DrawingSnapshot {
        final LinkedHashSet<String> concrete=new LinkedHashSet<>();
        final LinkedHashSet<String> rebar=new LinkedHashSet<>();
        final LinkedHashSet<String> sections=new LinkedHashSet<>();
        final LinkedHashSet<String> diameters=new LinkedHashSet<>();
        final LinkedHashSet<String> foundations=new LinkedHashSet<>();
        final LinkedHashMap<String,String> tagged=new LinkedHashMap<>();
        final LinkedHashMap<String,Integer> tagSource=new LinkedHashMap<>();
        final ArrayList<Element> elements=new ArrayList<>();
    }

    public static Model parse(String name,String text){
        String raw=text==null?"":text;
        if(raw.length()>MAX_TEXT)raw=raw.substring(0,MAX_TEXT);
        LinkedHashSet<String> concrete=new LinkedHashSet<>(),rebar=new LinkedHashSet<>(),
            sections=new LinkedHashSet<>(),diameters=new LinkedHashSet<>(),foundations=new LinkedHashSet<>();
        LinkedHashMap<String,String>tagged=new LinkedHashMap<>(),designParameters=new LinkedHashMap<>();
        ArrayList<String> seismic=new ArrayList<>(),loads=new ArrayList<>(),warnings=new ArrayList<>();
        ArrayList<Element>elements=parseReportElements(raw);

        collectConcrete(raw,concrete);
        collectRebar(raw,rebar);
        collectSections(raw,sections);
        collectDiameters(raw,diameters);
        collectTagged(raw,tagged);
        collectFoundationTypes(raw,foundations);
        collectDesignParameters(raw,designParameters);
        collectCues(raw,seismic,loads);

        if(concrete.isEmpty())warnings.add("Beton sınıfı metinden güvenilir biçimde çıkarılamadı.");
        if(rebar.isEmpty())warnings.add("Donatı çeliği sınıfı metinden güvenilir biçimde çıkarılamadı.");
        if(sections.isEmpty())warnings.add("Kesit/ebat ifadesi bulunamadı.");
        if(tagged.isEmpty())warnings.add("Eleman etiketi ile kesit birlikte eşleştirilemedi; kesit karşılaştırması kısmi olacaktır.");
        if(elements.isEmpty())warnings.add("Kat/aks/eleman bazında ayrıştırılabilir statik eleman satırı bulunamadı.");
        if(seismic.isEmpty())warnings.add("Deprem/zemin parametrelerine ilişkin okunabilir metin ipucu bulunamadı.");
        if(raw.trim().isEmpty())warnings.add("Hesap raporundan okunabilir metin elde edilemedi.");

        return new Model(name,raw.length(),concrete,rebar,sections,diameters,foundations,tagged,designParameters,
            seismic,loads,warnings,elements);
    }

    public static Comparison compare(MusaAiDrawingIndex index,Model report){
        if(index==null||report==null)return Comparison.none();
        DrawingSnapshot drawing=snapshot(index);
        StringBuilder out=new StringBuilder("STATİK HESAP RAPORU ↔ DWG PROJE KARŞILAŞTIRMASI");
        out.append("\nRapor: ").append(report.name.isEmpty()?"Yüklenen hesap raporu":report.name);
        out.append("\nKarşılaştırma türü: kat + aks + eleman etiketi + kesit + donatı + detaylandırma + temel/zımbalama/rezervasyon ön kontrolü");

        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        List<ElementCheck> elementChecks=compareElements(report.elements,drawing.elements);
        int sameElement=0,diffElement=0,reportOnlyElement=0,drawingOnlyElement=0,unverifiedElement=0;
        for(ElementCheck c:elementChecks){
            switch(c.status){
                case MATCH:sameElement++;break;
                case MISMATCH:
                    diffElement++;
                    if(c.drawing!=null&&c.drawing.sourceId>=0)ids.add(c.drawing.sourceId);
                    break;
                case REPORT_ONLY:reportOnlyElement++;break;
                case DRAWING_ONLY:drawingOnlyElement++;break;
                case UNVERIFIED:unverifiedElement++;break;
            }
        }

        out.append("\n\n1. KAT / AKS / ELEMAN BAZLI KARŞILAŞTIRMA");
        out.append("\n• UYUMLU: ").append(sameElement);
        out.append(" • FARKLI: ").append(diffElement);
        out.append(" • SADECE RAPOR: ").append(reportOnlyElement);
        out.append(" • SADECE DWG: ").append(drawingOnlyElement);
        out.append(" • DOĞRULANAMADI: ").append(unverifiedElement);
        appendElementChecks(out,elementChecks,120);
        if(report.elements.isEmpty()||drawing.elements.isEmpty())
            out.append("\n• Kat/aks/eleman düzeyinde karşılaştırma için taraflardan en az birinde yeterli etiketli veri bulunamadı.");

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

        out.append("\n\n2. GENEL ETİKET / KESİT KONTROLÜ");
        out.append("\n• Aynı etiket ve aynı kesit: ").append(same);
        out.append("\n• Aynı etiket fakat farklı kesit: ").append(different);
        out.append("\n• Hesap raporunda olup DWG'de eşleşmeyen etiket: ").append(reportOnly);
        out.append("\n• DWG'de olup hesap raporunda eşleşmeyen etiket: ").append(drawingOnly);
        for(String line:mismatchLines)out.append("\n• YÜKSEK • ").append(line);
        if(report.taggedSections.isEmpty()||drawing.tagged.isEmpty())
            out.append("\n• Genel etiketli kesit kontrolü için taraflardan birinde yeterli K1/S1/P1 benzeri etiket + ebat verisi bulunamadı.");

        out.append("\n\n3. MALZEME SINIFLARI");
        appendSetComparison(out,"Beton",report.concreteGrades,drawing.concrete,true);
        appendSetComparison(out,"Donatı çeliği",report.rebarGrades,drawing.rebar,true);

        out.append("\n\n4. KESİT / DONATI VERİSİ");
        appendSetComparison(out,"Kesit/ebat",report.sections,drawing.sections,false);
        appendSetComparison(out,"Donatı çapı",report.rebarDiameters,drawing.diameters,false);

        out.append("\n\n5. TEMEL SİSTEMİ");
        appendSetComparison(out,"Temel tipi",report.foundationTypes,drawing.foundations,false);

        out.append("\n\n6. HESAP RAPORUNDAN OKUNAN TASARIM PARAMETRELERİ");
        appendParameters(out,report.designParameters);
        appendSamples(out,"Deprem / zemin",report.seismicCues,10);
        appendSamples(out,"Yük",report.loadCues,8);

        out.append("\n\n7. İNCELEME NOTLARI");
        for(String w:report.warnings)out.append("\n• ").append(w);
        if(diffElement>0||different>0)
            out.append("\n• Açık farklılıklar proje müellifi tarafından hesap modeli ve son revizyon paftası üzerinden doğrulanmalıdır.");
        if(reportOnlyElement+drawingOnlyElement+reportOnly+drawingOnly>0)
            out.append("\n• Tek tarafta görünen eleman doğrudan eksik kabul edilmez; kat, pafta, isimlendirme ve rapor kapsamı kontrol edilmelidir.");
        if(unverifiedElement>0)
            out.append("\n• DOĞRULANAMADI satırları eksik kat/aks/kesit/donatı verisi veya aynı etiketin belirsiz tekrarı nedeniyle otomatik hüküm verilemeyen elemanlardır.");
        out.append("\n• Bindirme, ankraj, pas payı, sıklaştırma ve süreklilik için yalnız pafta/raporda açıkça yazılı değerler kıyaslanır; eksik değer yönetmelikten türetilmez.");
        out.append("\n• Temel/kazık, zımbalama ve boşluk-rezervasyon verileri de yalnız açık ölçü/etiket üzerinden kıyaslanır; zımbalama kapasitesi veya temel taşıma gücü hesaplanmaz.");
        out.append("\n• Bu karşılaştırma statik analiz motoru değildir; iç kuvvet, kapasite, düzensizlik, deplasman, performans veya yönetmelik uygunluğu hesabı yapmaz.");

        return new Comparison(true,out.toString(),same,different,reportOnly,drawingOnly,
            sameElement,diffElement,reportOnlyElement,drawingOnlyElement,unverifiedElement,elementChecks,ids);
    }

    public static String summary(Model model){
        if(model==null)return "Statik hesap raporu yüklenmedi.";
        StringBuilder out=new StringBuilder("STATİK HESAP RAPORU ÖZETİ");
        out.append("\n• Dosya: ").append(model.name.isEmpty()?"—":model.name);
        out.append("\n• Okunan metin: ").append(model.textLength).append(" karakter");
        out.append("\n• Beton sınıfları: ").append(join(model.concreteGrades,8));
        out.append("\n• Donatı sınıfları: ").append(join(model.rebarGrades,8));
        out.append("\n• Kesit/ebat: ").append(model.sections.size()).append(" farklı değer");
        out.append("\n• Donatı çapları: ").append(join(model.rebarDiameters,12));
        out.append("\n• Etiketli kesit: ").append(model.taggedSections.size());
        out.append("\n• Kat/aks/eleman satırı: ").append(model.elements.size());
        out.append("\n• Algılanan kat: ").append(join(elementFloors(model.elements),12));
        out.append("\n• Temel tipi: ").append(join(model.foundationTypes,6));
        if(!model.designParameters.isEmpty())out.append("\n• Tasarım parametreleri: ").append(joinParameters(model.designParameters,10));
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

    private static ArrayList<Element> parseReportElements(String raw){
        ArrayList<Element> out=new ArrayList<>();
        String currentFloor="";
        if(raw==null)return out;
        for(String line:raw.replace('\r','\n').split("\n")){
            String t=line.trim();
            if(t.isEmpty())continue;
            String explicitFloor=detectFloor(t);
            if(!explicitFloor.isEmpty())currentFloor=explicitFloor;
            addElementsFromLine(out,t,currentFloor,-1);
            if(out.size()>=2500)break;
        }
        return out;
    }

    private static void addElementsFromLine(List<Element>out,String raw,String inheritedFloor,int sourceId){
        if(raw==null||raw.isEmpty()||out.size()>=2500)return;
        String floor=detectFloor(raw);
        if(floor.isEmpty())floor=clean(inheritedFloor);
        String axis=detectAxis(raw);
        String axisTag=axis.replace("-","");
        String section=firstSection(raw);
        String concrete=firstConcrete(raw);
        String rebar=firstRebar(raw);
        LinkedHashSet<String>diameters=new LinkedHashSet<>();collectDiameters(raw,diameters);
        LinkedHashSet<String>longitudinal=new LinkedHashSet<>();collectLongitudinalRebar(raw,longitudinal);
        LinkedHashSet<String>stirrups=new LinkedHashSet<>();collectStirrups(raw,stirrups);
        LinkedHashSet<String>distributed=new LinkedHashSet<>();collectDistributedRebar(raw,distributed);
        LinkedHashSet<String>lapSplices=new LinkedHashSet<>();collectLengthDetail(raw,LAP_SPLICE,"BİNDİRME",lapSplices);
        LinkedHashSet<String>anchorage=new LinkedHashSet<>();collectLengthDetail(raw,ANCHORAGE,"ANKRAJ",anchorage);
        LinkedHashSet<String>cover=new LinkedHashSet<>();collectLengthDetail(raw,COVER,"PAS PAYI",cover);
        LinkedHashSet<String>confinement=new LinkedHashSet<>();collectLengthDetail(raw,CONFINEMENT,"SIKLAŞTIRMA",confinement);
        LinkedHashSet<String>continuity=new LinkedHashSet<>();collectContinuity(raw,continuity);
        LinkedHashSet<String>foundationDetails=new LinkedHashSet<>();collectFoundationDetails(raw,foundationDetails);
        LinkedHashSet<String>punchingDetails=new LinkedHashSet<>();collectPunchingDetails(raw,punchingDetails);
        LinkedHashSet<String>openings=new LinkedHashSet<>();collectOpenings(raw,openings);

        Matcher tags=TAG_TOKEN.matcher(raw);
        LinkedHashSet<String> found=new LinkedHashSet<>();
        while(tags.find()&&found.size()<20){
            String tag=canonicalTag(tags.group(1));
            if(!isElementTag(tag))continue;
            if(!axisTag.isEmpty()&&tag.equals(axisTag))continue;
            found.add(tag);
        }
        if(found.isEmpty())return;
        boolean hasElementData=!section.isEmpty()||!diameters.isEmpty()||!longitudinal.isEmpty()||!stirrups.isEmpty()||!distributed.isEmpty()||
            !lapSplices.isEmpty()||!anchorage.isEmpty()||!cover.isEmpty()||!confinement.isEmpty()||!continuity.isEmpty()||
            !foundationDetails.isEmpty()||!punchingDetails.isEmpty()||!openings.isEmpty()||
            !axis.isEmpty()||!floor.isEmpty()||!concrete.isEmpty()||!rebar.isEmpty();
        if(!hasElementData)return;
        for(String tag:found){
            MemberType memberType=detectMemberType(tag,raw);
            LinkedHashSet<String>memberRebar=new LinkedHashSet<>();
            collectMemberSpecificRebar(memberType,longitudinal,stirrups,distributed,memberRebar);
            out.add(new Element(tag,floor,axis,section,concrete,rebar,memberType,diameters,longitudinal,stirrups,distributed,memberRebar,lapSplices,anchorage,cover,confinement,continuity,foundationDetails,punchingDetails,openings,sourceId));
            if(out.size()>=2500)break;
        }
    }

    private static List<ElementCheck> compareElements(List<Element>report,List<Element>drawing){
        ArrayList<ElementCheck> out=new ArrayList<>();
        boolean[] used=new boolean[drawing==null?0:drawing.size()];
        if(report==null)report=Collections.emptyList();
        if(drawing==null)drawing=Collections.emptyList();

        for(Element r:report){
            int match=findElementMatch(r,drawing,used);
            if(match<0){
                out.add(new ElementCheck(ElementStatus.REPORT_ONLY,r,null,
                    Collections.singletonList("Aynı eleman etiketi DWG tarafında güvenilir biçimde eşleştirilemedi.")));
                continue;
            }
            used[match]=true;
            Element d=drawing.get(match);
            ArrayList<String> differences=new ArrayList<>();
            boolean mismatch=false,unverified=false;

            int state=compareField("Kat",r.floor,d.floor,differences);mismatch|=state<0;unverified|=state>0;
            state=compareField("Aks",r.axis,d.axis,differences);mismatch|=state<0;unverified|=state>0;
            state=compareField("Kesit",r.section,d.section,differences);mismatch|=state<0;unverified|=state>0;
            state=compareField("Beton",r.concreteGrade,d.concreteGrade,differences);mismatch|=state<0;unverified|=state>0;
            state=compareField("Donatı çeliği",r.rebarGrade,d.rebarGrade,differences);mismatch|=state<0;unverified|=state>0;
            state=compareMemberType(r.memberType,d.memberType,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Donatı çapı",r.rebarDiameters,d.rebarDiameters,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Boyuna donatı",r.longitudinalRebar,d.longitudinalRebar,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Etriye",r.stirrups,d.stirrups,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Aralıklı donatı",r.distributedRebar,d.distributedRebar,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Tip bazlı donatı",r.memberRebar,d.memberRebar,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Bindirme boyu",r.lapSplices,d.lapSplices,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Ankraj / kenetlenme",r.anchorage,d.anchorage,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Pas payı",r.cover,d.cover,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Sıklaştırma bölgesi",r.confinement,d.confinement,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Donatı sürekliliği",r.continuity,d.continuity,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Temel / radye / kazık detayı",r.foundationDetails,d.foundationDetails,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Zımbalama detayı",r.punchingDetails,d.punchingDetails,differences);mismatch|=state<0;unverified|=state>0;
            state=compareSetField("Boşluk / rezervasyon",r.openings,d.openings,differences);mismatch|=state<0;unverified|=state>0;

            ElementStatus status=mismatch?ElementStatus.MISMATCH:(unverified?ElementStatus.UNVERIFIED:ElementStatus.MATCH);
            out.add(new ElementCheck(status,r,d,differences));
        }

        for(int i=0;i<drawing.size();i++)if(!used[i]){
            Element d=drawing.get(i);
            out.add(new ElementCheck(ElementStatus.DRAWING_ONLY,null,d,
                Collections.singletonList("Eleman DWG'de var; hesap raporu tarafında güvenilir eşleşme bulunamadı.")));
        }
        return out;
    }

    private static int findElementMatch(Element r,List<Element>drawing,boolean[]used){
        ArrayList<Integer> candidates=new ArrayList<>();
        for(int i=0;i<drawing.size();i++){
            if(used[i])continue;
            Element d=drawing.get(i);
            if(r.tag.equals(d.tag))candidates.add(i);
        }
        if(candidates.isEmpty())return -1;
        if(candidates.size()==1)return candidates.get(0);

        ArrayList<Integer> floorMatches=new ArrayList<>();
        if(!r.floor.isEmpty()){
            for(int i:candidates){
                Element d=drawing.get(i);
                if(!d.floor.isEmpty()&&r.floor.equals(d.floor))floorMatches.add(i);
            }
            if(floorMatches.size()==1)return floorMatches.get(0);
            if(!floorMatches.isEmpty())candidates=floorMatches;
        }
        if(!r.axis.isEmpty()){
            ArrayList<Integer> axisMatches=new ArrayList<>();
            for(int i:candidates){
                Element d=drawing.get(i);
                if(!d.axis.isEmpty()&&r.axis.equals(d.axis))axisMatches.add(i);
            }
            if(axisMatches.size()==1)return axisMatches.get(0);
            if(!axisMatches.isEmpty())candidates=axisMatches;
        }
        if(!r.section.isEmpty()){
            ArrayList<Integer> sectionMatches=new ArrayList<>();
            for(int i:candidates){
                Element d=drawing.get(i);
                if(!d.section.isEmpty()&&r.section.equals(d.section))sectionMatches.add(i);
            }
            if(sectionMatches.size()==1)return sectionMatches.get(0);
        }
        return candidates.size()==1?candidates.get(0):-1;
    }

    private static int compareField(String label,String report,String drawing,List<String>out){
        boolean r=!clean(report).isEmpty(),d=!clean(drawing).isEmpty();
        if(!r&&!d)return 0;
        if(r&&d){
            if(clean(report).equalsIgnoreCase(clean(drawing)))return 0;
            out.add(label+": rapor "+report+" • DWG "+drawing);
            return -1;
        }
        out.add(label+": "+(r?"rapor "+report+" • DWG veri yok":"rapor veri yok • DWG "+drawing));
        return 1;
    }

    private static int compareSetField(String label,Set<String>report,Set<String>drawing,List<String>out){
        boolean r=report!=null&&!report.isEmpty(),d=drawing!=null&&!drawing.isEmpty();
        if(!r&&!d)return 0;
        if(r&&d){
            if(report.equals(drawing))return 0;
            out.add(label+": rapor "+join(report,12)+" • DWG "+join(drawing,12));
            return -1;
        }
        out.add(label+": "+(r?"rapor "+join(report,12)+" • DWG veri yok":"rapor veri yok • DWG "+join(drawing,12)));
        return 1;
    }

    private static void appendElementChecks(StringBuilder out,List<ElementCheck>checks,int max){
        if(checks==null||checks.isEmpty())return;
        int shown=0;
        for(ElementCheck c:checks){
            if(shown++>=max){out.append("\n• … kalan eleman satırları rapor görüntüleme sınırı nedeniyle özetlendi.");break;}
            Element e=c.report!=null?c.report:c.drawing;
            out.append("\n• ").append(statusLabel(c.status)).append(" • ")
                .append(e==null?"KAT/AKS ?":e.location()).append(" • ")
                .append(e==null?"?":e.tag);
            if(e!=null&&!e.section.isEmpty())out.append(" • ").append(e.section);
            if(e!=null&&e.memberType!=MemberType.UNKNOWN)out.append(" • ").append(memberTypeLabel(e.memberType));
            if(c.differences.isEmpty()){
                if(c.status==ElementStatus.MATCH)out.append(" • açık veriler uyumlu");
            }else{
                for(String d:c.differences)out.append("\n  - ").append(d);
            }
        }
    }

    private static String statusLabel(ElementStatus status){
        switch(status){
            case MATCH:return "UYUMLU";
            case MISMATCH:return "FARKLI";
            case REPORT_ONLY:return "SADECE RAPOR";
            case DRAWING_ONLY:return "SADECE DWG";
            default:return "DOĞRULANAMADI";
        }
    }

    private static DrawingSnapshot snapshot(MusaAiDrawingIndex index){
        DrawingSnapshot out=new DrawingSnapshot();
        String defaultFloor=detectFloor(index.layout);
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String raw=(item.layer+" "+item.text).trim();
            String norm=MusaAiDrawingIndex.normalize(raw);
            boolean structural=MusaAiDiscipline.classify(raw)==MusaAiDiscipline.STRUCTURAL||
                has(norm,"kolon","kiris","perde","doseme","temel","radye","kazik","donati","betonarme","tasiyici")||
                item.layer.toUpperCase(Locale.ROOT).startsWith("S_");
            if(!structural)continue;
            collectConcrete(raw,out.concrete);
            collectRebar(raw,out.rebar);
            collectSections(raw,out.sections);
            collectDiameters(raw,out.diameters);
            collectFoundationTypes(raw,out.foundations);
            LinkedHashMap<String,String> found=new LinkedHashMap<>();
            collectTagged(raw,found);
            for(Map.Entry<String,String>e:found.entrySet()){
                out.tagged.putIfAbsent(e.getKey(),e.getValue());
                if(item.sourceId>=0)out.tagSource.putIfAbsent(e.getKey(),item.sourceId);
            }
            addElementsFromLine(out.elements,(item.text+" "+item.layer).trim(),defaultFloor,item.sourceId);
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

    private static void appendParameters(StringBuilder out,Map<String,String>values){
        out.append("\n• Açık parametreler:");
        if(values==null||values.isEmpty()){out.append(" güvenilir değer ayrıştırılamadı.");return;}
        for(Map.Entry<String,String>e:values.entrySet())
            out.append("\n  - ").append(e.getKey()).append(" = ").append(e.getValue());
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
    private static String firstConcrete(String raw){
        Matcher m=CONCRETE.matcher(raw==null?"":raw);
        return m.find()?"C"+m.group(1):"";
    }
    private static void collectRebar(String raw,Set<String>out){
        Matcher m=REBAR.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<30)out.add(m.group(1).toUpperCase(Locale.ROOT)+m.group(2)+(m.group(3)==null?"":m.group(3).toUpperCase(Locale.ROOT)));
    }
    private static String firstRebar(String raw){
        Matcher m=REBAR.matcher(raw==null?"":raw);
        return m.find()?m.group(1).toUpperCase(Locale.ROOT)+m.group(2)+(m.group(3)==null?"":m.group(3).toUpperCase(Locale.ROOT)):"";
    }
    private static void collectSections(String raw,Set<String>out){
        String value=raw==null?"":raw;
        Matcher m=SECTION.matcher(value);
        while(m.find()&&out.size()<250){
            if(isNonSectionDimension(value,m.start()))continue;
            out.add(section(m.group(1),m.group(2)));
        }
    }
    private static String firstSection(String raw){
        String value=raw==null?"":raw;
        Matcher m=SECTION.matcher(value);
        while(m.find()){
            if(isNonSectionDimension(value,m.start()))continue;
            return section(m.group(1),m.group(2));
        }
        return "";
    }
    private static boolean isNonSectionDimension(String raw,int start){
        if(raw==null||start<0)return false;
        if(start>0){
            char c=raw.charAt(start-1);
            if(c=='Ø'||c=='Φ'||c=='ø')return true;
        }
        int from=Math.max(0,start-48);
        String prefix=MusaAiDrawingIndex.normalize(raw.substring(from,start));
        return has(prefix,"rezervasyon","bosluk","saft","opening","sleeve","zimbalama donatisi","punching reinforcement");
    }
    private static void collectDiameters(String raw,Set<String>out){
        Matcher m=REBAR_DIAMETER.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<60)out.add("Ø"+Integer.parseInt(m.group(1)));
    }
    private static void collectLongitudinalRebar(String raw,Set<String>out){
        Matcher m=LONGITUDINAL_REBAR.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<40){
            String role=normalizeRebarRole(m.group(1));
            int count=Integer.parseInt(m.group(2)),diameter=Integer.parseInt(m.group(3));
            if(count<1||count>100||diameter<4||diameter>50)continue;
            out.add(role+":"+count+"Ø"+diameter);
        }
    }
    private static void collectStirrups(String raw,Set<String>out){
        Matcher m=STIRRUP_REBAR.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<20){
            int diameter=Integer.parseInt(m.group(2)),spacing=Integer.parseInt(m.group(3));
            if(diameter<4||diameter>25||spacing<2||spacing>1000)continue;
            out.add("ETRİYE:Ø"+diameter+"/"+spacing);
        }
    }
    private static void collectDistributedRebar(String raw,Set<String>out){
        Matcher m=DISTRIBUTED_REBAR.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<30){
            String role=normalizeRebarRole(m.group(1));
            int diameter=Integer.parseInt(m.group(2)),spacing=Integer.parseInt(m.group(3));
            if(diameter<4||diameter>40||spacing<2||spacing>1000)continue;
            out.add(role+":Ø"+diameter+"/"+spacing);
        }
    }
    private static String normalizeRebarRole(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.equals("ALT")||q.equals("ALTDONATI")||q.equals("BOTTOM")||q.equals("BOT"))return "ALT";
        if(q.equals("UST")||q.equals("USTDONATI")||q.equals("TOP"))return "ÜST";
        if(q.equals("ILAVE")||q.equals("ADDITIONAL"))return "İLAVE";
        if(q.equals("MESNET")||q.equals("SUPPORT"))return "MESNET";
        if(q.equals("GOVDE")||q.equals("SIDE"))return "GÖVDE";
        if(q.equals("DUSEY")||q.equals("VERTICAL"))return "DÜŞEY";
        if(q.equals("YATAY")||q.equals("HORIZONTAL"))return "YATAY";
        return "GENEL";
    }
    private static MemberType detectMemberType(String tag,String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(has(q,"kiris","beam"))return MemberType.BEAM;
        if(has(q,"kolon","column"))return MemberType.COLUMN;
        if(has(q,"perde","shear wall","wall"))return MemberType.WALL;
        if(has(q,"doseme","slab"))return MemberType.SLAB;
        if(has(q,"temel","foundation","footing","radye","kazik","pile"))return MemberType.FOUNDATION;
        String t=canonicalTag(tag);
        if(t.matches("(?:K|KIR|B|BEAM)\\d{1,4}[A-Z]?"))return MemberType.BEAM;
        if(t.matches("(?:S|KOL|C|COL)\\d{1,4}[A-Z]?"))return MemberType.COLUMN;
        if(t.matches("(?:P|PER|W|WALL)\\d{1,4}[A-Z]?"))return MemberType.WALL;
        if(t.matches("(?:D|SLAB)\\d{1,4}[A-Z]?"))return MemberType.SLAB;
        if(t.matches("(?:T|FOOT)\\d{1,4}[A-Z]?"))return MemberType.FOUNDATION;
        return MemberType.UNKNOWN;
    }
    private static void collectMemberSpecificRebar(MemberType type,Set<String>longitudinal,Set<String>stirrups,
                                                    Set<String>distributed,Set<String>out){
        if(type==null||type==MemberType.UNKNOWN)return;
        String prefix=memberTypeLabel(type).toUpperCase(new Locale("tr","TR"));
        if(type==MemberType.COLUMN){
            for(String v:longitudinal)out.add(prefix+":BOYUNA:"+stripRole(v));
            for(String v:stirrups)out.add(prefix+":"+v);
        }else if(type==MemberType.BEAM){
            for(String v:longitudinal)out.add(prefix+":"+v);
            for(String v:stirrups)out.add(prefix+":"+v);
        }else if(type==MemberType.WALL){
            for(String v:distributed)if(v.startsWith("DÜŞEY:")||v.startsWith("YATAY:"))out.add(prefix+":"+v);
            for(String v:longitudinal)if(v.startsWith("DÜŞEY:")||v.startsWith("YATAY:"))out.add(prefix+":"+v);
        }else if(type==MemberType.SLAB){
            for(String v:distributed)if(v.startsWith("ALT:")||v.startsWith("ÜST:"))out.add(prefix+":"+v);
            for(String v:longitudinal)if(v.startsWith("ALT:")||v.startsWith("ÜST:"))out.add(prefix+":"+v);
        }else if(type==MemberType.FOUNDATION){
            for(String v:distributed)out.add(prefix+":"+v);
            for(String v:longitudinal)out.add(prefix+":"+v);
        }
    }
    private static String stripRole(String value){
        if(value==null)return "";
        int colon=value.indexOf(':');
        return colon>=0&&colon+1<value.length()?value.substring(colon+1):value;
    }
    private static int compareMemberType(MemberType report,MemberType drawing,List<String>out){
        MemberType r=report==null?MemberType.UNKNOWN:report,d=drawing==null?MemberType.UNKNOWN:drawing;
        if(r==MemberType.UNKNOWN&&d==MemberType.UNKNOWN)return 0;
        if(r==MemberType.UNKNOWN||d==MemberType.UNKNOWN){
            out.add("Eleman tipi: "+(r==MemberType.UNKNOWN?"rapor veri yok":"rapor "+memberTypeLabel(r))+
                " • "+(d==MemberType.UNKNOWN?"DWG veri yok":"DWG "+memberTypeLabel(d)));
            return 1;
        }
        if(r==d)return 0;
        out.add("Eleman tipi: rapor "+memberTypeLabel(r)+" • DWG "+memberTypeLabel(d));
        return -1;
    }
    private static String memberTypeLabel(MemberType type){
        if(type==null)return "BİLİNMİYOR";
        switch(type){
            case BEAM:return "Kiriş";
            case COLUMN:return "Kolon";
            case WALL:return "Perde";
            case SLAB:return "Döşeme";
            case FOUNDATION:return "Temel";
            default:return "Bilinmiyor";
        }
    }
    private static void collectFoundationDetails(String raw,Set<String>out){
        collectMetricPattern(raw,FOUNDATION_THICKNESS,"TEMEL KALINLIĞI",out);
        collectMetricPattern(raw,PILE_DIAMETER,"KAZIK ÇAPI",out);
        collectMetricPattern(raw,PILE_LENGTH,"KAZIK BOYU",out);
        collectMetricPattern(raw,PILE_SPACING,"KAZIK ARALIĞI",out);
        collectMetricPattern(raw,PILE_CAP_THICKNESS,"KAZIK BAŞLIĞI",out);
        Matcher count=PILE_COUNT.matcher(raw==null?"":raw);
        while(count.find()&&out.size()<20){
            try{
                int n=Integer.parseInt(count.group(1));
                if(n>0&&n<=100000)out.add("KAZIK ADEDİ:"+n);
            }catch(Exception ignored){}
        }
    }
    private static void collectPunchingDetails(String raw,Set<String>out){
        Matcher rebar=PUNCHING_REBAR.matcher(raw==null?"":raw);
        while(rebar.find()&&out.size()<20){
            int dia=Integer.parseInt(rebar.group(1)),spacing=Integer.parseInt(rebar.group(2));
            if(dia>=4&&dia<=40&&spacing>=2&&spacing<=1000)out.add("ZIMBALAMA DONATISI:Ø"+dia+"/"+spacing);
        }
        collectMetricPattern(raw,PUNCHING_PERIMETER,"ZIMBALAMA ÇEVRESİ",out);
    }
    private static void collectOpenings(String raw,Set<String>out){
        Matcher m=OPENING_SIZE.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<30){
            double a=metricMm(m.group(3),m.group(5)),b=metricMm(m.group(4),m.group(5));
            if(!(a>0.0)||!(b>0.0)||a>100000.0||b>100000.0)continue;
            String kind=normalizeOpeningKind(m.group(1));
            String id=canonicalOpeningId(m.group(2));
            StringBuilder value=new StringBuilder(kind);
            if(!id.isEmpty())value.append(":").append(id);
            value.append(":").append(formatDetailNumber(a)).append("x").append(formatDetailNumber(b)).append("mm");
            out.add(value.toString());
        }
    }
    private static void collectMetricPattern(String raw,Pattern pattern,String label,Set<String>out){
        Matcher m=pattern.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<20){
            double mm=metricMm(m.group(1),m.group(2));
            if(mm>0.0&&mm<=100000.0)out.add(label+":"+formatDetailNumber(mm)+"mm");
        }
    }
    private static double metricMm(String rawValue,String rawUnit){
        if(rawValue==null||rawUnit==null)return Double.NaN;
        double value;
        try{value=Double.parseDouble(rawValue.replace(',','.'));}catch(Exception e){return Double.NaN;}
        String unit=rawUnit.toUpperCase(Locale.ROOT);
        if(unit.equals("CM"))value*=10.0;
        else if(unit.equals("M"))value*=1000.0;
        else if(!unit.equals("MM"))return Double.NaN;
        return value;
    }
    private static String normalizeOpeningKind(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.equals("REZERVASYON"))return "REZERVASYON";
        if(q.equals("SAFT"))return "ŞAFT";
        if(q.equals("SLEEVE"))return "SLEEVE";
        if(q.equals("OPENING"))return "BOŞLUK";
        return "BOŞLUK";
    }
    private static String canonicalOpeningId(String raw){
        if(raw==null)return "";
        return raw.toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");
    }

    private static void collectLengthDetail(String raw,Pattern pattern,String label,Set<String>out){
        Matcher m=pattern.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<12){
            String value=normalizeDetailLength(m.group(1),m.group(2));
            if(!value.isEmpty())out.add(label+":"+value);
        }
    }
    private static String normalizeDetailLength(String rawValue,String rawUnit){
        if(rawValue==null||rawUnit==null)return "";
        double value;
        try{value=Double.parseDouble(rawValue.replace(',','.'));}catch(Exception e){return "";}
        if(!(value>0.0)||value>100000.0)return "";
        String unit=rawUnit.toUpperCase(Locale.ROOT);
        if(unit.equals("Ø")||unit.equals("D")||unit.equals("DB"))return formatDetailNumber(value)+"Ø";
        if(unit.equals("CM"))value*=10.0;
        else if(unit.equals("M"))value*=1000.0;
        else if(!unit.equals("MM"))return "";
        return formatDetailNumber(value)+"mm";
    }
    private static String formatDetailNumber(double value){
        long rounded=Math.round(value);
        if(Math.abs(value-rounded)<0.0001)return Long.toString(rounded);
        String s=String.format(Locale.ROOT,"%.2f",value);
        while(s.endsWith("0"))s=s.substring(0,s.length()-1);
        if(s.endsWith("."))s=s.substring(0,s.length()-1);
        return s;
    }
    private static void collectContinuity(String raw,Set<String>out){
        Matcher m=CONTINUITY.matcher(raw==null?"":raw);
        while(m.find()&&out.size()<12){
            String role=normalizeRebarRole(m.group(1));
            String state=normalizeContinuityState(m.group(2));
            if(!state.isEmpty())out.add(role+":"+state);
        }
    }
    private static String normalizeContinuityState(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.equals("SUREKLI")||q.equals("DEVAMLI")||q.equals("KESINTISIZ")||q.equals("CONTINUOUS"))return "SÜREKLİ";
        if(q.equals("KESILIR")||q.equals("SONLANIR")||q.equals("DISCONTINUOUS"))return "SONLANIR";
        return "";
    }

    private static void collectTagged(String raw,Map<String,String>out){
        String value=raw==null?"":raw;
        Matcher sec=SECTION.matcher(value);
        while(sec.find()&&out.size()<500){
            if(isNonSectionDimension(value,sec.start()))continue;
            int lineStart=Math.max(value.lastIndexOf('\n',sec.start()),value.lastIndexOf('\r',sec.start()));
            int windowStart=Math.max(lineStart+1,sec.start()-56);
            String prefix=value.substring(windowStart,sec.start());
            Matcher tags=TAG_TOKEN.matcher(prefix);
            String selected="";
            while(tags.find()){
                String candidate=canonicalTag(tags.group(1));
                if(!isElementTag(candidate))continue;
                selected=candidate;
            }
            if(!selected.isEmpty())out.putIfAbsent(selected,section(sec.group(1),sec.group(2)));
        }
    }

    private static String detectFloor(String raw){
        if(raw==null||raw.isEmpty())return "";
        String q=MusaAiDrawingIndex.normalize(raw).trim();
        Matcher m=FLOOR_BASEMENT.matcher(raw);
        if(m.find())return canonicalFloor(m.group(1)+(m.group(2)==null?"":m.group(2)));
        if(!q.contains("zemin sinifi")&&!q.contains("ground type")){
            m=FLOOR_GROUND.matcher(raw);
            if(m.find()&&(q.equals("zemin")||q.equals("ground")||q.contains("zemin kat")||q.contains("ground floor")))
                return "ZEMIN";
        }
        m=FLOOR_AFTER.matcher(raw);
        if(m.find())return canonicalFloor(m.group(1));
        m=FLOOR_BEFORE.matcher(raw);
        if(m.find())return canonicalFloor(m.group(1));
        return "";
    }

    private static String canonicalFloor(String raw){
        String q=MusaAiDrawingIndex.normalize(raw).toUpperCase(Locale.ROOT).replaceAll("\\s+","");
        if(q.contains("GROUND")||q.contains("ZEMIN"))return "ZEMIN";
        if(q.startsWith("BASEMENT"))q="BODRUM"+q.substring("BASEMENT".length());
        if(q.startsWith("BODRUM"))return q;
        if(q.matches("[+-]?\\d{1,2}"))return q+".KAT";
        return q;
    }

    private static String detectAxis(String raw){
        Matcher m=AXIS.matcher(raw==null?"":raw);
        if(!m.find())return "";
        String a=canonicalAxisPart(m.group(1)),b=canonicalAxisPart(m.group(2));
        return b.isEmpty()?a:a+"-"+b;
    }

    private static String canonicalAxisPart(String raw){
        if(raw==null)return "";
        return raw.toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");
    }

    private static boolean isElementTag(String tag){
        if(tag==null||tag.isEmpty())return false;
        if(tag.matches("C\\d{2,3}")||tag.matches("[BS]\\d{3}[A-Z]?"))return false;
        String q=MusaAiDrawingIndex.normalize(tag).toUpperCase(Locale.ROOT);
        return ELEMENT_TAG.matcher(q).matches();
    }

    private static Set<String> elementFloors(Collection<Element>elements){
        LinkedHashSet<String> out=new LinkedHashSet<>();
        if(elements!=null)for(Element e:elements)if(e!=null&&!e.floor.isEmpty())out.add(e.floor);
        return out;
    }

    private static void collectFoundationTypes(String raw,Set<String>out){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(has(q,"radye","raft"))out.add("RADYE");
        if(has(q,"fore kazik","kazik temel","pile foundation","bored pile"))out.add("KAZIK");
        if(has(q,"tekil temel","isolated footing","münferit temel","munferit temel"))out.add("TEKİL");
        if(has(q,"surekli temel","mütemadi temel","mutemadi temel","strip footing"))out.add("SÜREKLİ");
        if(has(q,"birlesik temel","combined footing"))out.add("BİRLEŞİK");
    }

    private static void collectDesignParameters(String raw,Map<String,String>out){
        if(raw==null||raw.isEmpty())return;
        collectNamedNumber(raw,out,"SDS","(?iu)\\bSDS\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"SD1","(?iu)\\bSD1\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"DTS","(?iu)\\bDTS\\s*[:=]?\\s*([1-4])\\b");
        collectNamedNumber(raw,out,"BYS","(?iu)\\bBYS\\s*[:=]?\\s*([1-8])\\b");
        collectNamedText(raw,out,"Zemin Sınıfı","(?iu)\\b(?:ZEM[İI]N\\s*SINIFI|GROUND\\s*TYPE)\\s*[:=]?\\s*(Z[A-F])\\b");
        collectNamedNumber(raw,out,"R","(?iu)(?:TAŞIYICI\\s*SİSTEM\\s*DAVRANIŞ\\s*KATSAYISI|TASIYICI\\s*SISTEM\\s*DAVRANIS\\s*KATSAYISI|R\\s*KATSAYISI)\\s*(?:\\(R\\))?\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"D","(?iu)(?:DAYANIM\\s*FAZLALIĞI\\s*KATSAYISI|DAYANIM\\s*FAZLALIGI\\s*KATSAYISI|D\\s*KATSAYISI)\\s*(?:\\(D\\))?\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"I","(?iu)(?:BİNA\\s*ÖNEM\\s*KATSAYISI|BINA\\s*ONEM\\s*KATSAYISI|IMPORTANCE\\s*FACTOR)\\s*(?:\\(I\\))?\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"T1X","(?iu)\\bT1\\s*[-_ ]?X\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedNumber(raw,out,"T1Y","(?iu)\\bT1\\s*[-_ ]?Y\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?)");
        collectNamedText(raw,out,"Zemin Taşıma Gücü","(?iu)\\b(?:ZEM[İI]N\\s+(?:EMN[İI]YET\\s+GER[İI]LMES[İI]|TAŞIMA\\s+GÜCÜ|TASIMA\\s+GUCU)|ALLOWABLE\\s+BEARING\\s+(?:CAPACITY|PRESSURE)|BEARING\\s+CAPACITY)\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?\\s*(?:KPA|KN\\s*/\\s*M(?:2|²)|T\\s*/\\s*M(?:2|²)|KG\\s*/\\s*CM(?:2|²)))");
        collectNamedText(raw,out,"Yatak Katsayısı","(?iu)\\b(?:YATAK\\s+KATSAYISI|ZEM[İI]N\\s+YATAK\\s+KATSAYISI|SUBGRADE\\s+MODULUS|MODULUS\\s+OF\\s+SUBGRADE\\s+REACTION|K[Ss])\\s*[:=]?\\s*([0-9]+(?:[\\.,][0-9]+)?\\s*(?:KN\\s*/\\s*M(?:3|³)|MN\\s*/\\s*M(?:3|³)|T\\s*/\\s*M(?:3|³)))");
        collectNamedText(raw,out,"Yeraltı Suyu","(?iu)\\b(?:YERALTI\\s+SUYU(?:\\s+SEV[İI]YES[İI])?|YER\\s+ALTI\\s+SUYU(?:\\s+SEV[İI]YES[İI])?|GROUNDWATER(?:\\s+LEVEL)?)\\s*[:=]?\\s*([+-]?[0-9]+(?:[\\.,][0-9]+)?\\s*M)");
        collectNamedText(raw,out,"Temel Alt Kotu","(?iu)\\b(?:TEMEL\\s+ALT\\s+KOTU|FOUNDATION\\s+(?:BOTTOM|BASE)\\s+LEVEL)\\s*[:=]?\\s*([+-]?[0-9]+(?:[\\.,][0-9]+)?\\s*M)");
    }

    private static void collectNamedNumber(String raw,Map<String,String>out,String key,String regex){
        Matcher m=Pattern.compile(regex).matcher(raw);
        if(m.find())out.putIfAbsent(key,m.group(1).replace(',','.'));
    }

    private static void collectNamedText(String raw,Map<String,String>out,String key,String regex){
        Matcher m=Pattern.compile(regex).matcher(raw);
        if(m.find())out.putIfAbsent(key,m.group(1).toUpperCase(new Locale("tr","TR")));
    }

    private static void collectCues(String raw,List<String>seismic,List<String>loads){
        if(raw==null)return;
        for(String line:raw.replace('\r','\n').split("\n")){
            String t=line.trim().replaceAll("\\s+"," ");
            if(t.isEmpty())continue;if(t.length()>180)t=t.substring(0,180)+"…";
            String q=MusaAiDrawingIndex.normalize(t);
            if(seismic.size()<220&&has(q,
                "sds","sd1","dts","bys","zemin sinifi","deprem","spektrum","bina onem","tasarim spektrumu","ra r","dayanim fazlaligi",
                "guclu kolon","zayif kiris","strong column","weak beam","kolon kiris birlesim","beam column joint",
                "sarilma bolgesi","confinement zone","ozel deprem etriyesi","special seismic hoop",
                "duzensizlik","irregularity","a1","a2","a3","b1","b2","b3",
                "kat otelemesi","goreli kat otelemesi","story drift","interstory drift",
                "burulma","torsional irregularity","eta bi","etabi",
                "yumusak kat","soft story","zayif kat","weak story",
                "modal","mod sekli","mode shape","periyot","period","kutle katilim","mass participation",
                "etkin modal kutle","effective modal mass","mod birlestirme","response spectrum","modal combination",
                "kisa kolon","short column","perde bag kirisi","coupling beam","rijit diyafram","rigid diaphragm","semi rigid diaphragm",
                "doseme sureksiz","slab discontinuity","bodrum perdesi","basement wall","cevre perdesi",
                "zemin tasima gucu","zemin emniyet gerilmesi","allowable bearing","bearing capacity",
                "yatak katsayisi","subgrade modulus","groundwater","yeralti suyu","yer alti suyu","temel alt kotu",
                "zemin basinci","temel basinci","soil pressure","contact pressure","oturma","settlement",
                "kazik kapasitesi","kazik tasima","pile capacity","pile load","kazik yuk",
                "uplift","yuzme","hidrostatik","hydrostatic","buoyancy",
                "p-delta","p delta","ikinci mertebe","second order","second-order",
                "taban kesme","base shear","spektrum olcekle","spectrum scale","scaling",
                "zemin yapi etkilesimi","soil structure interaction","soil-structure interaction","yay katsayisi","spring stiffness","area spring",
                "kutle kaynagi","mass source","deprem kutlesi","seismic weight","seismic mass",
                "kutle merkezi","rijitlik merkezi","center of mass","centre of mass","center of rigidity","centre of rigidity","eksantrisite","eccentricity",
                "tesadufi eksantrisite","accidental eccentricity","additional eccentricity",
                "collector","drag strut","diyafram kiri","diaphragm chord","chord force",
                "dusey deprem","dikey deprem","vertical earthquake","vertical seismic","vertical response spectrum",
                "yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon","combination",
                "deprem yuk durum","deprem yük durum","seismic load case","earthquake load case","ex","ey","rsx","rsy",
                "r d i","r/d/i","tasiyici sistem katsay","taşıyıcı sistem katsay","behavior factor","overstrength","importance factor",
                "etkin rijitlik","etkin kesit rijitligi","çatlamış kesit","catlamis kesit","cracked section","effective stiffness","stiffness modifier",
                "rijitlik carpani","rijitlik çarpanı","property modifier","section modifier",
                "mafsal","hinge","release","end release","moment release","rijit bolge","rijit bölge","rigid zone","end offset","joint offset",
                "pmm","p-m-m","interaction ratio","etkilesim orani","etkileşim oranı","kapasite orani","kapasite oranı","capacity ratio",
                "utilization","utilisation","kullanim orani","kullanım oranı","demand capacity","d/c ratio","dc ratio",
                "kiris kesme","kiriş kesme","beam shear","kiris moment","kiriş moment","beam moment","flexural ratio","moment ratio",
                "perde kesme","wall shear","shear wall shear","kesme kapasitesi","shear capacity",
                "singular","singularity","tekil rijitlik","instability","unstable","kararsiz","kararsız","mechanism","mekanizma","zero stiffness","negative stiffness",
                "unconnected","disconnected","orphan node","orphan joint","baglantisiz dugum","bağlantısız düğüm","baglantisiz eleman","bağlantısız eleman","floating node","floating joint","joint not connected","element not connected",
                "mesh quality","mesh warning","finite element mesh","shell mesh","aspect ratio","distorted element","mesh size","sonlu eleman ag","sonlu eleman ağ","kabuk mesh","mesh kalitesi",
                "nonconvergence","non-convergence","did not converge","not converged","convergence failed","converged","convergence achieved","yakinsamadi","yakınsamadı","yakinsama","yakınsama","iteration limit","iterasyon limiti",
                "local axis","local axes","yerel eksen","orientation assignment","section orientation","major axis","minor axis","eleman yonu","eleman yönü",
                "revizyon","revision","rev no","revizyon no","model revision","model rev",
                "material assignment","material property","malzeme atama","malzeme ataması","undefined material","material not assigned","malzeme atanmamis","malzeme atanmamış","default material",
                "section assignment","section property","kesit atama","kesit ataması","undefined section","section not assigned","property not assigned","default section",
                "story assignment","story data","kat atama","kat bilgisi","unknown story","undefined story","floor assignment",
                "duplicate element","duplicate joint","duplicate member","mukerrer eleman","mükerrer eleman","conflicting assignment","çelişkili atama","celiskili atama",
                "zero length","zero-length","very short element","very short member","coincident joint","coincident node","sifir uzunluk","sıfır uzunluk","degenerate element",
                "support restraint","joint restraint","boundary condition","mesnet atama","mesnet tanimi","mesnet tanımı","support not assigned","missing restraint","unrestrained joint",
                "diaphragm assignment","diaphragm constraint","constraint assignment","diyafram atama","diyafram tanimi","diyafram tanımı","diaphragm not assigned",
                "load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama",
                "self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier",
                "unit system","model units","birim sistemi","birim ayari","birim ayarı","unit mismatch","birim uyumsuz",
                "design code","code version","tasarim yonetmeligi","tasarım yönetmeliği","yonetmelik surumu","yönetmelik sürümü","tbdy","ts500",
                "story elevation","floor elevation","kat kotu","kat kotlari","kat kotları","coordinate system","koordinat sistemi","elevation mismatch",
                "analysis case","run status","analysis status","not run","case failed","analysis outdated","analiz durumu","calistirilmamis","çalıştırılmamış",
                "duplicate load case","duplicate combination","undefined load case","missing load case","combination reference","load case reference","yuk durumu referansi","yük durumu referansı",
                "yetersiz","uygunsuz","failed","fail","does not comply","not satisfied"))
                addUnique(seismic,t);
            if(loads.size()<80&&has(q,
                "hareketli yuk","sabit yuk","kar yuku","ruzgar yuku","duvar yuku","live load","dead load","snow load","wind load",
                "sehim","deflection","servisabilite","serviceability",
                "catlak genisligi","çatlak genişliği","crack width",
                "titresim","titreşim","vibration","comfort frequency","floor frequency",
                "sunme","sünme","creep","rotre","rötre","shrinkage","uzun sureli sehim","uzun süreli sehim","long term deflection",
                "load assignment","load not assigned","unassigned load","missing load","area load","frame load","shell load","yuk atama","yük atama",
                "self weight multiplier","self-weight multiplier","self weight","self-weight","oz agirlik","öz ağırlık","gravity load","gravity case","dead load multiplier",
                "yuk kombinasyonu","yük kombinasyonu","load combination","load combo","kombinasyon","combination",
                "deprem yuk durum","deprem yük durum","seismic load case","earthquake load case"))
                addUnique(loads,t);
        }
    }

    private static void addUnique(List<String>out,String value){if(!out.contains(value))out.add(value);}
    private static String section(String a,String b){return Integer.parseInt(a)+"x"+Integer.parseInt(b);}
    private static String canonicalTag(String raw){return raw==null?"":raw.toUpperCase(new Locale("tr","TR")).replaceAll("[^A-ZÇĞİÖŞÜ0-9]","");}
    private static String joinParameters(Map<String,String>values,int max){
        if(values==null||values.isEmpty())return "—";StringBuilder out=new StringBuilder();int n=0;
        for(Map.Entry<String,String>e:values.entrySet()){
            if(n++>=max){out.append(", …");break;}
            if(out.length()>0)out.append(", ");
            out.append(e.getKey()).append("=").append(e.getValue());
        }
        return out.toString();
    }

    private static String join(Collection<String>values,int max){
        if(values==null||values.isEmpty())return "—";StringBuilder out=new StringBuilder();int n=0;
        for(String v:values){if(n++>=max){out.append(", …");break;}if(out.length()>0)out.append(", ");out.append(v);}
        return out.toString();
    }
    private static boolean has(String q,String...terms){for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;}
    private static Set<String>immutableSet(Collection<String>source){
        if(source==null||source.isEmpty())return Collections.emptySet();
        return Collections.unmodifiableSet(new LinkedHashSet<>(source));
    }
    private static String extension(String name){if(name==null)return "";int dot=name.lastIndexOf('.');return dot>=0&&dot+1<name.length()?name.substring(dot+1).toLowerCase(Locale.ROOT):"";}
    private static String clean(String value){return value==null?"":value.trim();}
    private MusaAiStructuralCalc(){}
}
