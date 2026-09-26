package com.musa.cad;

import java.util.*;

/** Layer visibility rules shared by DXF block expansion and rendering. */
public final class DxfLayerState {
    public static String key(String value){
        String s=value==null?"":value.trim();
        return (s.isEmpty()?"0":s).toUpperCase(Locale.ROOT);
    }

    /** DXF LAYER: negative group 62 means off; bit 1 of group 70 means frozen. */
    public static boolean tableVisible(int rawAci,int flags){
        return rawAci>=0 && (flags&1)==0;
    }

    /** Add one visibility gate while keeping the chain compact and case-insensitive. */
    public static String[] addGate(String[] inherited,String layer){
        String gate=key(layer);
        String[] base=inherited==null?new String[0]:inherited;
        for(String value:base)if(gate.equals(key(value)))return base;
        String[] out=Arrays.copyOf(base,base.length+1);out[base.length]=gate;return out;
    }

    public static Set<String> normalized(Set<String> layers){
        LinkedHashSet<String> out=new LinkedHashSet<>();
        if(layers!=null)for(String layer:layers)out.add(key(layer));
        return out;
    }

    public static boolean allVisible(String[] gates,Set<String> selectedKeys){
        if(gates==null||gates.length==0)return true;
        if(selectedKeys==null||selectedKeys.isEmpty())return false;
        for(String gate:gates)if(!selectedKeys.contains(key(gate)))return false;
        return true;
    }

    private DxfLayerState(){}
}
