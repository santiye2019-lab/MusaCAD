package com.musa.cad;

import java.util.*;

/**
 * Multidisciplinary deterministic project review over the active CAD index.
 *
 * Local commands:
 *   ARKAI_*      architecture
 *   STATIKAI_*   structural
 *   PROJAI_*     combined architecture + structure + mechanical + coordination
 *
 * Cloud commands use the same names prefixed with G (GARKAI, GSTATIKAI, GPROJAI).
 * Results are review candidates from visible CAD metadata, never design approval.
 */
public final class MusaAiProjectExpert {
    public enum Profile {
        ARCHITECTURE("architecture","Mimari"),
        STRUCTURAL("structural","Statik"),
        COORDINATION("coordination","Disiplinler arası koordinasyon"),
        FULL("project_full","Tüm proje");

        public final String cloudKey,label;
        Profile(String cloudKey,String label){this.cloudKey=cloudKey;this.label=label;}
    }

    public static final class Result {
        public final boolean matched;
        public final Profile profile;
        public final String text;
        public final int findingCount;
        public final List<Integer> sourceIds;
        private Result(boolean matched,Profile profile,String text,int findingCount,Collection<Integer>ids){
            this.matched=matched;this.profile=profile;this.text=text==null?"":text;this.findingCount=Math.max(0,findingCount);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids==null?Collections.emptyList():ids));
        }
        public static Result none(){return new Result(false,null,"",0,Collections.emptyList());}
    }

    private static final class ArchStats {
        int items,walls,doors,windows,rooms,stairs,ramps,shafts,wetAreas,levels,dimensions,axes,review,degenerate;
        final LinkedHashSet<Integer> issueIds=new LinkedHashSet<>();
        final StringBuilder corpus=new StringBuilder();
    }

    private static final class StructStats {
        int items,columns,beams,walls,slabs,foundations,axes,rebar,sizeLabels,levels,review,degenerate;
        final LinkedHashSet<Integer> issueIds=new LinkedHashSet<>();
        final StringBuilder corpus=new StringBuilder();
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        Profile profile=detect(raw);
        if(index==null||profile==null||!isLocalExpertCommand(raw))return Result.none();

        ArchStats arch=scanArchitecture(index);
        StructStats structure=scanStructural(index);
        LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        StringBuilder out=new StringBuilder();
        int findings=0;

        out.append("PROJAI Uzman Kontrol • ").append(profile.label)
           .append(" • ").append(index.layout.isEmpty()?"aktif layout":index.layout);

        if(profile==Profile.ARCHITECTURE||profile==Profile.FULL){
            int before=findings;
            findings+=appendArchitecture(out,arch);
            ids.addAll(arch.issueIds);
            if(profile==Profile.FULL&&findings==before)out.append("\n• Mimari: görünür CAD metadata taramasında belirgin kontrol adayı oluşmadı.");
        }

        if(profile==Profile.STRUCTURAL||profile==Profile.FULL){
            int before=findings;
            findings+=appendStructural(out,structure);
            ids.addAll(structure.issueIds);
            if(profile==Profile.FULL&&findings==before)out.append("\n• Statik: görünür CAD metadata taramasında belirgin kontrol adayı oluşmadı.");
        }

        if(profile==Profile.COORDINATION||profile==Profile.FULL){
            Coordination coordination=scanCoordination(index);
            findings+=appendCoordination(out,coordination,arch,structure);
            ids.addAll(coordination.ids);
        }

        if(profile==Profile.FULL){
            MusaAiMechanicalExpert.Result mechanical=MusaAiMechanicalExpert.analyze(index,"MEKAI_FULL");
            if(mechanical.matched){
                out.append("\n\nMekanik uzman alt raporu\n").append(mechanical.text);
                findings+=mechanical.findingCount;
                ids.addAll(mechanical.sourceIds);
            }
        }

        if(findings==0)out.append("\n• Seçilen kapsamda otomatik uzman taramasında belirgin bir kontrol adayı oluşmadı.");

        out.append("\n\nNot: PROJAI görünür katman, metin, nesne türü ve sınırlı geometri metadata'sını tarar. ")
           .append("Taşıyıcı sistem hesabı, deprem hesabı, yangın/kaçış hesabı, erişilebilirlik, enerji, tesisat hesapları ve mevzuat uygunluğu ")
           .append("ilgili proje girdileri ve yetkili mühendis/mimar değerlendirmesi olmadan onaylanmış sayılmaz.");

        return new Result(true,profile,out.toString(),findings,limit(ids,400));
    }

    public static boolean isHelpCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.equals("projai help")||q.equals("projai yardim")||q.equals("projai komutlari")||
            q.equals("arkai help")||q.equals("statikai help");
    }

    public static String commandHelp(){
        return "MusaCAD çok disiplinli AI komutları"+
            "\n• ARKAI_FULL — mimari proje taraması"+
            "\n• STATIKAI_FULL — statik proje taraması"+
            "\n• PROJAI_COORD — disiplinler arası koordinasyon"+
            "\n• PROJAI_FULL — mimari + statik + mekanik + koordinasyon"+
            "\n\nBulut/Gandalf derin analizi için başına G ekleyin:"+
            "\n• GARKAI_FULL"+
            "\n• GSTATIKAI_FULL"+
            "\n• GPROJAI_COORD"+
            "\n• GPROJAI_FULL"+
            "\n\nMEKAI_* ve GMEKAI_* mekanik alt uzman komutları aynen kullanılmaya devam eder.";
    }

    public static boolean isCloudExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.equals("garkai")||q.startsWith("garkai ")||
            q.equals("gstatikai")||q.startsWith("gstatikai ")||
            q.equals("gprojai")||q.startsWith("gprojai ")||
            q.equals("gandalf projai")||q.startsWith("gandalf projai ");
    }

    public static boolean isLocalExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.equals("arkai")||q.startsWith("arkai ")||
            q.equals("statikai")||q.startsWith("statikai ")||
            q.equals("projai")||q.startsWith("projai ");
    }

    public static String cloudProfile(String raw){
        Profile p=detect(raw);return p==null?"":p.cloudKey;
    }

    public static Profile detect(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return null;
        if(q.equals("arkai")||q.startsWith("arkai ")||q.equals("garkai")||q.startsWith("garkai ")||
           q.contains("mimari uzman")||q.contains("mimari proje uzman"))return Profile.ARCHITECTURE;
        if(q.equals("statikai")||q.startsWith("statikai ")||q.equals("gstatikai")||q.startsWith("gstatikai ")||
           q.contains("statik uzman")||q.contains("statik proje uzman"))return Profile.STRUCTURAL;
        if((q.equals("projai")||q.startsWith("projai ")||q.equals("gprojai")||q.startsWith("gprojai ")||
            q.equals("gandalf projai")||q.startsWith("gandalf projai "))){
            if(has(q,"coord","koordinasyon","cakisma","çakışma"))return Profile.COORDINATION;
            return Profile.FULL;
        }
        return null;
    }

    private static ArchStats scanArchitecture(MusaAiDrawingIndex index){
        ArchStats s=new ArchStats();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String h=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            boolean architectural=has(h,"mimari","architect","duvar","wall","kapi","door","pencere","window","mahal","room",
                "merdiven","stair","rampa","ramp","saft","shaft","wc","banyo","bath","mutfak","kitchen","asma tavan","ceiling",
                "doseme","floor finish","mobilya","furniture");
            if(!architectural)continue;
            s.items++;append(s.corpus,h);
            if(has(h,"duvar","wall"))s.walls++;
            if(has(h,"kapi","door"))s.doors++;
            if(has(h,"pencere","window"))s.windows++;
            if(has(h,"mahal","room","salon","ofis","office","depo","wc","banyo","mutfak"))s.rooms++;
            if(has(h,"merdiven","stair"))s.stairs++;
            if(has(h,"rampa","ramp"))s.ramps++;
            if(has(h,"saft","shaft"))s.shafts++;
            if(has(h,"wc","banyo","bath","mutfak","kitchen","islak hacim"))s.wetAreas++;
            if(has(h,"kot","level","elevation","±","+0.00","0.00"))s.levels++;
            if(has(h,"aks","axis","grid"))s.axes++;
            if("DIMENSION".equals(item.type))s.dimensions++;
            if(reviewMarker(item.text)){s.review++;add(s.issueIds,item);}
            if(item.hasLength()&&item.length<=1e-9d){s.degenerate++;add(s.issueIds,item);}
        }
        return s;
    }

    private static StructStats scanStructural(MusaAiDrawingIndex index){
        StructStats s=new StructStats();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String h=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            boolean structural=has(h,"statik","struct","kolon","column","kiris","beam","perde","shear wall","doseme","slab",
                "temel","foundation","radye","raft","donati","rebar","etriye","stirrup","aks","axis","grid","beton","concrete");
            if(!structural)continue;
            s.items++;append(s.corpus,h);
            if(has(h,"kolon","column"))s.columns++;
            if(has(h,"kiris","beam"))s.beams++;
            if(has(h,"perde","shear wall"))s.walls++;
            if(has(h,"doseme","slab"))s.slabs++;
            if(has(h,"temel","foundation","radye","raft"))s.foundations++;
            if(has(h,"aks","axis","grid"))s.axes++;
            if(has(h,"donati","rebar","etriye","stirrup","ø","phi"))s.rebar++;
            if(sizeMarker(item.text)||sizeMarker(item.layer))s.sizeLabels++;
            if(has(h,"kot","level","elevation","+0.00","0.00"))s.levels++;
            if(reviewMarker(item.text)){s.review++;add(s.issueIds,item);}
            if(item.hasLength()&&item.length<=1e-9d){s.degenerate++;add(s.issueIds,item);}
        }
        return s;
    }

    private static int appendArchitecture(StringBuilder out,ArchStats s){
        out.append("\n\nMimari kontrol");
        if(s.items==0){out.append("\n• Mimari katman/metin eşleşmesi bulunamadı.");return 0;}
        out.append("\n• Mimari aday nesne: ").append(s.items)
           .append(" • duvar ").append(s.walls).append(" • kapı ").append(s.doors)
           .append(" • pencere ").append(s.windows).append(" • mahal ").append(s.rooms);
        int n=0;
        if(s.rooms==0){out.append("\n• Mahal/oda isim veya numara etiketi görünmüyor; mahal listesi/plan paftasını kontrol edin.");n++;}
        if(s.walls>0&&s.dimensions==0){out.append("\n• Mimari planda DIMENSION türünde ölçü nesnesi görünmüyor; ölçülendirme paftasını kontrol edin.");n++;}
        if((s.stairs>0||s.ramps>0)&&s.levels==0){out.append("\n• Merdiven/rampa tespit edildi ancak görünür kot/seviye etiketi bulunamadı.");n++;}
        if(s.wetAreas>0&&s.shafts==0){out.append("\n• Islak hacim işaretleri var ancak görünür şaft etiketi bulunamadı; mekanik düşey koordinasyonunu kontrol edin.");n++;}
        if(s.review>0){out.append("\n• Mimari revizyon/TODO/EKSİK notu: ").append(s.review);n+=s.review;}
        if(s.degenerate>0){out.append("\n• Mimari sıfır uzunluk/dejenere geometri: ").append(s.degenerate);n+=s.degenerate;}
        out.append("\n• İnceleme kapsamı: mahal/kapı/pencere • ölçüler • kotlar • merdiven/rampa • şaft/ıslak hacim • çizim notları.");
        return n;
    }

    private static int appendStructural(StringBuilder out,StructStats s){
        out.append("\n\nStatik kontrol");
        if(s.items==0){out.append("\n• Statik katman/metin eşleşmesi bulunamadı.");return 0;}
        out.append("\n• Statik aday nesne: ").append(s.items)
           .append(" • kolon ").append(s.columns).append(" • kiriş ").append(s.beams)
           .append(" • perde ").append(s.walls).append(" • döşeme ").append(s.slabs)
           .append(" • temel ").append(s.foundations);
        int n=0;
        int primary=s.columns+s.beams+s.walls+s.slabs+s.foundations;
        if(primary>0&&s.axes==0){out.append("\n• Taşıyıcı elemanlar tespit edildi ancak görünür aks/grid etiketi bulunamadı.");n++;}
        if((s.columns+s.beams+s.walls)>0&&s.sizeLabels==0){out.append("\n• Kolon/kiriş/perde adayları var ancak görünür kesit/ebat etiketi bulunamadı.");n++;}
        if(primary>0&&s.rebar==0){out.append("\n• Taşıyıcı sistem işaretleri var ancak görünür donatı/etriye referansı bulunamadı; ilgili donatı paftasını kontrol edin.");n++;}
        if(primary>0&&s.levels==0){out.append("\n• Statik elemanlar var ancak görünür kot/seviye referansı bulunamadı.");n++;}
        if(s.review>0){out.append("\n• Statik revizyon/TODO/EKSİK notu: ").append(s.review);n+=s.review;}
        if(s.degenerate>0){out.append("\n• Statik sıfır uzunluk/dejenere geometri: ").append(s.degenerate);n+=s.degenerate;}
        out.append("\n• İnceleme kapsamı: akslar • kolon/kiriş/perde/döşeme/temel • kesit etiketleri • donatı referansları • kotlar.");
        return n;
    }

    private static final class Coordination {
        int exactCrossDisciplineGeometry;
        boolean structural,mechanical,openingOrShaft;
        final LinkedHashSet<Integer>ids=new LinkedHashSet<>();
    }

    private static Coordination scanCoordination(MusaAiDrawingIndex index){
        Coordination c=new Coordination();
        LinkedHashMap<String,MusaAiDrawingIndex.Item>structByGeometry=new LinkedHashMap<>();
        LinkedHashMap<String,MusaAiDrawingIndex.Item>mechByGeometry=new LinkedHashMap<>();
        LinkedHashMap<String,MusaAiDrawingIndex.Item>archByGeometry=new LinkedHashMap<>();

        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String h=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            boolean st=has(h,"statik","struct","kolon","column","kiris","beam","perde","shear wall","doseme","slab","temel","foundation");
            boolean me=has(h,"pis su","atik su","temiz su","yangin","sprinkler","havalandirma","duct","vrf","dogalgaz","heating","pompa","mekanik");
            boolean ar=has(h,"mimari","architect","duvar","wall","kapi","door","mahal","room","saft","shaft");
            c.structural|=st;c.mechanical|=me;c.openingOrShaft|=has(h,"saft","shaft","rezervasyon","opening","delik","bosluk","boşluk");
            if(item.geometryKey.isEmpty())continue;
            if(st)structByGeometry.putIfAbsent(item.geometryKey,item);
            if(me)mechByGeometry.putIfAbsent(item.geometryKey,item);
            if(ar)archByGeometry.putIfAbsent(item.geometryKey,item);
        }

        LinkedHashSet<String>keys=new LinkedHashSet<>(structByGeometry.keySet());
        keys.retainAll(mechByGeometry.keySet());
        for(String key:keys){
            c.exactCrossDisciplineGeometry++;
            add(c.ids,structByGeometry.get(key));add(c.ids,mechByGeometry.get(key));
        }
        LinkedHashSet<String>archStruct=new LinkedHashSet<>(archByGeometry.keySet());
        archStruct.retainAll(structByGeometry.keySet());
        for(String key:archStruct){
            c.exactCrossDisciplineGeometry++;
            add(c.ids,archByGeometry.get(key));add(c.ids,structByGeometry.get(key));
        }
        return c;
    }

    private static int appendCoordination(StringBuilder out,Coordination c,ArchStats arch,StructStats structure){
        out.append("\n\nKoordinasyon kontrolü");int n=0;
        if(c.exactCrossDisciplineGeometry>0){
            out.append("\n• Disiplinler arasında birebir aynı geometri anahtarına sahip ").append(c.exactCrossDisciplineGeometry)
               .append(" aday bulundu; mükerrer çizim veya gerçek ortak sınır olup olmadığını kontrol edin.");
            n+=c.exactCrossDisciplineGeometry;
        }
        if(c.structural&&c.mechanical&&!c.openingOrShaft){
            out.append("\n• Statik ve mekanik sistem işaretleri birlikte görülüyor ancak görünür şaft/rezervasyon/delik etiketi bulunamadı; geçiş koordinasyon paftasını kontrol edin.");
            n++;
        }
        if(arch.wetAreas>0&&arch.shafts==0){
            out.append("\n• Islak hacim–mekanik düşey şaft koordinasyonu için görünür şaft referansı eksik görünüyor.");
            n++;
        }
        if((structure.columns+structure.beams+structure.walls)>0&&c.mechanical){
            out.append("\n• Taşıyıcı elemanlarla mekanik hat/ekipmanların gerçek çakışması yalnız metadata ile doğrulanamaz; geometrik clash kontrolü için çoklu pafta/3B koordinat karşılaştırması gerekir.");
        }
        if(n==0)out.append("\n• Metadata tabanlı koordinasyon taramasında belirgin aday oluşmadı.");
        return n;
    }

    private static boolean reviewMarker(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return has(q,"todo","tbd","fixme","eksik","revize","revizyon","duzelt","kontrol et");
    }

    private static boolean sizeMarker(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d{2,4}\\s*x\\s*\\d{2,4}\\b.*")||q.matches(".*\\b\\d{2,4}\\s*/\\s*\\d{2,4}\\b.*");
    }

    private static void append(StringBuilder b,String text){if(text!=null&&!text.isEmpty())b.append(' ').append(text);}
    private static void add(Set<Integer>out,MusaAiDrawingIndex.Item item){if(item!=null&&item.sourceId>=0)out.add(item.sourceId);}
    private static boolean has(String q,String...terms){
        if(q==null)return false;
        for(String term:terms){
            String wanted=MusaAiDrawingIndex.normalize(term);
            if(!wanted.isEmpty()&&q.contains(wanted))return true;
        }
        return false;
    }
    private static Collection<Integer>limit(Collection<Integer>ids,int max){
        ArrayList<Integer>out=new ArrayList<>();if(ids==null)return out;
        for(Integer id:ids){if(id!=null&&id>=0)out.add(id);if(out.size()>=max)break;}
        return out;
    }

    private MusaAiProjectExpert(){}
}
