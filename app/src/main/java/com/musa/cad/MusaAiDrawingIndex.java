package com.musa.cad;

import java.text.Normalizer;
import java.util.*;

/** Immutable, lightweight drawing knowledge index used by MusaCAD AI. */
public final class MusaAiDrawingIndex {
    public static final class Item {
        public final int sourceId;
        public final String type,layer,text;
        public final double length,area;
        public Item(String type,String layer,String text){this(-1,type,layer,text,Double.NaN,Double.NaN);}
        public Item(int sourceId,String type,String layer,String text){this(sourceId,type,layer,text,Double.NaN,Double.NaN);}
        public Item(int sourceId,String type,String layer,String text,double length,double area){
            this.sourceId=sourceId;
            this.type=clean(type).toUpperCase(Locale.ROOT);
            this.layer=clean(layer);
            this.text=clean(text);
            this.length=Double.isFinite(length)&&length>=0d?length:Double.NaN;
            this.area=Double.isFinite(area)&&area>=0d?area:Double.NaN;
        }
        public boolean hasLength(){return Double.isFinite(length);}
        public boolean hasArea(){return Double.isFinite(area);}
    }

    public final String layout,unitName;
    public final int entityCount,oleObjectCount;
    public final Set<String> allLayers,visibleLayers;
    private final List<Item> items;
    private final Map<String,Integer> typeCounts;

    public MusaAiDrawingIndex(String layout,int entityCount,int oleObjectCount,
                              Collection<String>allLayers,Collection<String>visibleLayers,
                              Collection<Item>items){
        this(layout,entityCount,oleObjectCount,allLayers,visibleLayers,items,"");
    }

    public MusaAiDrawingIndex(String layout,int entityCount,int oleObjectCount,
                              Collection<String>allLayers,Collection<String>visibleLayers,
                              Collection<Item>items,String unitName){
        this.layout=clean(layout);
        this.unitName=clean(unitName);
        this.entityCount=Math.max(0,entityCount);
        this.oleObjectCount=Math.max(0,oleObjectCount);
        this.allLayers=immutableSet(allLayers);
        this.visibleLayers=immutableSet(visibleLayers);
        ArrayList<Item>copy=new ArrayList<>();
        LinkedHashMap<String,Integer>types=new LinkedHashMap<>();
        if(items!=null)for(Item item:items){
            if(item==null)continue;
            copy.add(item);
            if(!item.type.isEmpty())types.put(item.type,types.getOrDefault(item.type,0)+1);
        }
        this.items=Collections.unmodifiableList(copy);
        this.typeCounts=Collections.unmodifiableMap(types);
    }

    public List<Item>items(){return items;}
    public Map<String,Integer>typeCounts(){return typeCounts;}
    public int countType(String type){return typeCounts.getOrDefault(clean(type).toUpperCase(Locale.ROOT),0);}

    public int textEntityCount(){
        int n=0;
        for(Item item:items)if(!item.text.isEmpty())n++;
        return n;
    }

    public int textOccurrenceCount(String query){
        String wanted=normalize(query);if(wanted.isEmpty())return 0;
        int count=0;
        for(Item item:items){
            String hay=normalize(item.text);if(hay.isEmpty())continue;
            int at=0;
            while((at=hay.indexOf(wanted,at))>=0){count++;at+=Math.max(1,wanted.length());}
        }
        return count;
    }

    public List<String>textSamples(int limit){
        LinkedHashSet<String>out=new LinkedHashSet<>();
        for(Item item:items){
            if(item.text.isEmpty())continue;
            out.add(item.text);
            if(out.size()>=Math.max(1,limit))break;
        }
        return new ArrayList<>(out);
    }

    /** Layers whose name or visible text contains the requested phrase. */
    public List<String>layersMatching(String query){
        String wanted=normalize(query);if(wanted.isEmpty())return Collections.emptyList();
        LinkedHashSet<String>out=new LinkedHashSet<>();
        for(String layer:allLayers)if(normalize(layer).contains(wanted))out.add(layer);
        for(Item item:items)if(normalize(item.text).contains(wanted)&&!item.layer.isEmpty())out.add(item.layer);
        return new ArrayList<>(out);
    }

    public boolean containsPhrase(String query){
        String wanted=normalize(query);if(wanted.isEmpty())return false;
        for(String layer:allLayers)if(normalize(layer).contains(wanted))return true;
        for(Item item:items)if(normalize(item.text).contains(wanted))return true;
        return false;
    }

    static String normalize(String raw){
        if(raw==null)return "";
        String s=raw.trim().toLowerCase(new Locale("tr","TR"))
            .replace('ı','i').replace('ğ','g').replace('ü','u').replace('ş','s').replace('ö','o').replace('ç','c');
        s=Normalizer.normalize(s,Normalizer.Form.NFD).replaceAll("\\p{M}+","");
        return s.replaceAll("[^a-z0-9]+"," ").trim().replaceAll("\\s+"," ");
    }

    private static Set<String>immutableSet(Collection<String>source){
        TreeSet<String>out=new TreeSet<>(String.CASE_INSENSITIVE_ORDER);
        if(source!=null)for(String s:source)if(s!=null&&!s.trim().isEmpty())out.add(s.trim());
        return Collections.unmodifiableSet(out);
    }
    private static String clean(String s){return s==null?"":s.trim();}
    private MusaAiDrawingIndex(){this("",0,0,null,null,null);}
}
