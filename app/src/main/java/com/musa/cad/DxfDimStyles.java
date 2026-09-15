package com.musa.cad;

import java.io.IOException;
import java.util.*;

/** Minimal DIMSTYLE reader used for LEADER display fidelity. */
public final class DxfDimStyles {
    public static final class Entry {
        public final String name;public final double dimScale,arrowSize;
        Entry(String name,double dimScale,double arrowSize){this.name=name;this.dimScale=dimScale;this.arrowSize=arrowSize;}
        public double effectiveArrowSize(){
            double s=Double.isFinite(dimScale)&&dimScale>1e-12?dimScale:1d;
            double a=Double.isFinite(arrowSize)&&arrowSize>1e-12?arrowSize:0d;
            return a*s;
        }
    }
    public static final class Table {
        private final Map<String,Entry> entries=new HashMap<>();
        public void add(String name,double dimScale,double arrowSize){
            String key=key(name);if(key.isEmpty())return;entries.put(key,new Entry(name==null?"":name.trim(),dimScale,arrowSize));
        }
        public Entry get(String name){return entries.get(key(name));}
        public double arrowSize(String name){Entry e=get(name);return e==null?0d:e.effectiveArrowSize();}
        public void addRecord(List<String> record)throws IOException{
            if(record==null)return;try{add(value(record,2,""),number(record,40,1d),number(record,41,0d));}
            catch(NumberFormatException e){throw new IOException("Geçersiz DXF DIMSTYLE kaydı",e);}
        }
    }

    public static Table parse(List<String> tags)throws IOException{
        Table table=new Table();String section="";
        try{
            for(int i=0;i<tags.size();){
                if(code(tags.get(i))!=0){i+=2;continue;}
                String type=tags.get(i+1).trim();int from=i+2;i=from;
                while(i<tags.size()&&code(tags.get(i))!=0)i+=2;
                if("SECTION".equals(type)){section=value(tags,from,i,2,"").trim();continue;}
                if("ENDSEC".equals(type)){section="";continue;}
                if(!"TABLES".equals(section)||!"DIMSTYLE".equals(type))continue;
                table.addRecord(new ArrayList<>(tags.subList(from,i)));
            }
        }catch(NumberFormatException e){throw new IOException("Geçersiz DXF DIMSTYLE tablosu",e);}
        return table;
    }

    private static String key(String s){return s==null?"":s.trim().toUpperCase(Locale.ROOT);}
    private static int code(String s){return Integer.parseInt(s.trim());}
    private static double number(List<String> tags,int wanted,double fallback){String v=value(tags,wanted,null);return v==null?fallback:Double.parseDouble(v.trim());}
    private static String value(List<String> tags,int wanted,String fallback){for(int i=0;i+1<tags.size();i+=2)if(code(tags.get(i))==wanted)return tags.get(i+1);return fallback;}
    private static String value(List<String> tags,int from,int to,int wanted,String fallback){for(int i=from;i+1<to;i+=2)if(code(tags.get(i))==wanted)return tags.get(i+1);return fallback;}
    private DxfDimStyles(){}
}
