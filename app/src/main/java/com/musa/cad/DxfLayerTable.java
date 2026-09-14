package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Reads the authoritative DXF LAYER table, including visibility state and color. */
public final class DxfLayerTable {
    public static final class Table {
        public final Map<String,Integer> colors=new HashMap<>();
        public final Set<String> names=new LinkedHashSet<>();
        public final Set<String> visible=new LinkedHashSet<>();
        public final Set<String> off=new LinkedHashSet<>();
        public final Set<String> frozen=new LinkedHashSet<>();
        public final Set<String> locked=new LinkedHashSet<>();
        private final Map<String,String> namesByKey=new HashMap<>();

        public boolean contains(String name){return namesByKey.containsKey(DxfColor.key(name));}

        public void add(String rawName,int aci,int flags,int trueColor){
            String name=(rawName==null||rawName.trim().isEmpty())?"0":rawName.trim();
            String key=DxfColor.key(name);
            String canonical=namesByKey.get(key);
            if(canonical==null){canonical=name;namesByKey.put(key,canonical);names.add(canonical);}
            colors.put(key,DxfColor.layerArgb(aci,trueColor));
            visible.remove(canonical);off.remove(canonical);frozen.remove(canonical);locked.remove(canonical);
            boolean isOff=aci<0;
            boolean isFrozen=(flags&1)!=0;
            boolean isLocked=(flags&4)!=0;
            if(isOff)off.add(canonical);
            if(isFrozen)frozen.add(canonical);
            if(isLocked)locked.add(canonical);
            if(!isOff&&!isFrozen)visible.add(canonical);
        }

        public void ensureDefaultLayer(){
            if(names.isEmpty())add("0",7,0,DxfColor.NO_TRUE_COLOR);
        }
    }

    public static Table parse(List<String> tags)throws IOException{
        Table table=new Table();String section="";
        try{
            if(tags.size()%2!=0)throw new NumberFormatException();
            for(int i=0;i<tags.size();){
                int code=Integer.parseInt(tags.get(i).trim());
                if(code!=0){i+=2;continue;}
                String type=tags.get(i+1).trim();int from=i+2;i=from;
                while(i<tags.size()&&Integer.parseInt(tags.get(i).trim())!=0)i+=2;
                if("SECTION".equals(type)){section=text(tags,from,i,2,"").trim();continue;}
                if("ENDSEC".equals(type)){section="";continue;}
                if(!"TABLES".equals(section)||!"LAYER".equals(type))continue;
                String name=text(tags,from,i,2,"0");
                int aci=integer(tags,from,i,62,7);
                int flags=integer(tags,from,i,70,0);
                int trueColor=DxfColor.NO_TRUE_COLOR;
                String raw=text(tags,from,i,420,"").trim();
                if(!raw.isEmpty())trueColor=DxfColor.trueColor(Long.parseLong(raw));
                table.add(name,aci,flags,trueColor);
            }
        }catch(NumberFormatException e){throw new IOException("Geçersiz DXF LAYER tablosu",e);}
        table.ensureDefaultLayer();return table;
    }

    private static String text(List<String> tags,int from,int to,int wanted,String fallback){
        for(int i=from;i+1<to;i+=2)if(Integer.parseInt(tags.get(i).trim())==wanted)return tags.get(i+1);
        return fallback;
    }
    private static int integer(List<String> tags,int from,int to,int wanted,int fallback){
        String value=text(tags,from,to,wanted,Integer.toString(fallback)).trim();
        return Integer.parseInt(value);
    }
    private DxfLayerTable(){}
}
