package com.musa.cad;

import java.util.*;

/** Non-destructive source-entity mutation state used by CadView. */
public final class SourceEditSession {
    private static final class Entry {
        final int id;final SourceRange range;final CadEdit original;String layer;int color;String lineType;double lineTypeScale;int lineWeight;
        CadEdit replacement;boolean deleted;
        Entry(int id,SourceRange range,CadEdit original,String layer,int color,String lineType,double lineTypeScale,int lineWeight){
            this.id=id;this.range=range;this.original=original.copy();this.layer=layer==null?"0":layer;this.color=color;
            this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=Double.isFinite(lineTypeScale)&&lineTypeScale>0d?lineTypeScale:1d;
            this.lineWeight=DxfLineStyle.normalizeWeight(lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT);
        }
        CadEdit current(){return replacement==null?original:replacement;}
        Entry copy(){Entry e=new Entry(id,range,original,layer,color,lineType,lineTypeScale,lineWeight);e.replacement=replacement==null?null:replacement.copy();e.deleted=deleted;return e;}
    }
    private static final class Undo {
        final int[] ids;final Entry[] previous;final int selected;
        Undo(int id,Entry previous,int selected){this.ids=new int[]{id};this.previous=new Entry[]{previous};this.selected=selected;}
        Undo(int id1,Entry previous1,int id2,Entry previous2,int selected){this.ids=new int[]{id1,id2};this.previous=new Entry[]{previous1,previous2};this.selected=selected;}
        Undo(int[] ids,Entry[] previous,int selected){this.ids=ids.clone();this.previous=new Entry[previous.length];for(int i=0;i<previous.length;i++)this.previous[i]=previous[i]==null?null:previous[i].copy();this.selected=selected;}
    }

    private final LinkedHashMap<Integer,Entry> entries=new LinkedHashMap<>();
    private final ArrayDeque<Undo> undo=new ArrayDeque<>();
    private final ArrayDeque<Undo> redo=new ArrayDeque<>();
    private int selected=-1;

    public void clear(){entries.clear();undo.clear();redo.clear();selected=-1;}
    public void clearRedo(){redo.clear();}
    public int selectedId(){return selected;}
    public boolean hasSelection(){Entry e=entries.get(selected);return e!=null&&!e.deleted;}

    public void select(int id,SourceRange range,CadEdit prototype){select(id,range,prototype,"0",0xFFFFFFFF,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT);}
    public void select(int id,SourceRange range,CadEdit prototype,String layer,int color){select(id,range,prototype,layer,color,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT);}
    public void select(int id,SourceRange range,CadEdit prototype,String layer,int color,String lineType,double lineTypeScale,int lineWeight){
        if(id<0||range==null||prototype==null){selected=-1;return;}
        Entry e=entries.get(id);if(e==null){e=new Entry(id,range,prototype,layer,color,lineType,lineTypeScale,lineWeight);entries.put(id,e);}selected=e.deleted?-1:id;
    }
    public void clearSelection(){selected=-1;}

    public CadEdit currentSelected(){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().copy();}
    public CadEdit currentFor(int id){Entry e=entries.get(id);return e==null||e.deleted?null:e.current().copy();}

    public boolean moveSelectedTo(float x,float y){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);CadEdit c=e.current();e.replacement=c.translated(x-c.centerX(),y-c.centerY());return true;}
    public boolean rotateSelected(float degrees){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);CadEdit c=e.current();e.replacement=c.rotated(degrees,c.centerX(),c.centerY());return true;}
    public boolean scaleSelected(float factor){Entry e=entries.get(selected);if(e==null||e.deleted||!Float.isFinite(factor)||factor<=0f)return false;save(e);CadEdit c=e.current();e.replacement=c.scaled(factor,c.centerX(),c.centerY());return true;}
    public boolean mirrorSelected(boolean verticalAxis){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);CadEdit c=e.current();e.replacement=c.mirrored(verticalAxis,c.centerX(),c.centerY());return true;}
    public boolean replaceSelected(CadEdit replacement){Entry e=entries.get(selected);if(e==null||e.deleted||replacement==null)return false;save(e);e.replacement=replacement.copy();return true;}
    public boolean replaceSelectedAndOther(int otherId,SourceRange otherRange,CadEdit otherPrototype,String otherLayer,int otherColor,String otherLineType,double otherLineTypeScale,int otherLineWeight,CadEdit selectedReplacement,CadEdit otherReplacement){
        Entry first=entries.get(selected);
        if(first==null||first.deleted||otherId<0||otherId==selected||otherRange==null||otherPrototype==null||selectedReplacement==null||otherReplacement==null)return false;
        Entry previousOther=entries.get(otherId);
        Entry other=previousOther==null?new Entry(otherId,otherRange,otherPrototype,otherLayer,otherColor,otherLineType,otherLineTypeScale,otherLineWeight):previousOther;
        if(other.deleted)return false;
        record(new Undo(first.id,first.copy(),otherId,previousOther==null?null:previousOther.copy(),selected));
        if(previousOther==null)entries.put(otherId,other);
        first.replacement=selectedReplacement.copy();
        other.replacement=otherReplacement.copy();
        return true;
    }
    public boolean replaceSelectedAndDeleteOther(int otherId,SourceRange otherRange,CadEdit otherPrototype,String otherLayer,int otherColor,String otherLineType,double otherLineTypeScale,int otherLineWeight,CadEdit selectedReplacement){
        Entry first=entries.get(selected);
        if(first==null||first.deleted||otherId<0||otherId==selected||otherRange==null||otherPrototype==null||selectedReplacement==null)return false;
        Entry previousOther=entries.get(otherId);
        Entry other=previousOther==null?new Entry(otherId,otherRange,otherPrototype,otherLayer,otherColor,otherLineType,otherLineTypeScale,otherLineWeight):previousOther;
        if(other.deleted)return false;
        record(new Undo(first.id,first.copy(),otherId,previousOther==null?null:previousOther.copy(),selected));
        if(previousOther==null)entries.put(otherId,other);
        first.replacement=selectedReplacement.copy();
        other.deleted=true;other.replacement=null;
        return true;
    }
    public boolean updateSelectedStyle(Integer color,String lineType,Integer lineWeight){
        Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);
        if(color!=null)e.color=color;
        if(lineType!=null&&!lineType.trim().isEmpty())e.lineType=DxfLineStyle.normalizeName(lineType);
        if(lineWeight!=null)e.lineWeight=DxfLineStyle.normalizeWeight(lineWeight,DxfLineStyle.DEFAULT_LINEWEIGHT);
        e.replacement=e.current().copy();return true;
    }

    public boolean matchSelectedPropertiesToOther(int otherId,SourceRange otherRange,CadEdit otherPrototype,String otherLayer,int otherColor,String otherLineType,double otherLineTypeScale,int otherLineWeight){
        Entry source=entries.get(selected);
        if(source==null||source.deleted||otherId<0||otherId==selected||otherRange==null||otherPrototype==null)return false;
        Entry previousOther=entries.get(otherId);
        Entry target=previousOther==null?new Entry(otherId,otherRange,otherPrototype,otherLayer,otherColor,otherLineType,otherLineTypeScale,otherLineWeight):previousOther;
        if(target.deleted)return false;
        record(new Undo(otherId,previousOther==null?null:previousOther.copy(),selected));
        if(previousOther==null)entries.put(otherId,target);
        target.layer=source.layer;target.color=source.color;target.lineType=source.lineType;target.lineTypeScale=source.lineTypeScale;target.lineWeight=source.lineWeight;
        target.replacement=target.current().copy();
        return true;
    }
    public CadEdit offsetSelected(float distance){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().offset(distance);}
    public CadEdit copySelected(float dx,float dy){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().translated(dx,dy);}
    public boolean deleteSelected(){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);e.deleted=true;e.replacement=null;selected=-1;return true;}

    public boolean undo(){Undo u=undo.pollFirst();if(u==null)return false;redo.addFirst(snapshot(u.ids,selected));restore(u);return true;}
    public boolean redo(){Undo r=redo.pollFirst();if(r==null)return false;undo.addFirst(snapshot(r.ids,selected));trim(undo);restore(r);return true;}

    public Set<Integer> hiddenSourceIds(){LinkedHashSet<Integer> ids=new LinkedHashSet<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)ids.add(e.id);return ids;}
    public List<CadEdit> replacements(){ArrayList<CadEdit> out=new ArrayList<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.add(e.replacement.copy());return out;}
    public Map<Integer,CadEdit> replacementMap(){LinkedHashMap<Integer,CadEdit> out=new LinkedHashMap<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.put(e.id,e.replacement.copy());return out;}

    public List<SourceReplacement> replacementRecords(){
        ArrayList<SourceReplacement> out=new ArrayList<>();
        for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.add(new SourceReplacement(e.id,e.range,e.layer,e.color,e.lineType,e.lineTypeScale,e.lineWeight,e.replacement));
        return out;
    }

    public List<SourceRange> removals(){ArrayList<SourceRange> out=new ArrayList<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)out.add(e.range);return out;}
    public int modifiedCount(){int n=0;for(Entry e:entries.values())if(e.deleted||e.replacement!=null)n++;return n;}

    /** Finds a currently replaced source entity so it remains selectable after its original is hidden. */
    public int findReplacement(float x,float y,float tolerance){float best=Math.max(0,tolerance);int found=-1;for(Entry e:entries.values()){if(e.deleted||e.replacement==null)continue;float d=e.replacement.hitDistance(x,y);if(d<=best){best=d;found=e.id;}}return found;}

    public SourceRange rangeFor(int id){Entry e=entries.get(id);return e==null?null:e.range;}
    public CadEdit prototypeFor(int id){Entry e=entries.get(id);return e==null?null:e.original.copy();}

    private void save(Entry e){record(new Undo(e.id,e.copy(),selected));}
    private void record(Undo state){undo.addFirst(state);trim(undo);redo.clear();}
    private static void trim(ArrayDeque<Undo> stack){while(stack.size()>50)stack.removeLast();}
    private Undo snapshot(int[] ids,int selectedState){Entry[] states=new Entry[ids.length];for(int i=0;i<ids.length;i++){Entry e=entries.get(ids[i]);states[i]=e==null?null:e.copy();}return new Undo(ids,states,selectedState);}
    private void restore(Undo state){for(int i=0;i<state.ids.length;i++){Entry e=state.previous[i];if(e==null)entries.remove(state.ids[i]);else entries.put(state.ids[i],e.copy());}selected=state.selected;}
}
