package com.musa.cad;

import java.util.Collection;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/** Model-space / paper-space and layout-name normalization for DXF entities. */
public final class DxfSpace {
    public static final String MODEL="Model";
    public static final String PAPER="Paper";
    private static final String OWNER_PREFIX="@PAPER:";

    /**
     * Resolve the layout owning an entity. Group 67 explicitly marks paper space;
     * group 410 identifies the layout tab and is also honored when 67 is omitted.
     */
    public static String layout(int paperSpace,String rawLayout){
        String name=normalizeName(rawLayout);
        if(paperSpace==1){
            return MODEL.equals(name)?PAPER:name;
        }
        return name;
    }

    /** Resolve a paper entity through its owning BLOCK_RECORD when group 410 is absent. */
    public static String layout(int paperSpace,String rawLayout,String ownerHandle,Map<String,String> layoutByOwner){
        String raw=rawLayout==null?"":rawLayout.trim();
        if(!raw.isEmpty())return layout(paperSpace,raw);
        if(paperSpace==1&&layoutByOwner!=null){
            String owner=handle(ownerHandle);String named=layoutByOwner.get(owner);
            if(named!=null&&!named.trim().isEmpty())return normalizeName(named);
        }
        return paperSpace==1?PAPER:MODEL;
    }

    /** Normalize AutoCAD model aliases and keep paper-layout names intact. */
    public static String normalizeName(String raw){
        String value=raw==null?"":raw.trim();
        if(value.isEmpty())return MODEL;
        String upper=value.toUpperCase(Locale.ROOT);
        if("MODEL".equals(upper)||"MODEL_SPACE".equals(upper)||"*MODEL_SPACE".equals(upper))return MODEL;
        return value;
    }

    public static boolean isModel(String layout){return MODEL.equals(normalizeName(layout));}

    /** LAYOUT objects contain PlotSettings group 1 before the actual AcDbLayout group 1; use the last non-empty value. */
    public static String layoutObjectName(List<String> tags,int from,int to){
        String value=last(tags,from,to,1,"");return value.isEmpty()?MODEL:normalizeName(value);
    }

    /** The AcDbLayout owning BLOCK_RECORD is likewise the last group 330 in a LAYOUT object. */
    public static String layoutObjectOwner(List<String> tags,int from,int to){return handle(last(tags,from,to,330,""));}

    /** Temporary streaming key used until OBJECTS/LAYOUT records, which usually follow ENTITIES, have been parsed. */
    public static String unresolvedPaper(String ownerHandle){return OWNER_PREFIX+handle(ownerHandle);}
    public static boolean isUnresolvedPaper(String value){return value!=null&&value.startsWith(OWNER_PREFIX);}
    public static String unresolvedOwner(String value){return isUnresolvedPaper(value)?value.substring(OWNER_PREFIX.length()):"";}
    public static String resolveUnresolved(String value,Map<String,String> layoutByOwner){
        if(!isUnresolvedPaper(value))return normalizeName(value);
        String named=layoutByOwner==null?null:layoutByOwner.get(unresolvedOwner(value));
        return named==null||named.trim().isEmpty()?PAPER:normalizeName(named);
    }

    /** Prefer an explicitly requested populated layout, otherwise use the normal fallback rule. */
    public static String chooseActive(Map<String,? extends Collection<?>> rootsByLayout,String preferred){
        if(preferred!=null&&!preferred.trim().isEmpty()){
            String wanted=normalizeName(preferred);
            Collection<?> direct=rootsByLayout.get(wanted);
            if(direct!=null&&!direct.isEmpty())return wanted;
            for(Map.Entry<String,? extends Collection<?>> entry:rootsByLayout.entrySet()){
                if(normalizeName(entry.getKey()).equalsIgnoreCase(wanted)){
                    Collection<?> values=entry.getValue();
                    if(values!=null&&!values.isEmpty())return normalizeName(entry.getKey());
                }
            }
        }
        return chooseActive(rootsByLayout);
    }

    /** Prefer real model-space geometry; for paper-only files fall back to the first populated layout. */
    public static String chooseActive(Map<String,? extends Collection<?>> rootsByLayout){
        Collection<?> model=rootsByLayout.get(MODEL);
        if(model!=null&&!model.isEmpty())return MODEL;
        for(Map.Entry<String,? extends Collection<?>> entry:rootsByLayout.entrySet()){
            Collection<?> values=entry.getValue();
            if(values!=null&&!values.isEmpty())return normalizeName(entry.getKey());
        }
        return MODEL;
    }

    private static String last(List<String> tags,int from,int to,int wanted,String fallback){
        String result=fallback;
        for(int i=from;i+1<to;i+=2){
            int code;try{code=Integer.parseInt(tags.get(i).trim());}catch(Exception invalid){continue;}
            if(code==wanted){String value=tags.get(i+1).trim();if(!value.isEmpty())result=value;}
        }
        return result;
    }
    private static String handle(String raw){return raw==null?"":raw.trim().toUpperCase(Locale.ROOT);}
    private DxfSpace(){}
}
