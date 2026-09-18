package com.musa.cad;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/** Session-local named block definitions. Inserted instances are persisted as ordinary DXF geometry. */
public final class CadBlockLibrary {
    private final Map<String,List<CadEdit>> definitions=new LinkedHashMap<>();

    public boolean define(String name,List<CadEdit> geometry){
        String key=key(name);if(key.isEmpty()||geometry==null||geometry.isEmpty())return false;
        ArrayList<CadEdit> copy=new ArrayList<>();for(CadEdit edit:geometry)if(edit!=null)copy.add(edit.copy());
        if(copy.isEmpty())return false;definitions.put(key,copy);return true;
    }
    public boolean define(String name,CadEdit edit){return edit!=null&&define(name,Collections.singletonList(edit));}
    public boolean contains(String name){return definitions.containsKey(key(name));}
    public List<String> names(){return new ArrayList<>(definitions.keySet());}
    public List<CadEdit> insert(String name,float x,float y,float scale,float rotation){
        List<CadEdit> source=definitions.get(key(name));if(source==null||source.isEmpty()||!Float.isFinite(scale)||scale<=0f||!Float.isFinite(rotation))return Collections.emptyList();
        float cx=0f,cy=0f;for(CadEdit e:source){cx+=e.centerX();cy+=e.centerY();}cx/=source.size();cy/=source.size();
        ArrayList<CadEdit> out=new ArrayList<>();
        for(CadEdit e:source){CadEdit t=e.scaled(scale,cx,cy).rotated(rotation,cx,cy).translated(x-cx,y-cy);out.add(t);}
        return out;
    }
    public void clear(){definitions.clear();}
    private static String key(String name){return name==null?"":name.trim().toUpperCase(Locale.ROOT);}
}
