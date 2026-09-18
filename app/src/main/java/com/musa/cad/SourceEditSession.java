package com.musa.cad;

import java.util.*;

/** Non-destructive source-entity mutation state used by CadView. */
public final class SourceEditSession {
    private static final class Entry {
        final int id;final SourceRange range;final CadEdit original;
        String layer,lineType;int colorMode,colorValue,lineWeight;double lineTypeScale;
        CadEdit replacement;boolean deleted;
        Entry(int id,SourceRange range,CadEdit original,String layer,int color,String lineType,double lineTypeScale,int lineWeight){
            this(id,range,original,layer,CadEdit.COLOR_TRUECOLOR,color&0x00FFFFFF,lineType,lineTypeScale,lineWeight);
        }
        Entry(int id,SourceRange range,CadEdit original,String layer,int colorMode,int colorValue,String lineType,double lineTypeScale,int lineWeight){
            this.id=id;this.range=range;this.original=original.copy();this.layer=layer==null||layer.trim().isEmpty()?"0":layer.trim();
            this.colorMode=colorMode>=CadEdit.COLOR_BYLAYER&&colorMode<=CadEdit.COLOR_TRUECOLOR?colorMode:CadEdit.COLOR_TRUECOLOR;
            this.colorValue=this.colorMode==CadEdit.COLOR_ACI?Math.max(1,Math.min(255,colorValue)):this.colorMode==CadEdit.COLOR_TRUECOLOR?(colorValue&0x00FFFFFF):colorValue;
            this.lineType=DxfLineStyle.normalizeName(lineType);this.lineTypeScale=Double.isFinite(lineTypeScale)&&lineTypeScale>0d?lineTypeScale:1d;
            this.lineWeight=rawLineWeight(lineWeight);
        }
        CadEdit current(){return replacement==null?original:replacement;}
        Entry copy(){Entry e=new Entry(id,range,original,layer,colorMode,colorValue,lineType,lineTypeScale,lineWeight);e.replacement=replacement==null?null:replacement.copy();e.deleted=deleted;return e;}
    }
    private static final class Undo {
        final int id;final Entry previous;final int selected;
        Undo(int id,Entry previous,int selected){this.id=id;this.previous=previous;this.selected=selected;}
    }

    private final LinkedHashMap<Integer,Entry> entries=new LinkedHashMap<>();
    private final ArrayDeque<Undo> undo=new ArrayDeque<>();
    private int selected=-1;

    /** Deep-copy state used when switching between open drawing tabs. */
    public static final class Snapshot {
        private final LinkedHashMap<Integer,Entry> entries;
        private final ArrayDeque<Undo> undo;
        private final int selected;
        private Snapshot(LinkedHashMap<Integer,Entry> entries,ArrayDeque<Undo> undo,int selected){
            this.entries=entries;this.undo=undo;this.selected=selected;
        }
    }

    public Snapshot snapshot(){
        LinkedHashMap<Integer,Entry> entryCopy=new LinkedHashMap<>();
        for(Map.Entry<Integer,Entry> item:entries.entrySet())entryCopy.put(item.getKey(),item.getValue().copy());
        ArrayDeque<Undo> undoCopy=new ArrayDeque<>();
        for(Undo item:undo)undoCopy.addLast(new Undo(item.id,item.previous==null?null:item.previous.copy(),item.selected));
        return new Snapshot(entryCopy,undoCopy,selected);
    }

    public void restore(Snapshot state){
        clear();if(state==null)return;
        for(Map.Entry<Integer,Entry> item:state.entries.entrySet())entries.put(item.getKey(),item.getValue().copy());
        for(Undo item:state.undo)undo.addLast(new Undo(item.id,item.previous==null?null:item.previous.copy(),item.selected));
        selected=state.selected;
    }

    public void clear(){entries.clear();undo.clear();selected=-1;}
    public int selectedId(){return selected;}
    public boolean hasSelection(){Entry e=entries.get(selected);return e!=null&&!e.deleted;}

    public void select(int id,SourceRange range,CadEdit prototype){select(id,range,prototype,"0",0xFFFFFFFF,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT);}
    public void select(int id,SourceRange range,CadEdit prototype,String layer,int color){select(id,range,prototype,layer,color,DxfLineStyle.CONTINUOUS,1d,DxfLineStyle.DEFAULT_LINEWEIGHT);}
    public void select(int id,SourceRange range,CadEdit prototype,String layer,int color,String lineType,double lineTypeScale,int lineWeight){
        select(id,range,prototype,layer,CadEdit.COLOR_TRUECOLOR,color&0x00FFFFFF,lineType,lineTypeScale,lineWeight);
    }
    public void select(int id,SourceRange range,CadEdit prototype,String layer,int colorMode,int colorValue,String lineType,double lineTypeScale,int lineWeight){
        if(id<0||range==null||prototype==null){selected=-1;return;}
        Entry e=entries.get(id);if(e==null){e=new Entry(id,range,prototype,layer,colorMode,colorValue,lineType,lineTypeScale,lineWeight);entries.put(id,e);}selected=e.deleted?-1:id;
    }
    public void clearSelection(){selected=-1;}

    public CadEdit currentSelected(){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().copy();}
    public String selectedLayer(){Entry e=entries.get(selected);return e==null||e.deleted?"0":e.layer;}
    public int selectedColorMode(){Entry e=entries.get(selected);return e==null||e.deleted?CadEdit.COLOR_BYLAYER:e.colorMode;}
    public int selectedColorValue(){Entry e=entries.get(selected);return e==null||e.deleted?7:e.colorValue;}
    public String selectedLineType(){Entry e=entries.get(selected);return e==null||e.deleted?DxfLineStyle.BYLAYER:e.lineType;}
    public int selectedLineWeight(){Entry e=entries.get(selected);return e==null||e.deleted?DxfLineStyle.LW_BYLAYER:e.lineWeight;}
    public boolean setSelectedProperties(String layer,int colorMode,int colorValue,String lineType,int lineWeight){
        Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);
        e.layer=layer==null||layer.trim().isEmpty()?"0":layer.trim();
        e.colorMode=colorMode>=CadEdit.COLOR_BYLAYER&&colorMode<=CadEdit.COLOR_TRUECOLOR?colorMode:CadEdit.COLOR_BYLAYER;
        e.colorValue=e.colorMode==CadEdit.COLOR_ACI?Math.max(1,Math.min(255,colorValue)):e.colorMode==CadEdit.COLOR_TRUECOLOR?(colorValue&0x00FFFFFF):colorValue;
        e.lineType=DxfLineStyle.normalizeName(lineType);e.lineWeight=rawLineWeight(lineWeight);
        if(e.replacement==null)e.replacement=e.original.copy();
        return true;
    }

    public boolean moveSelectedTo(float x,float y){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);CadEdit c=e.current();e.replacement=c.translated(x-c.centerX(),y-c.centerY());return true;}
    public boolean rotateSelected(float degrees){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);CadEdit c=e.current();e.replacement=c.rotated(degrees,c.centerX(),c.centerY());return true;}
    public CadEdit copySelected(float dx,float dy){Entry e=entries.get(selected);return e==null||e.deleted?null:e.current().translated(dx,dy);}
    public boolean deleteSelected(){Entry e=entries.get(selected);if(e==null||e.deleted)return false;save(e);e.deleted=true;e.replacement=null;selected=-1;return true;}

    public boolean undo(){Undo u=undo.pollFirst();if(u==null)return false;if(u.previous==null)entries.remove(u.id);else entries.put(u.id,u.previous.copy());selected=u.selected;return true;}

    public Set<Integer> hiddenSourceIds(){LinkedHashSet<Integer> ids=new LinkedHashSet<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)ids.add(e.id);return ids;}
    public List<CadEdit> replacements(){ArrayList<CadEdit> out=new ArrayList<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.add(e.replacement.copy());return out;}
    public Map<Integer,CadEdit> replacementMap(){LinkedHashMap<Integer,CadEdit> out=new LinkedHashMap<>();for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.put(e.id,e.replacement.copy());return out;}

    public List<SourceReplacement> replacementRecords(){
        ArrayList<SourceReplacement> out=new ArrayList<>();
        for(Entry e:entries.values())if(!e.deleted&&e.replacement!=null)out.add(new SourceReplacement(e.id,e.range,e.layer,e.colorMode,e.colorValue,e.lineType,e.lineTypeScale,e.lineWeight,e.replacement));
        return out;
    }

    public List<SourceRange> removals(){ArrayList<SourceRange> out=new ArrayList<>();for(Entry e:entries.values())if(e.deleted||e.replacement!=null)out.add(e.range);return out;}
    public int modifiedCount(){int n=0;for(Entry e:entries.values())if(e.deleted||e.replacement!=null)n++;return n;}

    /** Finds a currently replaced source entity so it remains selectable after its original is hidden. */
    public int findReplacement(float x,float y,float tolerance){float best=Math.max(0,tolerance);int found=-1;for(Entry e:entries.values()){if(e.deleted||e.replacement==null)continue;float d=e.replacement.hitDistance(x,y);if(d<=best){best=d;found=e.id;}}return found;}

    public SourceRange rangeFor(int id){Entry e=entries.get(id);return e==null?null:e.range;}
    public CadEdit prototypeFor(int id){Entry e=entries.get(id);return e==null?null:e.original.copy();}

    private static int rawLineWeight(int value){
        if(value==DxfLineStyle.LW_BYLAYER||value==DxfLineStyle.LW_BYBLOCK||value==DxfLineStyle.LW_DEFAULT)return value;
        return value>=0&&value<=211?value:DxfLineStyle.DEFAULT_LINEWEIGHT;
    }
    private void save(Entry e){undo.addFirst(new Undo(e.id,e.copy(),selected));while(undo.size()>50)undo.removeLast();}
}
