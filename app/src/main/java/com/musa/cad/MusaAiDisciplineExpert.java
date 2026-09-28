package com.musa.cad;

import java.util.*;

/**
 * Deterministic expert profiles for non-mechanical MusaCAD disciplines.
 * Local commands run on-device. G-prefixed commands route the same trusted
 * profile to Gandalf Cloud AI. Results are review candidates, not approvals.
 */
public final class MusaAiDisciplineExpert {
    public enum Profile {
        ARCH_FULL(MusaAiDiscipline.ARCHITECTURAL,"arch_full","Mimari • Tüm proje"),
        ARCH_ACCESS(MusaAiDiscipline.ARCHITECTURAL,"arch_access","Mimari • Erişilebilirlik"),
        ARCH_ESCAPE(MusaAiDiscipline.ARCHITECTURAL,"arch_escape","Mimari • Kaçış / dolaşım"),
        ARCH_SPACE(MusaAiDiscipline.ARCHITECTURAL,"arch_space","Mimari • Mahal / kapı / geçiş"),
        ARCH_ENVELOPE(MusaAiDiscipline.ARCHITECTURAL,"arch_envelope","Mimari • Cephe / çatı"),

        STRUCT_FULL(MusaAiDiscipline.STRUCTURAL,"structural_full","Statik • Tüm taşıyıcı sistem"),
        STRUCT_FRAME(MusaAiDiscipline.STRUCTURAL,"structural_frame","Statik • Kolon / kiriş / perde / döşeme"),
        STRUCT_FOUNDATION(MusaAiDiscipline.STRUCTURAL,"structural_foundation","Statik • Temel"),
        STRUCT_OPENINGS(MusaAiDiscipline.STRUCTURAL,"structural_openings","Statik • Boşluk / rezervasyon"),
        STRUCT_STAIRS(MusaAiDiscipline.STRUCTURAL,"structural_stairs","Statik • Merdiven / asansör kuyusu"),

        ELEC_FULL(MusaAiDiscipline.ELECTRICAL,"electrical_full","Elektrik • Tüm proje"),
        ELEC_POWER(MusaAiDiscipline.ELECTRICAL,"electrical_power","Elektrik • Kuvvetli akım"),
        ELEC_LIGHTING(MusaAiDiscipline.ELECTRICAL,"electrical_lighting","Elektrik • Aydınlatma"),
        ELEC_WEAK(MusaAiDiscipline.ELECTRICAL,"electrical_weak","Elektrik • Zayıf akım"),
        ELEC_GROUNDING(MusaAiDiscipline.ELECTRICAL,"electrical_grounding","Elektrik • Topraklama / yıldırımdan korunma"),
        ELEC_EMERGENCY(MusaAiDiscipline.ELECTRICAL,"electrical_emergency","Elektrik • Jeneratör / UPS / acil durum"),

        LAND_FULL(MusaAiDiscipline.LANDSCAPE,"landscape_full","Peyzaj • Tüm proje"),
        LAND_HARD(MusaAiDiscipline.LANDSCAPE,"landscape_hard","Peyzaj • Sert zemin"),
        LAND_SOFT(MusaAiDiscipline.LANDSCAPE,"landscape_soft","Peyzaj • Bitkilendirme"),
        LAND_IRRIGATION(MusaAiDiscipline.LANDSCAPE,"landscape_irrigation","Peyzaj • Sulama"),
        LAND_DRAINAGE(MusaAiDiscipline.LANDSCAPE,"landscape_drainage","Peyzaj • Drenaj"),

        INFRA_FULL(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_full","Altyapı • Tüm proje"),
        INFRA_WASTE(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_waste","Altyapı • Atık su"),
        INFRA_RAIN(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_rain","Altyapı • Yağmur suyu"),
        INFRA_WATER(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_water","Altyapı • İçme / kullanma suyu"),
        INFRA_UTILITIES(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_utilities","Altyapı • Enerji / gaz / telekom"),
        INFRA_LEVELS(MusaAiDiscipline.INFRASTRUCTURE,"infrastructure_levels","Altyapı • Kot / eğim / rögar"),

        ELEV_FULL(MusaAiDiscipline.ELEVATOR,"elevator_full","Asansör • Tüm proje"),
        ELEV_SHAFT(MusaAiDiscipline.ELEVATOR,"elevator_shaft","Asansör • Kuyu / kuyu dibi / üst boşluk"),
        ELEV_DOOR(MusaAiDiscipline.ELEVATOR,"elevator_door","Asansör • Kapı / erişim"),
        ELEV_MACHINE(MusaAiDiscipline.ELEVATOR,"elevator_machine","Asansör • Makine / tahrik"),
        ELEV_ELECTRICAL(MusaAiDiscipline.ELEVATOR,"elevator_electrical","Asansör • Elektrik / kumanda"),
        ELEV_FIRE(MusaAiDiscipline.ELEVATOR,"elevator_fire","Asansör • Yangın senaryosu"),

        FIRE_FULL(MusaAiDiscipline.FIRE_SAFETY,"fire_safety_full","Yangın • Tüm can güvenliği"),
        FIRE_ESCAPE(MusaAiDiscipline.FIRE_SAFETY,"fire_escape","Yangın • Kaçış / yangın kapıları"),
        FIRE_SPRINKLER(MusaAiDiscipline.FIRE_SAFETY,"fire_sprinkler","Yangın • Sprinkler"),
        FIRE_HYDRANT(MusaAiDiscipline.FIRE_SAFETY,"fire_hydrant","Yangın • Hidrant / dolap / itfaiye bağlantısı"),
        FIRE_DETECTION(MusaAiDiscipline.FIRE_SAFETY,"fire_detection","Yangın • Algılama / ihbar"),
        FIRE_SMOKE(MusaAiDiscipline.FIRE_SAFETY,"fire_smoke","Yangın • Duman kontrolü / basınçlandırma"),
        FIRE_PUMP(MusaAiDiscipline.FIRE_SAFETY,"fire_pump","Yangın • Pompa / depo");

        public final MusaAiDiscipline discipline;
        public final String cloudKey,label;
        Profile(MusaAiDiscipline discipline,String cloudKey,String label){
            this.discipline=discipline;this.cloudKey=cloudKey;this.label=label;
        }
    }

    public static final class Result {
        public final boolean matched;
        public final Profile profile;
        public final String text;
        public final int findingCount;
        public final List<Integer> sourceIds;
        private Result(boolean matched,Profile profile,String text,int count,Collection<Integer>ids){
            this.matched=matched;this.profile=profile;this.text=text==null?"":text;this.findingCount=Math.max(0,count);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(ids==null?Collections.emptyList():ids));
        }
        public static Result none(){return new Result(false,null,"",0,Collections.emptyList());}
    }

    private static final class Stats {
        int items,linear,degenerate,openRuns;
        boolean dimension,level,slope,tag;
        final LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        final LinkedHashSet<Integer> issueIds=new LinkedHashSet<>();
        final StringBuilder corpus=new StringBuilder();
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        Profile p=detect(raw);
        if(index==null||p==null||!isLocalExpertCommand(raw))return Result.none();
        Stats st=scan(index,p);
        StringBuilder out=new StringBuilder();
        LinkedHashSet<Integer>highlights=new LinkedHashSet<>(st.issueIds);
        int findings=0;

        out.append(commandFamily(p)).append(" Uzman Kontrol • ").append(p.label)
           .append(" • ").append(index.layout.isEmpty()?"aktif layout":index.layout);
        out.append("\nSistem özeti");
        out.append("\n• İlgili görünür nesne: ").append(st.items);
        if(st.linear>0)out.append(" • çizgisel öğe: ").append(st.linear);
        if(st.items==0)out.append("\n• Bu uzmanlık profilinde güvenilir katman/metin eşleşmesi bulunamadı.");

        out.append("\nUzman inceleme adayları");
        if(st.degenerate>0){
            out.append("\n• Sıfır uzunluk / dejenere geometri: ").append(st.degenerate);
            findings+=st.degenerate;highlights.addAll(st.issueIds);
        }
        if(st.openRuns>0&&expectsContinuity(p)){
            out.append("\n• Açık polyline / süreklilik adayı: ").append(st.openRuns);
            findings+=st.openRuns;highlights.addAll(st.issueIds);
        }
        findings+=appendChecks(out,p,st);
        if(findings==0)out.append("\n• Görünür CAD verisinden belirgin uzman kontrol adayı oluşmadı.");

        if(findings>0&&highlights.isEmpty())highlights.addAll(st.ids);
        out.append("\nKontrol kapsamı");
        appendChecklist(out,p);
        out.append("\nNot: ").append(commandFamily(p))
           .append(" yalnız görünür CAD katmanları, metinleri ve sınırlı geometri metadata'sını tarar. ")
           .append("Hesap, yönetmelik uygunluğu, üretici seçimi, saha koşulu ve resmi mühendislik onayı ayrıca doğrulanmalıdır.");
        return new Result(true,p,out.toString(),findings,limit(highlights,300));
    }

    public static Profile detect(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return null;
        Family f=family(q);if(f==null)return null;
        switch(f){
            case ARCH:
                if(has(q,"erisilebilir","engelli","access"))return Profile.ARCH_ACCESS;
                if(has(q,"kacis","dol asim","dolasim","escape","sirkulasyon"))return Profile.ARCH_ESCAPE;
                if(has(q,"mahal","kapi","gecis","door","room","space"))return Profile.ARCH_SPACE;
                if(has(q,"cephe","cati","facade","roof","envelope"))return Profile.ARCH_ENVELOPE;
                return Profile.ARCH_FULL;
            case STRUCT:
                if(has(q,"temel","radye","kazik","foundation"))return Profile.STRUCT_FOUNDATION;
                if(has(q,"rezervasyon","bosluk","delik","opening","gecis"))return Profile.STRUCT_OPENINGS;
                if(has(q,"merdiven","asansor","kuyu","stair","shaft"))return Profile.STRUCT_STAIRS;
                if(has(q,"kolon","kiris","perde","doseme","frame"))return Profile.STRUCT_FRAME;
                return Profile.STRUCT_FULL;
            case ELEC:
                if(has(q,"aydinlat","lighting","lux"))return Profile.ELEC_LIGHTING;
                if(has(q,"zayif","data","cctv","telefon","network","weak"))return Profile.ELEC_WEAK;
                if(has(q,"toprak","paratoner","lightning","ground"))return Profile.ELEC_GROUNDING;
                if(has(q,"jenerator","ups","acil","emergency"))return Profile.ELEC_EMERGENCY;
                if(has(q,"kuvvet","pano","kablo","priz","power"))return Profile.ELEC_POWER;
                return Profile.ELEC_FULL;
            case LAND:
                if(has(q,"sulama","irrigation"))return Profile.LAND_IRRIGATION;
                if(has(q,"drenaj","drain"))return Profile.LAND_DRAINAGE;
                if(has(q,"bitki","agac","cim","soft","plant","softscape"))return Profile.LAND_SOFT;
                if(has(q,"sert","yol","bordur","paving","hardscape","hard"))return Profile.LAND_HARD;
                return Profile.LAND_FULL;
            case INFRA:
                if(has(q,"atik","pis su","kanalizasyon","waste","sewer"))return Profile.INFRA_WASTE;
                if(has(q,"yagmur","rain","storm"))return Profile.INFRA_RAIN;
                if(has(q,"icme","kullanma suyu","water"))return Profile.INFRA_WATER;
                if(has(q,"elektrik","telekom","dogalgaz","utility","enerji"))return Profile.INFRA_UTILITIES;
                if(has(q,"kot","egim","rogar","level","slope"))return Profile.INFRA_LEVELS;
                return Profile.INFRA_FULL;
            case ELEV:
                if(has(q,"yangin","fire"))return Profile.ELEV_FIRE;
                if(has(q,"elektrik","electrical","kumanda","pano","control"))return Profile.ELEV_ELECTRICAL;
                if(has(q,"makine","motor","tahrik","machine","drive"))return Profile.ELEV_MACHINE;
                if(has(q,"kapi","door","eris"))return Profile.ELEV_DOOR;
                if(has(q,"kuyu","pit","ust bosluk","overhead","shaft"))return Profile.ELEV_SHAFT;
                return Profile.ELEV_FULL;
            case FIRE:
                if(has(q,"sprinkler"))return Profile.FIRE_SPRINKLER;
                if(has(q,"hidrant","dolap","itfaiye","fdc"))return Profile.FIRE_HYDRANT;
                if(has(q,"algilama","ihbar","dedektor","detector","detection","alarm"))return Profile.FIRE_DETECTION;
                if(has(q,"duman","basinclandirma","smoke","pressur"))return Profile.FIRE_SMOKE;
                if(has(q,"pompa","jokey","depo","pump","tank"))return Profile.FIRE_PUMP;
                if(has(q,"kacis","yangin kapisi","escape","exit"))return Profile.FIRE_ESCAPE;
                return Profile.FIRE_FULL;
            default:return null;
        }
    }

    public static boolean isLocalExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);Family f=family(q);
        return f!=null&&!isCloudPrefix(q);
    }

    public static boolean isCloudExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);return family(q)!=null&&isCloudPrefix(q);
    }

    public static String cloudProfile(String raw){
        Profile p=detect(raw);return p==null?"":p.cloudKey;
    }

    public static boolean isHelpCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(!(q.contains("help")||q.contains("yardim")||q.contains("komut")))return false;
        return family(q)!=null||q.equals("disiplin ai yardim")||q.equals("uzman ai yardim");
    }

    public static String commandHelp(){
        return "MusaCAD disiplin uzman komutları"+
            "\n• MIMAI_FULL / GMIMAI_FULL — mimari"+
            "\n  ACCESS, ESCAPE, SPACE, ENVELOPE"+
            "\n• STATIKAI_FULL / GSTATIKAI_FULL — statik"+
            "\n  FRAME, FOUNDATION, OPENINGS, STAIRS"+
            "\n• ELKAI_FULL / GELKAI_FULL — elektrik"+
            "\n  POWER, LIGHTING, WEAK, GROUNDING, EMERGENCY"+
            "\n• PEYAI_FULL / GPEYAI_FULL — peyzaj"+
            "\n  HARD, SOFT, IRRIGATION, DRAINAGE"+
            "\n• ALTYAPIAI_FULL / GALTYAPIAI_FULL — altyapı"+
            "\n  WASTE, RAIN, WATER, UTILITIES, LEVELS"+
            "\n• ASNAI_FULL / GASNAI_FULL — asansör"+
            "\n  SHAFT, DOOR, MACHINE, ELECTRICAL, FIRE"+
            "\n• YANGAI_FULL / GYANGAI_FULL — yangın ve can güvenliği"+
            "\n  ESCAPE, SPRINKLER, HYDRANT, DETECTION, SMOKE, PUMP"+
            "\n\nG ile başlayan komut Gandalf derin analizine gider; yerel komut çevrimdışı ön kontrol yapar. "+
            "Çizim değişikliği önerileri kullanıcı onayı olmadan uygulanmaz.";
    }

    private enum Family { ARCH,STRUCT,ELEC,LAND,INFRA,ELEV,FIRE }

    private static Family family(String q){
        if(q==null||q.isEmpty())return null;
        if(starts(q,"mimai","gmimai","mimari ai","g mimari ai"))return Family.ARCH;
        if(starts(q,"statikai","gstatikai","stai","gstai","statik ai","g statik ai"))return Family.STRUCT;
        if(starts(q,"elkai","gelkai","elekai","gele kai","elektrik ai","g elektrik ai"))return Family.ELEC;
        if(starts(q,"peyai","gpeyai","peyzaj ai","g peyzaj ai"))return Family.LAND;
        if(starts(q,"altyapiai","galtyapiai","altai","galtai","altyapi ai","g altyapi ai"))return Family.INFRA;
        if(starts(q,"asnai","gasnai","asansor ai","g asansor ai"))return Family.ELEV;
        if(starts(q,"yangai","gyangai","yangin ai","g yangin ai","can guvenligi ai"))return Family.FIRE;
        return null;
    }

    private static boolean isCloudPrefix(String q){
        return starts(q,"gmimai","gstatikai","gstai","gelkai","gpeyai","galtyapiai","galtai","gasnai","gyangai",
            "g mimari ai","g statik ai","g elektrik ai","g peyzaj ai","g altyapi ai","g asansor ai","g yangin ai");
    }

    private static Stats scan(MusaAiDrawingIndex index,Profile p){
        Stats st=new Stats();
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            if(!belongs(p,hay))continue;
            st.items++;
            if(item.hasLength()){
                st.linear++;
                if(item.length<=1e-9d){st.degenerate++;if(item.sourceId>=0)st.issueIds.add(item.sourceId);}
            }
            if(isPolyline(item.type)&&item.closedKnown&&!item.closed){
                st.openRuns++;if(item.sourceId>=0)st.issueIds.add(item.sourceId);
            }
            st.dimension|=hasDimension(item.layer)||hasDimension(item.text);
            st.level|=has(hay,"kot","level","elevation");
            st.slope|=item.text.contains("%")||item.text.contains("‰")||has(hay,"egim","slope","meyil");
            st.tag|=hasTag(item.text);
            if(item.sourceId>=0)st.ids.add(item.sourceId);
            if(!item.text.isEmpty())st.corpus.append(' ').append(MusaAiDrawingIndex.normalize(item.text));
            if(!item.layer.isEmpty())st.corpus.append(' ').append(MusaAiDrawingIndex.normalize(item.layer));
        }
        return st;
    }

    private static boolean belongs(Profile p,String hay){
        MusaAiDiscipline d=MusaAiDiscipline.classify(hay);
        if(d==p.discipline)return profileMatches(p,hay);
        if(p.discipline==MusaAiDiscipline.FIRE_SAFETY&&has(hay,"yangin","sprinkler","hidrant","duman","itfaiye","alarm"))return profileMatches(p,hay);
        if(p.discipline==MusaAiDiscipline.ELEVATOR&&has(hay,"asansor","kuyu","kabin","lift","elevator"))return profileMatches(p,hay);
        return false;
    }

    private static boolean profileMatches(Profile p,String h){
        switch(p){
            case ARCH_ACCESS:return has(h,"engelli","erisilebilir","rampa","access");
            case ARCH_ESCAPE:return has(h,"kacis","koridor","merdiven","cikis","escape","sirkulasyon");
            case ARCH_SPACE:return has(h,"mahal","kapi","gecis","oda","room","door");
            case ARCH_ENVELOPE:return has(h,"cephe","cati","yalitim","facade","roof");
            case STRUCT_FRAME:return has(h,"kolon","kiris","perde","doseme");
            case STRUCT_FOUNDATION:return has(h,"temel","radye","kazik","foundation");
            case STRUCT_OPENINGS:return has(h,"rezervasyon","bosluk","delik","gecis","opening");
            case STRUCT_STAIRS:return has(h,"merdiven","asansor","kuyu","stair");
            case ELEC_POWER:return has(h,"pano","kablo","tava","priz","kuvvet","busbar");
            case ELEC_LIGHTING:return has(h,"aydinlat","armat ur","armatur","lighting","lux");
            case ELEC_WEAK:return has(h,"zayif","data","cctv","telefon","yangin alarm","network");
            case ELEC_GROUNDING:return has(h,"toprak","paratoner","lightning","ground");
            case ELEC_EMERGENCY:return has(h,"jenerator","ups","acil","emergency");
            case LAND_HARD:return has(h,"sert","yol","bordur","paving","hardscape");
            case LAND_SOFT:return has(h,"bitki","agac","cim","plant","softscape");
            case LAND_IRRIGATION:return has(h,"sulama","irrigation");
            case LAND_DRAINAGE:return has(h,"drenaj","drain","yagmur");
            case INFRA_WASTE:return has(h,"atik","pis su","kanalizasyon","sewer","waste");
            case INFRA_RAIN:return has(h,"yagmur","storm","rain");
            case INFRA_WATER:return has(h,"icme","kullanma suyu","water");
            case INFRA_UTILITIES:return has(h,"elektrik","telekom","dogalgaz","enerji","utility");
            case INFRA_LEVELS:return has(h,"kot","egim","rogar","level","slope");
            case ELEV_SHAFT:return has(h,"kuyu","pit","ust bosluk","overhead");
            case ELEV_DOOR:return has(h,"kapi","door","eris");
            case ELEV_MACHINE:return has(h,"makine","motor","tahrik","machine","drive");
            case ELEV_ELECTRICAL:return has(h,"elektrik","kumanda","pano","control");
            case ELEV_FIRE:return has(h,"yangin","fire","itfaiye");
            case FIRE_ESCAPE:return has(h,"kacis","yangin kapisi","cikis","escape");
            case FIRE_SPRINKLER:return has(h,"sprinkler");
            case FIRE_HYDRANT:return has(h,"hidrant","yangin dolabi","itfaiye","fdc");
            case FIRE_DETECTION:return has(h,"algilama","ihbar","dedektor","detector","alarm");
            case FIRE_SMOKE:return has(h,"duman","basinclandirma","smoke","pressur");
            case FIRE_PUMP:return has(h,"pompa","jokey","depo","pump","tank");
            default:return true;
        }
    }

    private static int appendChecks(StringBuilder out,Profile p,Stats st){
        String c=st.corpus.toString();int n=0;
        switch(p.discipline){
            case ARCHITECTURAL:
                if(st.items>0&&!st.dimension){out.append("\n• Mimari: görünür ölçü/dimension bilgisi sınırlı; net geçiş ve mahal ölçülerini doğrulayın.");n++;}
                if((p==Profile.ARCH_FULL||p==Profile.ARCH_ESCAPE)&&!has(c,"kacis","cikis","merdiven","exit")){out.append("\n• Mimari: kaçış/çıkış referansı görünmüyor.");n++;}
                if((p==Profile.ARCH_FULL||p==Profile.ARCH_ACCESS)&&!has(c,"engelli","erisilebilir","rampa","access")){out.append("\n• Mimari: erişilebilirlik/rampa referansı görünmüyor.");n++;}
                if((p==Profile.ARCH_FULL||p==Profile.ARCH_SPACE)&&!has(c,"mahal","room","kapi","door")){out.append("\n• Mimari: mahal/kapı tanımları görünür veride sınırlı.");n++;}
                break;
            case STRUCTURAL:
                if((p==Profile.STRUCT_FULL||p==Profile.STRUCT_FRAME)&&!has(c,"kolon")){out.append("\n• Statik: kolon etiketi/katmanı görünmüyor.");n++;}
                if((p==Profile.STRUCT_FULL||p==Profile.STRUCT_FRAME)&&!has(c,"kiris")){out.append("\n• Statik: kiriş etiketi/katmanı görünmüyor.");n++;}
                if((p==Profile.STRUCT_FULL||p==Profile.STRUCT_FOUNDATION)&&!has(c,"temel","radye","kazik")){out.append("\n• Statik: temel sistemi referansı görünmüyor.");n++;}
                if((p==Profile.STRUCT_FULL||p==Profile.STRUCT_OPENINGS)&&has(c,"delik","bosluk","rezervasyon","gecis")){out.append("\n• Statik: taşıyıcı sistemde rezervasyon/delik/geçiş koordinasyonu doğrulanmalı.");n++;highlightsFromKeywords(st,c);}
                break;
            case ELECTRICAL:
                if((p==Profile.ELEC_FULL||p==Profile.ELEC_POWER)&&!has(c,"pano","panel")){out.append("\n• Elektrik: pano referansı görünmüyor.");n++;}
                if((p==Profile.ELEC_FULL||p==Profile.ELEC_POWER)&&!has(c,"kablo","tava","busbar")){out.append("\n• Elektrik: kablo/tava/ana dağıtım güzergâhı etiketi görünmüyor.");n++;}
                if((p==Profile.ELEC_FULL||p==Profile.ELEC_GROUNDING)&&!has(c,"toprak","ground")){out.append("\n• Elektrik: topraklama referansı görünmüyor.");n++;}
                if((p==Profile.ELEC_FULL||p==Profile.ELEC_EMERGENCY)&&!has(c,"jenerator","ups","acil")){out.append("\n• Elektrik: jeneratör/UPS/acil enerji referansı görünmüyor; proje kapsamına göre doğrulayın.");n++;}
                if((p==Profile.ELEC_FULL||p==Profile.ELEC_POWER)&&has(c,"pompa besleme","fan besleme","vrf besleme","asansor besleme")){out.append("\n• Elektrik: mekanik/asansör ekipman beslemeleri disiplinler arası güç ve kumanda kontrolü gerektiriyor.");n++;}
                break;
            case LANDSCAPE:
                if((p==Profile.LAND_FULL||p==Profile.LAND_IRRIGATION)&&!has(c,"sulama","irrigation")){out.append("\n• Peyzaj: sulama sistemi referansı görünmüyor.");n++;}
                if((p==Profile.LAND_FULL||p==Profile.LAND_DRAINAGE)&&!has(c,"drenaj","drain","yagmur")){out.append("\n• Peyzaj: drenaj/yağmur suyu referansı görünmüyor.");n++;}
                if((p==Profile.LAND_FULL||p==Profile.LAND_SOFT)&&!has(c,"agac","bitki","cim","plant")){out.append("\n• Peyzaj: bitkilendirme etiketi görünmüyor.");n++;}
                if(has(c,"altyapi","rogar","hat","kablo")){out.append("\n• Peyzaj: altyapı hatlarıyla kök bölgesi/sert zemin koordinasyonu kontrol edilmeli.");n++;}
                break;
            case INFRASTRUCTURE:
                if((p==Profile.INFRA_FULL||p==Profile.INFRA_LEVELS)&&!st.level){out.append("\n• Altyapı: görünür kot/level bilgisi bulunamadı.");n++;}
                if((p==Profile.INFRA_FULL||p==Profile.INFRA_LEVELS||p==Profile.INFRA_WASTE||p==Profile.INFRA_RAIN)&&!st.slope){out.append("\n• Altyapı: görünür eğim/meyil bilgisi bulunamadı.");n++;}
                if((p==Profile.INFRA_FULL||p==Profile.INFRA_LEVELS)&&!has(c,"rogar","baca","manhole")){out.append("\n• Altyapı: rögar/baca referansı görünmüyor.");n++;}
                if(!has(c,"baglanti","connection","kurum")){out.append("\n• Altyapı: kurum/şebeke bağlantı noktası görünür veride tanımlı değil.");n++;}
                break;
            case ELEVATOR:
                if((p==Profile.ELEV_FULL||p==Profile.ELEV_SHAFT)&&!st.dimension){out.append("\n• Asansör: kuyu/boşluk ölçü bilgisi görünür veride sınırlı.");n++;}
                if((p==Profile.ELEV_FULL||p==Profile.ELEV_DOOR)&&!has(c,"kapi","door")){out.append("\n• Asansör: kapı/net açıklık referansı görünmüyor.");n++;}
                if((p==Profile.ELEV_FULL||p==Profile.ELEV_ELECTRICAL)&&!has(c,"elektrik","pano","besleme","kumanda")){out.append("\n• Asansör: elektrik besleme/kumanda referansı görünmüyor.");n++;}
                if((p==Profile.ELEV_FULL||p==Profile.ELEV_FIRE)&&!has(c,"yangin","itfaiye","fire")){out.append("\n• Asansör: yangın senaryosu/itfaiyeci kullanımı referansı görünmüyor; bina kullanımına göre kontrol edin.");n++;}
                break;
            case FIRE_SAFETY:
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_ESCAPE)&&!has(c,"kacis","cikis","yangin kapisi","exit")){out.append("\n• Yangın: kaçış/çıkış/yangın kapısı referansı görünmüyor.");n++;}
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_SPRINKLER)&&!has(c,"sprinkler")){out.append("\n• Yangın: sprinkler referansı görünmüyor; gereklilik bina kullanımına göre ayrıca değerlendirilmelidir.");n++;}
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_HYDRANT)&&!has(c,"hidrant","yangin dolabi","itfaiye","fdc")){out.append("\n• Yangın: hidrant/dolap/itfaiye bağlantısı referansı görünmüyor.");n++;}
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_DETECTION)&&!has(c,"algilama","ihbar","dedektor","alarm")){out.append("\n• Yangın: algılama/ihbar referansı görünmüyor.");n++;}
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_SMOKE)&&!has(c,"duman","basinclandirma","smoke")){out.append("\n• Yangın: duman kontrolü/basınçlandırma referansı görünmüyor; bina tipine göre kontrol edin.");n++;}
                if((p==Profile.FIRE_FULL||p==Profile.FIRE_PUMP)&&!has(c,"pompa","jokey","depo","tank")){out.append("\n• Yangın: pompa/jokey/depo referansı görünmüyor.");n++;}
                break;
            default:break;
        }
        return n;
    }

    private static void appendChecklist(StringBuilder out,Profile p){
        switch(p.discipline){
            case ARCHITECTURAL:out.append("\n• Mahaller • kapılar/geçişler • erişilebilirlik • kaçış • kot/ölçü • cephe/çatı • şaftlar");break;
            case STRUCTURAL:out.append("\n• Kolon • kiriş • perde • döşeme • temel • merdiven/kuyu • rezervasyonlar • disiplin geçişleri");break;
            case ELECTRICAL:out.append("\n• Panolar • kablo/tava • aydınlatma • priz • zayıf akım • topraklama • jeneratör/UPS • mekanik beslemeler");break;
            case LANDSCAPE:out.append("\n• Sert/yumuşak zemin • bitkilendirme • sulama • drenaj • aydınlatma • altyapı çakışmaları");break;
            case INFRASTRUCTURE:out.append("\n• Atık/yağmur suyu • içme suyu • enerji/gaz/telekom • rögar • kot/eğim • kurum bağlantıları");break;
            case ELEVATOR:out.append("\n• Kuyu • kuyu dibi • üst boşluk • kapı • makine/tahrik • elektrik/kumanda • yangın senaryosu");break;
            case FIRE_SAFETY:out.append("\n• Kaçış • yangın kapıları • sprinkler • hidrant/dolap • algılama • duman kontrolü • pompa/depo • itfaiye erişimi");break;
            default:break;
        }
    }

    private static String commandFamily(Profile p){
        switch(p.discipline){
            case ARCHITECTURAL:return "MIMAI";
            case STRUCTURAL:return "STATIKAI";
            case ELECTRICAL:return "ELKAI";
            case LANDSCAPE:return "PEYAI";
            case INFRASTRUCTURE:return "ALTYAPIAI";
            case ELEVATOR:return "ASNAI";
            case FIRE_SAFETY:return "YANGAI";
            default:return "AI";
        }
    }

    private static boolean expectsContinuity(Profile p){
        return p.discipline==MusaAiDiscipline.INFRASTRUCTURE||
            p==Profile.ELEC_POWER||p==Profile.ELEC_WEAK||p==Profile.LAND_IRRIGATION||p==Profile.LAND_DRAINAGE;
    }
    private static boolean isPolyline(String type){return "POLYLINE".equals(type)||"LWPOLYLINE".equals(type);}
    private static boolean hasDimension(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*[x×]\\s*\\d+(?:[.,]\\d+)?\\b.*")||
            q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*(mm|cm|m)\\b.*")||has(q,"olcu","dimension","genislik","yukseklik");
    }
    private static boolean hasTag(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b[a-z]{1,5}[-_]?\\d{1,4}\\b.*");
    }
    private static void highlightsFromKeywords(Stats st,String corpus){/* marker for future per-finding source refinement */}
    private static boolean starts(String q,String...prefixes){
        for(String p:prefixes){String n=MusaAiDrawingIndex.normalize(p);if(q.equals(n)||q.startsWith(n+" "))return true;}
        return false;
    }
    private static boolean has(String q,String...terms){
        if(q==null)return false;for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;return false;
    }
    private static Collection<Integer>limit(Collection<Integer>ids,int max){
        ArrayList<Integer>out=new ArrayList<>();if(ids==null)return out;
        for(Integer id:ids){if(id!=null&&id>=0)out.add(id);if(out.size()>=max)break;}return out;
    }
    private MusaAiDisciplineExpert(){}
}
