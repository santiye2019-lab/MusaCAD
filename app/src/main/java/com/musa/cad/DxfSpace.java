package com.musa.cad;

import java.util.Collection;
import java.util.Map;

/** Model-space / paper-space and layout-name normalization for DXF entities. */
public final class DxfSpace {
    public static final String MODEL="Model";
    public static final String PAPER="Paper";

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

    /** Normalize AutoCAD model aliases and keep paper-layout names intact. */
    public static String normalizeName(String raw){
        String value=raw==null?"":raw.trim();
        if(value.isEmpty())return MODEL;
        String upper=value.toUpperCase(java.util.Locale.ROOT);
        if("MODEL".equals(upper)||"MODEL_SPACE".equals(upper)||"*MODEL_SPACE".equals(upper))return MODEL;
        return value;
    }

    public static boolean isModel(String layout){return MODEL.equals(normalizeName(layout));}

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

    private DxfSpace(){}
}
