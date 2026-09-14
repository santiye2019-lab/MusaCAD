package com.musa.cad;

import java.math.BigInteger;
import java.util.*;

/** Parses SORTENTSTABLE mappings used by AutoCAD DRAWORDER. */
public final class DxfDrawOrder {
    public static Map<String,String> parse(List<String> tags){
        LinkedHashMap<String,String> result=new LinkedHashMap<>();
        if(tags==null)return result;
        String section="";
        for(int i=0;i<tags.size();){
            int code=code(tags,i);if(code!=0){i+=2;continue;}
            String type=value(tags,i);int from=i+2;i=from;
            while(i<tags.size()&&code(tags,i)!=0)i+=2;
            if("SECTION".equals(type)){section=text(tags,from,i,2,"");continue;}
            if("ENDSEC".equals(type)){section="";continue;}
            if("OBJECTS".equals(section)&&"SORTENTSTABLE".equals(type))addRecord(tags,from,i,result);
        }
        return result;
    }

    /** A SORTENTSTABLE stores repeated 331 entity handle + 5 sort-handle pairs. */
    public static void addRecord(List<String> tags,int from,int to,Map<String,String> out){
        String pending=null;
        for(int i=from;i+1<to;i+=2){
            int code=code(tags,i);String value=value(tags,i).trim();
            if(code==331){pending=key(value);continue;}
            if(code==5&&pending!=null){out.put(pending,key(value));pending=null;}
        }
    }

    /** Compare two entity handles according to SORTENTSTABLE, falling back to their own handles. */
    public static int compare(String a,String b,Map<String,String> order){
        String ka=effective(a,order),kb=effective(b,order);
        if(ka.isEmpty()||kb.isEmpty())return 0;
        try{return new BigInteger(ka,16).compareTo(new BigInteger(kb,16));}
        catch(NumberFormatException ignored){return ka.compareToIgnoreCase(kb);}
    }

    private static String effective(String handle,Map<String,String> order){
        String h=key(handle);if(order==null)return h;String mapped=order.get(h);return mapped==null?h:mapped;
    }
    private static String key(String value){return value==null?"":value.trim().toUpperCase(Locale.ROOT);}
    private static int code(List<String> tags,int i){try{return Integer.parseInt(tags.get(i).trim());}catch(Exception e){return Integer.MIN_VALUE;}}
    private static String value(List<String> tags,int i){return i+1<tags.size()?tags.get(i+1):"";}
    private static String text(List<String> tags,int from,int to,int wanted,String fallback){
        for(int i=from;i+1<to;i+=2)if(code(tags,i)==wanted)return value(tags,i).trim();return fallback;
    }
    private DxfDrawOrder(){}
}
