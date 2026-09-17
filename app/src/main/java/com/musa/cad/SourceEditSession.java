package com.musa.cad;

import java.util.*;

/** Non-destructive source-entity mutation state used by CadView. */
public final class SourceEditSession {
    private static final class Entry {
        final int id;final SourceRange range;final CadEdit original;CadEdit replacement;boolean deleted;
        Entry(int id,SourceRange range,CadEdit original){this.id=id;this.range=range;this.original=original.copy();}
        CadEdit current(){return replacement==null?original:replacement;}
        Entry copy(){Entry e=new Entry(id,range,original);e.replacement=replacement==null?null:replacement.copy();e.deleted=deleted;return e;}
    }
    private static final class Undo {
        final int id;final Entry previous;final int selected;
        Undo(int id,Entry previous,int selected){this.id=id;this.previous=previous;this.selected=selected;}
    }

    private final LinkedHashMap<Integer,Entry> entries=new LinkedHashMap<>();
    private final ArrayDeque<Undo> undo=new ArrayDeque<>();
    private int selected=-1;

    public void clear(){entries.clear();undo.clear();selected=-1;}
    public int selectedId(){return selected;}
    public boolean hasSelection(){Entry e=entries.get(selected);return e!=null&&!e.deleted;}

    public void select(int id,SourceRange range,CadEdit prototype){
        if(id<0||range==null||prototype==null){selected=-1;return;}
        Entry e=entries.get(id);if(e==null){e=new Entry(id,range,prototype);entries.put(id,e);}selected=e.deleted?-1:id;
    }
    public void clearSelection(){selected=-1;}

    public CadEdit currentSelected(){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().copy();}

    public boolean moveSelectedTo(float x,float y){
        Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);
        CadEdit c=e.current();e.replacement=c.translated(x-c.centerX(),y-c.centerY());return true;
    }

    public boolean rotateSelected(float degrees){
        Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);
        CadEdit c=e.current();e.replacement=c.rotated(degrees,c.centerX(),c.centerY());return true;
    }

    public CadEdit copySelected(float dx,float dy){
        Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().translated(dx,dy);
    }

    public boolean deleteSelected(){
        Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);e.deleted=true;e.replacement=null;selected=-1;return true;
    }

    public boolean undo(){
        Undo u=undo.pollFirst();if(u==null)return false;
        if(u.previous==null)entries.remove(u.id);else entries.put(u.id,u.previous.copy());selected=u.selected;return true;
    }

    public Set<Integer> hiddenSourceIds(){
        LinkedHashSet<Integer> ids=new LinkedHashSet<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)ids.add(e.id);return ids;
    }

    public List<CadEdit> replacements(){
        ArrayList<CadEdit> out=new ArrayList<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.add(e.replacement.copy());return out;
    }

    public Map<Integer,CadEdit> replacementMap(){
        LinkedHashMap<Integer,CadEdit> out=new LinkedHashMap<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.put(e.id,e.replacement.copy());return out;
    }

    public List<SourceRange> removals(){
        ArrayList<SourceRange> out=new ArrayList<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)out.add(e.range);return out;
    }

    public int modifiedCount(){int n=0;for(Entry e:entries.values())if(e.deleted||e.replacement!=null)n++;return n;}

    /** Finds a currently replaced source entity so it remains selectable after its original is hidden. */
    public int findReplacement(float x,float y,float tolerance){
        float best=Math.max(0,tolerance);int found=-1;
        for(Entry e:entries.values()){
            if(e.deleted||e.replacement==null)continue;float d=e.replacement.hitDistance(x,y);
            if(d<=best){best=d;found=e.id;}
        }
        return found;
    }

    public SourceRange rangeFor(int id){Entry e=entries.get(id);return e==null?null:e.range;}
    public CadEdit prototypeFor(int id){Entry e=entries.get(id);return e==null?null:e.original.copy();}

    private void save(Entry e){undo.addFirst(new Undo(e.id,e.copy(),selected));while(undo.size()>50)undo.removeLast();}
}
