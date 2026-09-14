package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Reads the authoritative DXF LAYER table, including visibility, color and display style. */
public final class DxfLayerTable {
    public static final class Table {
        public final Map<String,Integer> colors=new HashMap<>();
        public final Map<String,String> lineTypes=new HashMap<>();
        public final Map<String,Integer> lineWeights=new HashMap<>();
        public final Map<String,Integer> opacities=new HashMap<>();
        public final Map<String,String> namesByHandle=new HashMap<>();
        public final Set<String> names=new LinkedHashSet<>();
        public final Set<String> visible=new LinkedHashSet<>();
        public final Set<String> off=new LinkedHashSet<>();
        public final Set<String> frozen=new LinkedHashSet<>();
        public final Set<String> locked=new LinkedHashSet<>();
        private final Map<String,String> namesByKey=new HashMap<>();

        public boolean contains(String name){return namesByKey.containsKey(DxfColor.key(name));}
        public String nameForHandle(String handle){
            if(handle==null)return null;return namesByHandle.get(handle.trim().toUpperCase(Locale.ROOT));
        }

        public void add(String rawName,int aci,int flags,int trueColor){
            add(rawName,aci,flags,trueColor,DxfStyle.CONTINUOUS,DxfStyle.LW_DEFAULT,DxfTransparency.UNSET,"");
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight){
            add(rawName,aci,flags,trueColor,lineType,lineWeight,DxfTransparency.UNSET,"");
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight,long transparency){
            add(rawName,aci,flags,trueColor,lineType,lineWeight,transparency,"");
        }

        public void add(String rawName,int aci,int flags,int trueColor,String lineType,int lineWeight,long transparency,String handle){
            String name=(rawName==null||rawName.trim().isEmpty())?"0":rawName.trim();
            String key=DxfColor.key(name);
            String canonical=namesByKey.get(key);
            if(canonical==null){canonical=name;namesByKey.put(key,canonical);names.add(canonical);}
            colors.put(key,DxfColor.layerArgb(aci,trueColor));
            lineTypes.put(key,DxfStyle.normalizeLineType(lineType));
            lineWeights.put(key,lineWeight);
            opacities.put(key,DxfTransparency.layerOpacity(transparency));
            String h=handle==null?"":handle.trim();if(!h.isEmpty())namesByHandle.put(h.toUpperCase(Locale.ROOT),canonical);
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
            if(names.isEmpty())add("0",7,0,DxfColor.NO_TRUE_COLOR,DxfStyle.CONTINUOUS,DxfStyle.LW_DEFAULT);
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
                String lineType=text(tags,from,i,6,DxfStyle.CONTINUOUS);
                int lineWeight=integer(tags,from,i,370,DxfStyle.LW_DEFAULT);
                long transparency=longInteger(tags,from,i,440,DxfTransparency.UNSET);
                String handle=text(tags,from,i,5,"");
                table.add(name,aci,flags,trueColor,lineType,lineWeight,transparency,handle);
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
    private static long longInteger(List<String> tags,int from,int to,int wanted,long fallback){
        String value=text(tags,from,to,wanted,Long.toString(fallback)).trim();
        return Long.parseLong(value);
    }
    private DxfLayerTable(){}
}
