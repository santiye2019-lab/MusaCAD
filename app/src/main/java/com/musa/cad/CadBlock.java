package com.musa.cad;

import java.util.*;

public final class CadBlock {
    public static final class Definition {
        public final String name;
        public final List<CadEdit> members;
        public Definition(String name,List<CadEdit> members){
            String n=name==null?"":name.trim();
            if(n.isEmpty())throw new IllegalArgumentException("block name");
            this.name=n;
            ArrayList<CadEdit> copy=new ArrayList<>();
            if(members!=null)for(CadEdit e:members)if(e!=null)copy.add(e.copy());
            if(copy.isEmpty())throw new IllegalArgumentException("block members");
            this.members=Collections.unmodifiableList(copy);
        }
    }

    public static String normalizeName(String name){
        if(name==null)return "";
        return name.trim().replaceAll("[^A-Za-z0-9_-]","_");
    }

    public static CadEdit memberRelativeToCenter(CadEdit edit){
        if(edit==null)return null;
        return edit.translated(-edit.centerX(),-edit.centerY());
    }

    private CadBlock(){}
}
