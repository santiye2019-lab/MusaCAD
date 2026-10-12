package com.musa.cad;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Natural Turkish request -> discipline and subdiscipline audit plan.
 * Requests do not need magic commands or exact phrasing.
 * An audit section is a requested CHECK, never proof that the system exists.
 */
public final class MusaAiDisciplinePlan {
    private static final String[] FULL={
        "architectural","structural","cleanwater","wastewater","heating",
        "ventilation","cooling","fire","automation","lowcurrent","highcurrent",
        "gas","landscape","infrastructure","elevator"
    };
    private static final String[] MECHANICAL={
        "cleanwater","wastewater","heating","ventilation","cooling","fire",
        "automation","gas"
    };
    private static final String[] ELECTRICAL={"lowcurrent","highcurrent"};
    private static final String[] SANITARY={"cleanwater","wastewater"};

    public static final class Plan {
        public final String scope;
        public final String label;
        public final List<String> sections;
        public final boolean fullProject;
        private Plan(String scope,String label,LinkedHashSet<String> requested,
                     boolean fullProject){
            this.scope=scope;
            this.label=label;
            this.sections=Collections.unmodifiableList(new ArrayList<>(requested));
            this.fullProject=fullProject;
        }
        public boolean single(){return sections.size()==1;}
        public boolean includes(String key){return sections.contains(key);}
        public boolean needsMechanicalEvidence(){
            for(String key:sections)
                if(isMechanical(key))return true;
            return false;
        }
        public List<MusaAiDiscipline> localParents(){
            LinkedHashSet<MusaAiDiscipline> types=new LinkedHashSet<>();
            for(String key:sections)types.add(parent(key));
            return Collections.unmodifiableList(new ArrayList<>(types));
        }
        public String register(){
            if(sections.isEmpty())return "Yerel CAD etiketlerinde güvenilir alt disiplin sınıflandırması bulunamadı; görsel AI tanısı bekleniyor.";
            StringBuilder out=new StringBuilder("İNCELENECEK DİSİPLİNLER • ÇİZİM KANITI / İSTEK");
            for(String key:sections)
                out.append("\n• ").append(sectionLabel(key)).append(" — ")
                   .append(MusaAiVisualDiscipline.focus(key));
            out.append("\nBu liste hedeflenen denetim kapsamıdır; çizimde görüldüğü veya ")
               .append("tamamının doğrulandığı anlamına gelmez.");
            return out.toString();
        }
        public String visionInstruction(){
            StringBuilder out=new StringBuilder("DOĞAL DİLLE SEÇİLEN İNCELEME PLANI: ")
                .append(label).append(".\n");
            for(String key:sections)
                out.append("• ").append(sectionLabel(key)).append(": ")
                   .append(MusaAiVisualDiscipline.focus(key)).append(".\n");
            out.append("Önce görselde baskın gerçek sistemleri belirle; çizimde görülmeyen disiplinler için uzun kontrol listesi üretme. Her kanıtlanan alt disiplin için AYRI başlık aç. Her başlık altında yalnız ")
               .append("görüntüde gerçekten okunan cihaz/hat/ölçü/mahal ve konumunu, ")
               .append("şüpheli eksik/çelişkiyi, koordinasyon ihtiyacını ve belirsizliği bildir. ")
               .append("Görülemeyen sistemi 'görsel kanıt yok / ayrıca incelenmeli' olarak belirt; ")
               .append("yokluğu, uygunluğu veya mevzuat ihlalini uydurma. ")
               .append("Kat, kesit, vaziyet, şaft, kot ve diğer disiplinlerle temasları karşılaştır. ")
               .append("Görsel gözlem CAD nesne kimliği, hesap raporu veya saha doğrulaması değildir. ")
               .append("Kullanıcı doğal dilde birden fazla sistemi istediyse hiçbirini atlama.");
            return out.toString();
        }
        public Evidence localEvidence(MusaAiDrawingIndex index){
            StringBuilder text=new StringBuilder("ALT DİSİPLİN CAD KANIT ENVANTERİ");
            LinkedHashSet<Integer> ids=new LinkedHashSet<>();
            int total=index==null?0:index.items().size();
            for(String key:sections){
                int count=0,printed=0;
                StringBuilder examples=new StringBuilder();
                if(index!=null)for(MusaAiDrawingIndex.Item item:index.items()){
                    if(item==null)continue;
                    if(!matches(key,item))continue;
                    count++;
                    if(item.sourceId>=0)ids.add(item.sourceId);
                    if(printed<3){
                        examples.append("\n  - ")
                            .append(item.type.isEmpty()?"CAD":item.type)
                            .append(" • katman ").append(item.layer.isEmpty()?"?":item.layer);
                        if(!item.text.isEmpty()){
                            String t=item.text.replace('\n',' ').replace('\r',' ');
                            examples.append(" • ").append(t.substring(0,Math.min(t.length(),85)));
                        }
                        if(item.sourceId>=0)examples.append(" [kaynak ").append(item.sourceId).append("]");
                        printed++;
                    }
                }
                text.append("\n• ").append(sectionLabel(key)).append(": ")
                    .append(count).append(" ilgili yazı/katman/nesne adayı (")
                    .append(total).append(" yerel indeks öğesi içinde).");
                if(count==0)text.append(" Açık etiket saptanmadı; sistemin mevcut olmadığı sonucu çıkarılamaz.");
                else text.append(examples);
            }
            text.append("\nNot: Aynı CAD öğesi birden fazla sistemi işaret edebilir. ")
                .append("Etiket sayısı tesisat adedi, metraj, uygunluk veya tam pafta denetimi değildir.");
            return new Evidence(text.toString(),new ArrayList<>(ids));
        }
    }

    public static final class Evidence {
        public final String text;
        public final List<Integer> sourceIds;
        private Evidence(String text,List<Integer> sourceIds){
            this.text=text;
            this.sourceIds=Collections.unmodifiableList(sourceIds);
        }
    }

    /** "All disciplines" is not synonymous with "all open DWG files". */
    public static boolean wantsMultiOpenFiles(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return contains(q,"proje paketi","tum acik proje","acik tum proje",
            "tum acik cizim","acik tum cizim","acik dosyalarin hepsi",
            "birden fazla acik proje");
    }

    public static Plan resolve(String raw,String fallbackScope){
        String q=MusaAiDrawingIndex.normalize(raw);
        LinkedHashSet<String> chosen=new LinkedHashSet<>();
        boolean full=contains(q,"tum disiplin","butun disiplin",
            "her disiplin","tum bran","butun bran","tum sistemleri",
            "tum projeyi","projenin tamamini","her seyi kontrol",
            "tumunu incele","hepsini incele","butununu incele");
        if(full){
            chosen.addAll(Arrays.asList(FULL));
            return new Plan("all","Tüm disiplinler ve alt sistemler",chosen,true);
        }

        // Specific components are recognized before their parent discipline.
        if(contains(q,"temiz su","kullanma suyu","soguk su tesisat","sicak su tesisat"))chosen.add("cleanwater");
        if(contains(q,"pis su","atik su","kanalizasyon","havalik","wastewater"))chosen.add("wastewater");
        if(contains(q,"kalorifer","isitma","radyator","yerden isitma","heat pump"))chosen.add("heating");
        if(contains(q,"havalandirma","hava kanali","ventilasyon","egzoz havasi"))chosen.add("ventilation");
        if(contains(q,"klima","sogutma","vrf","chiller","fancoil"))chosen.add("cooling");
        if(contains(q,"sprinkler","hidrant","yangin tesisati","yangin sondurme","yangin pompasi"))chosen.add("fire");
        if(contains(q,"otomasyon","bina yonetim sistemi","bms","ddc","scada",
                      "akilli bina","kontrol otomasyonu"))chosen.add("automation");
        if(contains(q,"zayif akim","zayif ve kuvvetli akim","elektrik zayif",
                      "zayif akim ve kuvvetli","data tesisati","veri kablolama","yangin algilama",
                      "kamera sistemi","cctv","network tesisati","kartli gecis","seslendirme"))
            chosen.add("lowcurrent");
        if(contains(q,"kuvvetli akim","guc dagitim","enerji dagitim","elektrik panosu",
                      "elektrik besleme","kompanzasyon","topraklama"))
            chosen.add("highcurrent");
        if(contains(q,"dogalgaz","gaz tesisati"))chosen.add("gas");

        boolean arch=contains(q,"mimari","mimarlik","mahal plani");
        boolean stat=contains(q,"statik","betonarme","tasiyici sistem","kalip plani");
        boolean mech=contains(q,"mekanik tesisat","mekanik proje","mekanik sistemi",
             "tum mekanik","butun mekanik","mekanigi","mekanikleri");
        boolean elec=contains(q,"elektrik proje","elektrik tesisat","elektrigi","tum elektrik",
             "elektrik sistem","elektrik disiplin","elektrik kontrol");
        if(arch)chosen.add("architectural");
        if(stat)chosen.add("structural");
        if(mech)chosen.addAll(Arrays.asList(MECHANICAL));
        if(elec)chosen.addAll(Arrays.asList(ELECTRICAL));
        if(contains(q,"sihhi tesisat","sihhi sistem"))chosen.addAll(Arrays.asList(SANITARY));
        // Broad category names also work alone ("mekaniği incele", "elektriğe bak").
        if(chosen.isEmpty()&&contains(q,"mekanik","tesisat sistemleri"))
            chosen.addAll(Arrays.asList(MECHANICAL));
        if(chosen.isEmpty()&&contains(q,"elektrik","elektrige","elektrigi"))
            chosen.addAll(Arrays.asList(ELECTRICAL));
        if(chosen.isEmpty()&&contains(q,"yangin")&&!contains(q,"algilama"))chosen.add("fire");
        if(chosen.isEmpty()&&contains(q,"peyzaj"))chosen.add("landscape");
        if(chosen.isEmpty()&&contains(q,"altyapi"))chosen.add("infrastructure");
        if(chosen.isEmpty()&&contains(q,"asansor"))chosen.add("elevator");

        if(chosen.isEmpty()){
            String fallback=fallbackScope==null?"all":fallbackScope;
            switch(fallback){
                case "all":chosen.addAll(Arrays.asList(FULL));break;
                case "mechanical":chosen.addAll(Arrays.asList(MECHANICAL));break;
                case "electrical":chosen.addAll(Arrays.asList(ELECTRICAL));break;
                case "sanitary":chosen.addAll(Arrays.asList(SANITARY));break;
                case "structural":chosen.add("structural");break;
                case "architectural":chosen.add("architectural");break;
                default:chosen.add(fallback);break;
            }
        }
        // If a broad system AND an explicit subsection were requested, cover both,
        // but never expand the entire project from the ambiguous word "tesisat".
        String reportScope;
        String label;
        if(chosen.equals(new LinkedHashSet<>(Arrays.asList(MECHANICAL)))){
            reportScope="mechanical";label="Mekanik tesisat ve alt sistemleri";
        }else if(chosen.equals(new LinkedHashSet<>(Arrays.asList(ELECTRICAL)))){
            reportScope="electrical";label="Elektrik • Zayıf ve Kuvvetli Akım";
        }else if(chosen.size()==1){
            reportScope=chosen.iterator().next();label=sectionLabel(reportScope);
        }else{
            reportScope="all";label="Çok disiplinli / çok sistemli denetim";
        }
        return new Plan(reportScope,label,chosen,false);
    }

    /**
     * For an open-ended request, select the CAD-evidenced predominant systems
     * instead of printing every possible mechanical/electrical subsystem.
     * This is only a *focus candidate* based on labels/layers; Qwen must
     * independently identify what is actually visible. Explicit user scope wins.
     */
    public static Plan focusOnDrawing(String raw,Plan plan,MusaAiDrawingIndex index){
        if(plan==null||plan.fullProject||plan.sections.size()<=3||index==null)return plan;
        String q=MusaAiDrawingIndex.normalize(raw);
        if(contains(q,"tum alt sistem","butun alt sistem","butun mekanik",
                "tum mekanik","tum elektrik","her sistemi","tum sistemleri",
                "tek tek hepsini","temiz su","pis su","atik su","kalorifer",
                "havalandirma","sogutma","klima","sprinkler","yangin sondurme",
                "otomasyon","dogalgaz","kuvvetli akim","zayif akim",
                "mimari ve statik","mimariyle statik"))return plan;
        LinkedHashMap<String,Integer> counts=new LinkedHashMap<>();
        for(String key:plan.sections)counts.put(key,0);
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            for(String key:counts.keySet()){
                if(matchesSection(key,item)){
                    int weight=item.text.isEmpty()?1:3;
                    counts.put(key,Math.min(100000,counts.get(key)+weight));
                }
            }
        }
        int maximum=0;
        for(Integer score:counts.values())maximum=Math.max(maximum,score);
        if(maximum<3){
            LinkedHashSet<String> candidates=new LinkedHashSet<>();
            String parent=plan.scope.equals("mechanical")?"mechanical":
                plan.scope.equals("electrical")?"electrical":"";
            if(!parent.isEmpty())candidates.add(parent);
            return new Plan(parent.isEmpty()?"all":parent,
                "Paftadaki alt disiplinler • görsel tanı bekleniyor",candidates,false);
        }
        final int minimum=Math.max(3,maximum/5);
        ArrayList<String> ordered=new ArrayList<>(counts.keySet());
        ordered.sort((a,b)->Integer.compare(counts.get(b),counts.get(a)));
        LinkedHashSet<String> selected=new LinkedHashSet<>();
        for(String key:ordered){
            if(counts.get(key)<minimum||selected.size()==3)break;
            selected.add(key);
        }
        String label=selected.size()==1?sectionLabel(selected.iterator().next()):
            "Paftada öne çıkan "+selected.size()+" alt sistem";
        String scope=selected.size()==1?selected.iterator().next():plan.scope;
        return new Plan(scope,label,selected,false);
    }

    public static String sectionLabel(String scope){
        switch(scope){
            case "cleanwater":return "Mekanik / Temiz su";
            case "wastewater":return "Mekanik / Pis su ve atık su";
            case "heating":return "Mekanik / Kalorifer ve ısıtma";
            case "ventilation":return "Mekanik / Havalandırma";
            case "cooling":return "Mekanik / Klima ve soğutma";
            case "fire":return "Mekanik / Yangın söndürme";
            case "automation":return "Mekanik–Elektrik / Bina otomasyonu";
            case "gas":return "Mekanik / Doğalgaz";
            case "lowcurrent":return "Elektrik / Zayıf akım";
            case "highcurrent":return "Elektrik / Kuvvetli akım";
            default:return MusaAiAnalysisIntent.label(scope);
        }
    }

    private static MusaAiDiscipline parent(String scope){
        switch(scope){
            case "architectural":return MusaAiDiscipline.ARCHITECTURAL;
            case "structural":return MusaAiDiscipline.STRUCTURAL;
            case "lowcurrent":case "highcurrent":case "automation":
                return MusaAiDiscipline.ELECTRICAL;
            case "fire":return MusaAiDiscipline.FIRE_SAFETY;
            case "landscape":return MusaAiDiscipline.LANDSCAPE;
            case "infrastructure":return MusaAiDiscipline.INFRASTRUCTURE;
            case "elevator":return MusaAiDiscipline.ELEVATOR;
            default:return MusaAiDiscipline.MECHANICAL;
        }
    }
    private static boolean isMechanical(String scope){
        for(String key:MECHANICAL)if(key.equals(scope))return true;
        return false;
    }
    public static boolean matchesSection(String key,MusaAiDrawingIndex.Item item){
        return item!=null && matches(key,item);
    }
    private static boolean matches(String key,MusaAiDrawingIndex.Item item){
        // Hyphen/underscore separators in CAD layer names are word boundaries.
        String normalized=MusaAiDrawingIndex.normalize(
            item.layer.replace('-',' ').replace('_',' ')+" "+item.text)+" "+
            MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
        switch(key){
            case "cleanwater":return contains(normalized,"temiz su","sicak su","soguk su","hidrofor","boyler","kullanma suyu","mek-ts","m-ts");
            case "wastewater":return contains(normalized,"pis su","atik su","havalik","kanalizasyon","mek-ps","m-ps");
            case "heating":return contains(normalized,"kalorifer","isitma","radyator","kolektor","kazan","m-isi","mek-isi");
            case "ventilation":return contains(normalized,"havalandirma","kanal","menfez","fan","egzoz","m-hav","mek-hav");
            case "cooling":return contains(normalized,"klima","vrf","sogutma","chiller","fancoil","drenaj");
            case "fire":return contains(normalized,"sprinkler","hidrant","yangin","yng-","yangin pomp");
            case "automation":return contains(normalized,"otomasyon","bms","ddc","scada","sensor","aktuator","kontrol sistemi");
            case "lowcurrent":return contains(normalized,"zayif akim","data","kamera","cctv","network","yangin algilama","kartli gecis","e-za");
            case "highcurrent":return contains(normalized,"kuvvetli akim","guc","pano","kompanzasyon","besleme","topraklama","e-ka");
            case "architectural":return contains(normalized,"mimari","mim-","mahal","kapi","merdiven","rampa");
            case "structural":return contains(normalized,"statik","sta-","kolon","kiris","perde","temel","radye");
            case "gas":return contains(normalized,"dogalgaz","gaz tesisat","igda");
            case "landscape":return contains(normalized,"peyzaj","bitki","sulama");
            case "infrastructure":return contains(normalized,"altyapi","rogar","kanalizasyon","yagmursuyu");
            case "elevator":return contains(normalized,"asansor","kuyu","makine dairesi");
            default:return false;
        }
    }
    private static boolean contains(String q,String... terms){
        for(String term:terms)if(q.contains(MusaAiDrawingIndex.normalize(term)))return true;
        return false;
    }
    private MusaAiDisciplinePlan(){}
}
