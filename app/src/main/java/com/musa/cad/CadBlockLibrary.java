package com.musa.cad;

import java.util.*;

/** Built-in, searchable CAD symbol library used by the INSERT workflow. */
public final class CadBlockLibrary {
    public static final String CATEGORY_ARCHITECTURE="Mimari";
    public static final String CATEGORY_MECHANICAL="Mekanik";
    public static final String CATEGORY_ELECTRICAL="Elektrik";
    public static final String CATEGORY_PROJECT="Proje";

    private interface Factory { List<CadEdit> create(Map<String,String> attributes); }

    public static final class Entry {
        public final String id;
        public final String name;
        public final String category;
        public final String keywords;
        public final Map<String,String> attributeDefaults;
        private final Factory factory;

        private Entry(String id,String name,String category,String keywords,Map<String,String> defaults,Factory factory){
            this.id=id;
            this.name=name;
            this.category=category;
            this.keywords=keywords==null?"":keywords;
            LinkedHashMap<String,String> copy=new LinkedHashMap<>();
            if(defaults!=null)copy.putAll(defaults);
            this.attributeDefaults=Collections.unmodifiableMap(copy);
            this.factory=factory;
        }

        public boolean matches(String query){
            String q=normalize(query);
            if(q.isEmpty())return true;
            String hay=normalize(name+" "+category+" "+keywords+" "+id);
            for(String token:q.split("\\s+"))if(!token.isEmpty()&&!hay.contains(token))return false;
            return true;
        }

        public CadBlock.Definition definition(Map<String,String> attributes){
            LinkedHashMap<String,String> effective=new LinkedHashMap<>(attributeDefaults);
            if(attributes!=null)for(Map.Entry<String,String> item:attributes.entrySet()){
                if(item.getKey()==null)continue;
                String key=item.getKey().trim();
                if(key.isEmpty())continue;
                String value=item.getValue()==null?"":item.getValue().trim();
                effective.put(key,value);
            }
            List<CadEdit> members=factory.create(Collections.unmodifiableMap(effective));
            return new CadBlock.Definition(definitionName(effective),members);
        }

        public String definitionName(Map<String,String> attributes){
            String base="LIB_"+CadBlock.normalizeName(id).toUpperCase(Locale.ROOT);
            if(attributeDefaults.isEmpty())return base;
            TreeMap<String,String> sorted=new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
            sorted.putAll(attributeDefaults);
            if(attributes!=null)sorted.putAll(attributes);
            StringBuilder canonical=new StringBuilder();
            for(Map.Entry<String,String> item:sorted.entrySet())canonical.append(item.getKey()).append('=').append(item.getValue()).append(';');
            return base+"_"+Integer.toHexString(canonical.toString().hashCode()).toUpperCase(Locale.ROOT);
        }

        public String attributeValue(Map<String,String> attributes,String key){
            String value=attributes==null?null:attributes.get(key);
            if(value==null)value=attributeDefaults.get(key);
            return value==null?"":value;
        }
    }

    private static final List<Entry> ENTRIES=build();

    private CadBlockLibrary(){}

    public static List<Entry> all(){return ENTRIES;}

    public static Entry findById(String id){
        if(id==null)return null;
        for(Entry entry:ENTRIES)if(entry.id.equalsIgnoreCase(id.trim()))return entry;
        return null;
    }

    public static List<Entry> filter(String category,String query){
        ArrayList<Entry> out=new ArrayList<>();
        for(Entry entry:ENTRIES){
            if(category!=null&&!category.trim().isEmpty()&&!entry.category.equalsIgnoreCase(category.trim()))continue;
            if(entry.matches(query))out.add(entry);
        }
        return out;
    }

    public static List<String> categories(){
        return Arrays.asList(CATEGORY_ARCHITECTURE,CATEGORY_MECHANICAL,CATEGORY_ELECTRICAL,CATEGORY_PROJECT);
    }

    private static List<Entry> build(){
        ArrayList<Entry> out=new ArrayList<>();
        out.add(entry("arch_door_single","Tek Kanat Kapı",CATEGORY_ARCHITECTURE,"kapı door mimari",a->doorSingle()));
        out.add(entry("arch_window","Pencere",CATEGORY_ARCHITECTURE,"pencere window doğrama",a->window()));
        out.add(entry("arch_wc","WC Klozet",CATEGORY_ARCHITECTURE,"wc klozet tuvalet sanitary",a->wc()));
        out.add(entry("arch_sink","Lavabo",CATEGORY_ARCHITECTURE,"lavabo eviye sink sanitary",a->sink()));
        out.add(entry("arch_room_tag","Mahal Etiketi",CATEGORY_ARCHITECTURE,"mahal oda room tag etiket",defaults("MAHAL","101"),a->roomTag(value(a,"MAHAL","101"))));

        out.add(entry("mec_pump","Pompa",CATEGORY_MECHANICAL,"pompa pump sirkülasyon hidrofor",a->pump()));
        out.add(entry("mec_valve","Vana",CATEGORY_MECHANICAL,"vana valve mekanik tesisat",a->valve()));
        out.add(entry("mec_fan","Fan",CATEGORY_MECHANICAL,"fan havalandırma ventilation",a->fan()));
        out.add(entry("mec_radiator","Radyatör",CATEGORY_MECHANICAL,"radyatör radiator ısıtma heating",a->radiator()));
        out.add(entry("mec_sprinkler","Sprinkler",CATEGORY_MECHANICAL,"sprinkler yangın fire söndürme",a->sprinkler()));
        out.add(entry("mec_diffuser","Difüzör",CATEGORY_MECHANICAL,"difüzör diffuser menfez havalandırma supply air",a->diffuser()));
        out.add(entry("mec_grille","Menfez",CATEGORY_MECHANICAL,"menfez grille return exhaust havalandırma",a->grille()));
        out.add(entry("mec_fire_cabinet","Yangın Dolabı",CATEGORY_MECHANICAL,"yangın dolabı fire cabinet hose reel",a->fireCabinet()));
        out.add(entry("mec_equipment_tag","Mekanik Ekipman Etiketi",CATEGORY_MECHANICAL,"ekipman tag mekanik cihaz",defaults("TAG","AHU-01"),a->equipmentTag(value(a,"TAG","AHU-01"))));

        out.add(entry("elec_socket","Priz",CATEGORY_ELECTRICAL,"priz socket outlet elektrik",a->socket()));
        out.add(entry("elec_switch","Anahtar",CATEGORY_ELECTRICAL,"anahtar switch elektrik",a->switchSymbol()));
        out.add(entry("elec_light","Armatür",CATEGORY_ELECTRICAL,"armatür lamba light luminaire elektrik",a->luminaire()));
        out.add(entry("elec_panel","Elektrik Panosu",CATEGORY_ELECTRICAL,"pano panel elektrik",defaults("PANO","P-01"),a->panel(value(a,"PANO","P-01"))));
        out.add(entry("elec_ground","Topraklama",CATEGORY_ELECTRICAL,"topraklama earth ground elektrik",a->ground()));

        out.add(entry("proj_north","Kuzey Oku",CATEGORY_PROJECT,"kuzey north yön oku proje",a->northArrow()));
        out.add(entry("proj_section","Kesit İşareti",CATEGORY_PROJECT,"kesit section marker proje",defaults("KESİT","A"),a->sectionMarker(value(a,"KESİT","A"))));
        out.add(entry("proj_elevation","Kot İşareti",CATEGORY_PROJECT,"kot elevation level proje",defaults("KOT","+0.00"),a->elevationMarker(value(a,"KOT","+0.00"))));
        out.add(entry("proj_grid","Aks Balonu",CATEGORY_PROJECT,"aks grid bubble proje",defaults("AKS","A"),a->gridBubble(value(a,"AKS","A"))));
        return Collections.unmodifiableList(out);
    }

    private static Entry entry(String id,String name,String category,String keywords,Factory factory){
        return new Entry(id,name,category,keywords,Collections.emptyMap(),factory);
    }

    private static Entry entry(String id,String name,String category,String keywords,Map<String,String> defaults,Factory factory){
        return new Entry(id,name,category,keywords,defaults,factory);
    }

    private static Map<String,String> defaults(String key,String value){
        LinkedHashMap<String,String> map=new LinkedHashMap<>();map.put(key,value);return map;
    }

    private static String value(Map<String,String> map,String key,String fallback){
        String value=map==null?null:map.get(key);return value==null||value.trim().isEmpty()?fallback:value.trim();
    }

    private static String normalize(String value){
        if(value==null)return "";
        return value.toLowerCase(new Locale("tr","TR"))
            .replace('ı','i').replace('ğ','g').replace('ü','u').replace('ş','s').replace('ö','o').replace('ç','c')
            .replaceAll("[^a-z0-9+._-]+"," ").trim();
    }

    private static List<CadEdit> list(CadEdit...items){
        ArrayList<CadEdit> out=new ArrayList<>();for(CadEdit item:items)if(item!=null)out.add(item);return out;
    }

    private static List<CadEdit> doorSingle(){
        return list(CadEdit.line(0,0,60,0),CadEdit.arc(60,0,42.4f,42.4f,0,60),CadEdit.line(0,0,0,60));
    }

    private static List<CadEdit> window(){
        return list(CadEdit.rectangle(-60,-10,60,10),CadEdit.line(-60,0,60,0),CadEdit.line(0,-10,0,10));
    }

    private static List<CadEdit> wc(){
        return list(CadEdit.ellipse(0,8,22,8,0,38),CadEdit.rectangle(-18,-42,18,-18),CadEdit.line(-15,-18,-10,-2),CadEdit.line(15,-18,10,-2));
    }

    private static List<CadEdit> sink(){
        return list(CadEdit.rectangle(-42,-28,42,28),CadEdit.ellipse(0,0,30,0,0,18),CadEdit.circle(0,-10,3,-10));
    }

    private static List<CadEdit> roomTag(String text){
        return list(CadEdit.rectangle(-52,-26,52,26),CadEdit.styledText(-38,7,text,0,"STANDARD","sans-serif",false,18,1,0,0));
    }

    private static List<CadEdit> pump(){
        return list(CadEdit.circle(0,0,34,0),CadEdit.polyline(new float[]{-18,-20,22,0,-18,20},true),CadEdit.line(-55,0,-34,0),CadEdit.line(34,0,55,0));
    }

    private static List<CadEdit> valve(){
        return list(CadEdit.polyline(new float[]{-36,-24,0,0,-36,24},true),CadEdit.polyline(new float[]{36,-24,0,0,36,24},true),CadEdit.line(-55,0,-36,0),CadEdit.line(36,0,55,0));
    }

    private static List<CadEdit> fan(){
        return list(CadEdit.circle(0,0,38,0),CadEdit.line(-26,-26,26,26),CadEdit.line(-26,26,26,-26),CadEdit.circle(0,0,7,0));
    }

    private static List<CadEdit> radiator(){
        ArrayList<CadEdit> out=new ArrayList<>();out.add(CadEdit.rectangle(-60,-22,60,22));
        for(int x=-45;x<=45;x+=15)out.add(CadEdit.line(x,-18,x,18));
        out.add(CadEdit.line(-75,0,-60,0));out.add(CadEdit.line(60,0,75,0));return out;
    }

    private static List<CadEdit> sprinkler(){
        ArrayList<CadEdit> out=new ArrayList<>();out.add(CadEdit.circle(0,0,9,0));
        for(int i=0;i<8;i++){double r=Math.toRadians(i*45d);out.add(CadEdit.line((float)(Math.cos(r)*12),(float)(Math.sin(r)*12),(float)(Math.cos(r)*34),(float)(Math.sin(r)*34)));}
        return out;
    }

    private static List<CadEdit> diffuser(){
        return list(
            CadEdit.rectangle(-32,-32,32,32),
            CadEdit.line(-32,-32,32,32),
            CadEdit.line(-32,32,32,-32),
            CadEdit.line(-32,0,32,0),
            CadEdit.line(0,-32,0,32)
        );
    }

    private static List<CadEdit> grille(){
        ArrayList<CadEdit> out=new ArrayList<>();out.add(CadEdit.rectangle(-46,-24,46,24));
        for(int x=-30;x<=30;x+=15)out.add(CadEdit.line(x,-20,x,20));
        return out;
    }

    private static List<CadEdit> fireCabinet(){
        return list(
            CadEdit.rectangle(-34,-44,34,44),
            CadEdit.circle(0,4,22,4),
            CadEdit.line(-18,4,18,4),
            CadEdit.line(0,-14,0,22),
            CadEdit.styledText(-18,-28,"YD",0,"STANDARD","sans-serif",false,14,1,0,0)
        );
    }

    private static List<CadEdit> equipmentTag(String tag){
        return list(CadEdit.rectangle(-58,-24,58,24),CadEdit.styledText(-46,7,tag,0,"STANDARD","sans-serif",false,18,1,0,0));
    }

    private static List<CadEdit> socket(){
        return list(CadEdit.circle(0,0,28,0),CadEdit.line(-10,-14,-10,12),CadEdit.line(10,-14,10,12),CadEdit.line(-18,17,18,17));
    }

    private static List<CadEdit> switchSymbol(){
        return list(CadEdit.circle(0,0,26,0),CadEdit.styledText(-7,8,"S",0,"STANDARD","sans-serif",false,22,1,0,0),CadEdit.line(26,0,50,0));
    }

    private static List<CadEdit> luminaire(){
        return list(CadEdit.circle(0,0,34,0),CadEdit.line(-24,-24,24,24),CadEdit.line(-24,24,24,-24));
    }

    private static List<CadEdit> panel(String label){
        return list(CadEdit.rectangle(-46,-34,46,34),CadEdit.line(-46,0,46,0),CadEdit.styledText(-34,-8,label,0,"STANDARD","sans-serif",false,16,1,0,0));
    }

    private static List<CadEdit> ground(){
        return list(CadEdit.line(0,-44,0,0),CadEdit.line(-32,0,32,0),CadEdit.line(-23,10,23,10),CadEdit.line(-13,20,13,20));
    }

    private static List<CadEdit> northArrow(){
        return list(CadEdit.circle(0,0,42,0),CadEdit.polyline(new float[]{0,-34,-13,12,0,4,13,12},true),CadEdit.styledText(-8,66,"N",0,"STANDARD","sans-serif",false,22,1,0,0));
    }

    private static List<CadEdit> sectionMarker(String label){
        return list(CadEdit.line(-70,0,70,0),CadEdit.circle(-52,0,-34,0),CadEdit.circle(52,0,70,0),CadEdit.styledText(-58,7,label,0,"STANDARD","sans-serif",false,16,1,0,0),CadEdit.styledText(46,7,label,0,"STANDARD","sans-serif",false,16,1,0,0));
    }

    private static List<CadEdit> elevationMarker(String label){
        return list(CadEdit.polyline(new float[]{-18,0,0,-18,18,0},true),CadEdit.line(-42,0,42,0),CadEdit.styledText(24,-7,label,0,"STANDARD","sans-serif",false,15,1,0,0));
    }

    private static List<CadEdit> gridBubble(String label){
        return list(CadEdit.circle(0,0,28,0),CadEdit.styledText(-8,7,label,0,"STANDARD","sans-serif",false,18,1,0,0),CadEdit.line(0,28,0,68));
    }
}
