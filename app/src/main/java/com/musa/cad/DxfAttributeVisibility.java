package com.musa.cad;

import java.util.List;

/** Resolves DXF ATTRIB/ATTDEF visibility without confusing later repeated group-70 fields. */
public final class DxfAttributeVisibility {
    public static boolean isInvisible(String type,List<String> tags,int from,int to){
        if(!"ATTRIB".equals(type)&&!"ATTDEF".equals(type))return false;
        String subclass="ATTRIB".equals(type)?"AcDbAttribute":"AcDbAttributeDefinition";
        Integer scoped=flagAfterSubclass(tags,from,to,subclass);
        int flags=scoped!=null?scoped:firstInt(tags,from,to,70,0);
        return (flags&1)!=0;
    }

    private static Integer flagAfterSubclass(List<String> tags,int from,int to,String subclass){
        boolean active=false;
        try{
            for(int p=from;p+1<to;p+=2){
                int code=Integer.parseInt(tags.get(p).trim());String value=tags.get(p+1)==null?"":tags.get(p+1).trim();
                if(code==100){if(active)return null;active=subclass.equalsIgnoreCase(value);continue;}
                if(active&&code==70)return Integer.parseInt(value);
            }
        }catch(Exception ignored){}
        return null;
    }
    private static int firstInt(List<String> tags,int from,int to,int wanted,int fallback){
        try{for(int p=from;p+1<to;p+=2)if(Integer.parseInt(tags.get(p).trim())==wanted)return Integer.parseInt(tags.get(p+1).trim());}catch(Exception ignored){}
        return fallback;
    }
    private DxfAttributeVisibility(){}
}
