package com.musa.cad;

import java.util.*;

/**
 * Mechanical expert profiles for MusaCAD AI.
 *
 * MEKAI_* commands run deterministic on-device review.
 * GMEKAI_* commands select the same expert profile for Gandalf Cloud AI.
 * Findings are review candidates from visible CAD metadata, never an approval
 * or a substitute for hydraulic/thermal/code calculations.
 */
public final class MusaAiMechanicalExpert {
    public enum Profile {
        FULL("mechanical_full","Tüm mekanik tesisat"),
        WASTE("waste","Pis su / atık su"),
        RAIN("rain","Yağmur suyu"),
        WATER("water","Temiz / sıcak-soğuk su"),
        HEATING("heating","Isıtma"),
        COOLING("cooling","Soğutma / VRF-VRV"),
        VENTILATION("ventilation","Havalandırma"),
        FIRE("fire","Yangın / sprinkler"),
        GAS("gas","Doğalgaz"),
        EQUIPMENT("equipment","Mekanik ekipman");

        public final String cloudKey,label;
        Profile(String cloudKey,String label){this.cloudKey=cloudKey;this.label=label;}
    }

    public static final class Result {
        public final boolean matched;
        public final Profile profile;
        public final String text;
        public final int findingCount;
        public final List<Integer> sourceIds;
        private Result(boolean matched,Profile profile,String text,int findingCount,Collection<Integer>sourceIds){
            this.matched=matched;this.profile=profile;this.text=text==null?"":text;
            this.findingCount=Math.max(0,findingCount);
            this.sourceIds=Collections.unmodifiableList(new ArrayList<>(sourceIds==null?Collections.emptyList():sourceIds));
        }
        public static Result none(){return new Result(false,null,"",0,Collections.emptyList());}
    }

    private static final class SystemStats {
        int items,linear,degenerate,openRuns;
        boolean diameter,slope,flow,size,capacity;
        final LinkedHashSet<Integer> ids=new LinkedHashSet<>();
        final StringBuilder corpus=new StringBuilder();
    }

    public static Result analyze(MusaAiDrawingIndex index,String raw){
        Profile profile=detect(raw);
        if(index==null||profile==null||!isLocalExpertCommand(raw))return Result.none();

        EnumMap<Profile,SystemStats>stats=scan(index);
        EnumSet<Profile> selected=selected(profile);
        LinkedHashSet<Integer> highlights=new LinkedHashSet<>();
        StringBuilder out=new StringBuilder();
        int findings=0;

        out.append("MEKAI Uzman Kontrol • ").append(profile.label)
           .append(" • ").append(index.layout.isEmpty()?"aktif layout":index.layout);

        out.append("\nSistem özeti");
        boolean any=false;
        for(Profile p:selected){
            SystemStats st=stats.get(p);
            if(st==null||st.items<=0)continue;
            any=true;
            out.append("\n• ").append(p.label).append(": ").append(st.items).append(" nesne");
            if(st.linear>0)out.append(" • ").append(st.linear).append(" çizgisel öğe");
        }
        if(!any)out.append("\n• Bu uzmanlık alanında görünür katman/metin eşleşmesi bulunamadı.");

        out.append("\nUzman inceleme adayları");
        for(Profile p:selected){
            SystemStats st=stats.get(p);
            if(st==null||st.items<=0)continue;

            if(st.degenerate>0){
                out.append("\n• ").append(p.label).append(": sıfır uzunluk/dejenere geometri ").append(st.degenerate);
                findings+=st.degenerate;highlights.addAll(st.ids);
            }
            if(st.openRuns>0&&isPipeProfile(p)){
                out.append("\n• ").append(p.label).append(": açık hat polyline ").append(st.openRuns)
                   .append(" — bağlantı sürekliliğini kontrol edin.");
                findings+=st.openRuns;highlights.addAll(st.ids);
            }
            if(isPipeProfile(p)&&st.linear>0&&!st.diameter){
                out.append("\n• ").append(p.label).append(": görünür çap / DN etiketi bulunamadı.");
                findings++;
            }

            findings+=appendProfileChecks(out,p,st);
        }

        if(findings==0)out.append("\n• Görünür CAD verisinden belirgin uzman kontrol adayı oluşmadı.");

        out.append("\nKontrol kapsamı");
        appendChecklist(out,profile);
        out.append("\nNot: MEKAI yalnız görünür CAD katmanları, metinleri ve sınırlı geometri metadata'sını tarar. ")
           .append("Debi, basınç kaybı, hidrolik hesap, ısı yükü, cihaz seçimi, yönetmelik uygunluğu ve saha koordinasyonu ayrıca doğrulanmalıdır.");

        return new Result(true,profile,out.toString(),findings,limitIds(highlights,300));
    }

    /** Returns a trusted server profile key, or empty when no mechanical expert profile was requested. */
    public static String cloudProfile(String raw){
        Profile p=detect(raw);return p==null?"":p.cloudKey;
    }

    public static boolean isCloudExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.equals("gmekai")||q.startsWith("gmekai ")||
            q.equals("gandalf mekai")||q.startsWith("gandalf mekai ");
    }

    public static boolean isLocalExpertCommand(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        return (q.equals("mekai")||q.startsWith("mekai "))&&!q.startsWith("mekai cloud ");
    }

    public static Profile detect(String raw){
        String q=MusaAiDrawingIndex.normalize(raw);
        if(q.isEmpty())return null;
        boolean explicit=q.equals("mekai")||q.startsWith("mekai ")||q.equals("gmekai")||q.startsWith("gmekai ")||
            q.equals("gandalf mekai")||q.startsWith("gandalf mekai ");
        boolean expertNatural=q.contains("mekanik uzman")||q.contains("mekanik tesisat uzman");
        if(!explicit&&!expertNatural)return null;

        if(has(q,"pis su","atik su","waste","sewer","drenaj","drain","pissu"))return Profile.WASTE;
        if(has(q,"yagmur","rain","storm"))return Profile.RAIN;
        if(has(q,"temiz su","sicak su","soguk su","kullanma suyu","water","temizsu"))return Profile.WATER;
        if(has(q,"isitma","heating","kalorifer","yerden isitma"))return Profile.HEATING;
        if(has(q,"vrf","vrv","sogutma","cooling","chiller","fancoil","fan coil","klima"))return Profile.COOLING;
        if(has(q,"havalandirma","vent","ventilasyon","duct","kanal","ahu","egzoz","taze hava"))return Profile.VENTILATION;
        if(has(q,"yangin","fire","sprinkler","hidrant","jokey","jockey"))return Profile.FIRE;
        if(has(q,"dogalgaz","natural gas","gaz tesisati"))return Profile.GAS;
        if(has(q,"ekipman","equipment","pompa","hidrofor","boyler","kazan","esanj"))return Profile.EQUIPMENT;
        return Profile.FULL;
    }

    private static EnumMap<Profile,SystemStats>scan(MusaAiDrawingIndex index){
        EnumMap<Profile,SystemStats>map=new EnumMap<>(Profile.class);
        for(Profile p:Profile.values())if(p!=Profile.FULL)map.put(p,new SystemStats());
        for(MusaAiDrawingIndex.Item item:index.items()){
            if(item==null)continue;
            String hay=MusaAiDrawingIndex.normalize(item.layer+" "+item.text);
            EnumSet<Profile>types=classify(hay);
            for(Profile p:types){
                SystemStats st=map.get(p);if(st==null)continue;
                st.items++;
                if(item.hasLength()){
                    st.linear++;
                    if(item.length<=1e-9d)st.degenerate++;
                }
                if(isPolyline(item.type)&&item.closedKnown&&!item.closed)st.openRuns++;
                st.diameter|=diameter(item.layer)||diameter(item.text);
                st.slope|=slope(item.layer)||slope(item.text);
                st.flow|=flow(item.layer)||flow(item.text);
                st.size|=dimension(item.layer)||dimension(item.text);
                st.capacity|=capacity(item.layer)||capacity(item.text);
                if(item.sourceId>=0)st.ids.add(item.sourceId);
                if(!item.text.isEmpty())st.corpus.append(' ').append(MusaAiDrawingIndex.normalize(item.text));
                if(!item.layer.isEmpty())st.corpus.append(' ').append(MusaAiDrawingIndex.normalize(item.layer));
            }
        }
        return map;
    }

    private static int appendProfileChecks(StringBuilder out,Profile p,SystemStats st){
        String c=st.corpus.toString();int n=0;
        switch(p){
            case WASTE:
                if(st.linear>0&&!st.slope){out.append("\n• Pis su: görünür eğim / % etiketi bulunamadı.");n++;}
                if(!has(c,"temizleme","cleanout","rogar","rögar","kontrol bacasi","inspection")){
                    out.append("\n• Pis su: temizleme / rögar / kontrol bacası referansı görünmüyor; ilgili paftayı kontrol edin.");n++;
                }
                if(!has(c,"havalik","vent","kolon havalik")){
                    out.append("\n• Pis su: havalık/vent referansı görünmüyor; sistem tipine göre kontrol edin.");n++;
                }
                break;
            case RAIN:
                if(!has(c,"suzgec","süzgeç","roof drain","yagmur suyu agzi","oluk","gutter")){
                    out.append("\n• Yağmur suyu: süzgeç/oluk/çatı drenaj elemanı etiketi bulunamadı.");n++;
                }
                if(!has(c,"tasirma","taşırma","overflow","acil drenaj","emergency drain")){
                    out.append("\n• Yağmur suyu: taşırma/acil drenaj referansı görünmüyor; gerekiyorsa ilgili detayı kontrol edin.");n++;
                }
                break;
            case WATER:
                if(!has(c,"vana","valve")){out.append("\n• Temiz su: vana/kesme elemanı etiketi görünmüyor.");n++;}
                if(!has(c,"hidrofor","pump","pompa","depo","tank","sayac","meter","basinc dusurucu","prv")){
                    out.append("\n• Temiz su: besleme/basınçlandırma/sayaç ekipmanı referansı görünmüyor; ilgili paftayı kontrol edin.");n++;
                }
                break;
            case HEATING:
                if(!has(c,"gidis","donus","dönüş","supply","return")){out.append("\n• Isıtma: gidiş/dönüş ayrımı görünür metinlerde bulunamadı.");n++;}
                if(!has(c,"balans","balancing","vana","valve")){out.append("\n• Isıtma: balans/kontrol vanası referansı görünmüyor.");n++;}
                if(!has(c,"kazan","boiler","esanj","eşanj","pompa","pump","kollektor","kollektör")){
                    out.append("\n• Isıtma: ısı üretim/dağıtım ekipmanı etiketi görünmüyor.");n++;
                }
                break;
            case COOLING:
                if(!has(c,"vrf","vrv","chiller","fancoil","fan coil","dis unite","dış ünite","ic unite","iç ünite")){
                    out.append("\n• Soğutma: ana sistem/cihaz tipi etiketi görünmüyor.");n++;
                }
                if(has(c,"vrf","vrv","klima")&&!has(c,"drenaj","drain","kondens","condensate")){
                    out.append("\n• VRF/klima: kondens drenaj referansı görünmüyor.");n++;
                }
                if(has(c,"vrf","vrv")&&!has(c,"refnet","branch","joint","dagitim kutusu","dağıtım kutusu")){
                    out.append("\n• VRF/VRV: branşman/refnet/dağıtım elemanı referansı görünmüyor.");n++;
                }
                break;
            case VENTILATION:
                if(!st.size){out.append("\n• Havalandırma: görünür kanal ölçüsü (örn. 600x300) bulunamadı.");n++;}
                if(!st.flow){out.append("\n• Havalandırma: görünür debi etiketi (m³/h, m3/h vb.) bulunamadı.");n++;}
                if(!has(c,"menfez","difuzor","diffuser","grille")){out.append("\n• Havalandırma: menfez/difüzör etiketi görünmüyor.");n++;}
                if(!has(c,"fan","ahu","santral","egzoz","exhaust","taze hava","fresh air")){
                    out.append("\n• Havalandırma: fan/santral/egzoz/taze hava ekipman referansı görünmüyor.");n++;
                }
                if(!has(c,"damper")){out.append("\n• Havalandırma: damper etiketi görünmüyor; zonlama/yangın geçişlerinde kontrol edin.");n++;}
                break;
            case FIRE:
                if(!has(c,"sprinkler","hidrant","yangin dolabi","yangın dolabı","fire cabinet")){
                    out.append("\n• Yangın: sprinkler/hidrant/yangın dolabı terminal etiketi görünmüyor.");n++;
                }
                if(!has(c,"pompa","pump","jokey","jockey","depo","tank")){
                    out.append("\n• Yangın: pompa/jokey/depo referansı görünmüyor; pompa dairesi paftasını kontrol edin.");n++;
                }
                if(!has(c,"itfaiye","fire department","fdc","siamese")){
                    out.append("\n• Yangın: itfaiye bağlantı ağzı/FDC referansı görünmüyor.");n++;
                }
                if(!has(c,"alarm vana","alarm valve","zone valve","test drenaj","test drain")){
                    out.append("\n• Yangın: zon/alarm vana/test-drenaj referansı görünmüyor; sistem tipine göre kontrol edin.");n++;
                }
                break;
            case GAS:
                if(!has(c,"vana","valve")){out.append("\n• Doğalgaz: kesme vanası etiketi görünmüyor.");n++;}
                if(!has(c,"sayac","meter","regulator","regülatör")){out.append("\n• Doğalgaz: sayaç/regülatör referansı görünmüyor.");n++;}
                if(!has(c,"solenoid","gaz alarm","gas detector","dedektor","dedektör")){
                    out.append("\n• Doğalgaz: solenoid/dedektör referansı görünmüyor; kullanım türüne göre kontrol edin.");n++;
                }
                break;
            case EQUIPMENT:
                if(!st.capacity){out.append("\n• Mekanik ekipman: görünür kapasite/debi/basınç/güç etiketi bulunamadı.");n++;}
                if(!has(c,"pompa","pump","hidrofor","kazan","boiler","chiller","ahu","santral","boyler","tank","depo","esanj","eşanj")){
                    out.append("\n• Mekanik ekipman: tanımlanabilir ekipman etiketi görünmüyor.");n++;
                }
                break;
            default:break;
        }
        return n;
    }

    private static void appendChecklist(StringBuilder out,Profile p){
        switch(p){
            case WASTE:out.append("\n• Çaplar • eğimler • havalık • temizleme/rögar • bağlantı sürekliliği");break;
            case RAIN:out.append("\n• Çatı süzgeçleri/oluklar • düşeyler • çaplar • taşırma/acil drenaj");break;
            case WATER:out.append("\n• Soğuk/sıcak/return ayrımı • çaplar • vanalar • sayaç/PRV • depo/hidrofor");break;
            case HEATING:out.append("\n• Gidiş-dönüş • çaplar • balans/kontrol • pompa/kollektör • ısı üretimi");break;
            case COOLING:out.append("\n• VRF/VRV/chiller/fancoil • soğutucu hatlar • branşman • kondens drenaj • kapasite");break;
            case VENTILATION:out.append("\n• Kanal ölçüleri • debiler • taze/egzoz/üfleme/emme • menfezler • fan/AHU • damperler");break;
            case FIRE:out.append("\n• Sprinkler/hidrant/dolap • çaplar • zon/alarm vana • pompa/jokey/depo • itfaiye bağlantısı");break;
            case GAS:out.append("\n• Çaplar • sayaç/regülatör • kesme/solenoid • dedektör • cihaz bağlantıları");break;
            case EQUIPMENT:out.append("\n• Ekipman etiketi • kapasite • debi/basınç/güç • bağlantı ve yedeklilik referansları");break;
            default:
                out.append("\n• Pis su • yağmur suyu • temiz su • ısıtma • soğutma/VRF • havalandırma • yangın • doğalgaz • ekipman");
        }
    }

    private static EnumSet<Profile>selected(Profile p){
        if(p!=Profile.FULL)return EnumSet.of(p);
        return EnumSet.of(Profile.WASTE,Profile.RAIN,Profile.WATER,Profile.HEATING,Profile.COOLING,Profile.VENTILATION,Profile.FIRE,Profile.GAS,Profile.EQUIPMENT);
    }

    private static EnumSet<Profile>classify(String hay){
        EnumSet<Profile>out=EnumSet.noneOf(Profile.class);
        if(has(hay,"pis su","atik su","kanalizasyon","waste","sewer","foul","soil","drenaj","drain"))out.add(Profile.WASTE);
        if(has(hay,"yagmur","rain","storm","roof drain","oluk"))out.add(Profile.RAIN);
        if(has(hay,"temiz su","kullanma suyu","sicak su","soguk su","potable","domestic water","cold water","hot water","hidrofor"))out.add(Profile.WATER);
        if(has(hay,"isitma","kalorifer","radyator","yerden isitma","heating","kazan","esanj","kollektor","kollektör"))out.add(Profile.HEATING);
        if(has(hay,"sogutma","vrf","vrv","chiller","fancoil","fan coil","klima","cooling","refrigerant","refnet"))out.add(Profile.COOLING);
        if(has(hay,"havalandirma","ventilasyon","duct","menfez","difuzor","diffuser","damper","ahu","santral","egzoz","exhaust","taze hava","fresh air"))out.add(Profile.VENTILATION);
        if(has(hay,"yangin","sprinkler","hidrant","fire","jokey","jockey","itfaiye","alarm vana"))out.add(Profile.FIRE);
        if(has(hay,"dogalgaz","natural gas","gaz tesisati","gas line","regulator","solenoid"))out.add(Profile.GAS);
        if(has(hay,"pompa","pump","hidrofor","depo","tank","boyler","boiler","esanj","kazan","chiller","ahu","santral","ekipman"))out.add(Profile.EQUIPMENT);
        return out;
    }

    private static boolean isPipeProfile(Profile p){
        return p==Profile.WASTE||p==Profile.RAIN||p==Profile.WATER||p==Profile.HEATING||
            p==Profile.COOLING||p==Profile.FIRE||p==Profile.GAS;
    }

    private static boolean isPolyline(String type){return "POLYLINE".equals(type)||"LWPOLYLINE".equals(type);}

    private static boolean diameter(String raw){
        if(raw==null||raw.trim().isEmpty())return false;
        String upper=raw.toUpperCase(Locale.ROOT).replace('Ø','D');
        return upper.matches(".*\\bDN\\s*[-:]?\\s*\\d+.*")||
            upper.matches(".*\\bD\\s*[-:]?\\s*\\d+.*")||
            upper.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*MM\\b.*")||
            has(MusaAiDrawingIndex.normalize(raw),"cap ","diameter ","diam ");
    }

    private static boolean slope(String raw){
        if(raw==null)return false;
        return raw.contains("%")||raw.contains("‰")||has(MusaAiDrawingIndex.normalize(raw),"egim","slope","meyil");
    }

    private static boolean flow(String raw){
        if(raw==null)return false;
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*m3\\s*h\\b.*")||
            q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*cfm\\b.*")||
            has(q,"debi","airflow","flow rate");
    }

    private static boolean dimension(String raw){
        if(raw==null)return false;
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d{2,4}\\s*x\\s*\\d{2,4}\\b.*")||has(q,"kanal olcusu","duct size");
    }

    private static boolean capacity(String raw){
        if(raw==null)return false;
        String q=MusaAiDrawingIndex.normalize(raw);
        return q.matches(".*\\b\\d+(?:[.,]\\d+)?\\s*(kw|w|hp|pa|kpa|bar|m3 h|kcal h|btu h)\\b.*")||
            has(q,"kapasite","capacity","debi","basinc","pressure","guc","power");
    }

    private static boolean has(String q,String...terms){
        if(q==null)return false;
        for(String t:terms)if(q.contains(MusaAiDrawingIndex.normalize(t)))return true;
        return false;
    }

    private static Collection<Integer>limitIds(Collection<Integer>ids,int max){
        ArrayList<Integer>out=new ArrayList<>();if(ids==null)return out;
        for(Integer id:ids){if(id!=null&&id>=0)out.add(id);if(out.size()>=max)break;}
        return out;
    }

    private MusaAiMechanicalExpert(){}
}
