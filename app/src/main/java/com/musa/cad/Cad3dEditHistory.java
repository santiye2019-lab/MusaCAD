package com.musa.cad;

import java.util.ArrayDeque;

/**
 * Bounded 3D vertex edit history. One completed gizmo drag or axis-step is one
 * undoable transaction; transient touch previews must never flood the history.
 * Pure Java; edits are checked against the current mesh before any XYZ writes.
 */
public final class Cad3dEditHistory {
    public static final int MAX_ACTIONS=100;
    private static final float LIMIT=1e9f;
    private static final class Action {
        final int vertex;
        final float[] before,after;
        Action(int vertex,float[] before,float[] after){
            this.vertex=vertex;this.before=before.clone();this.after=after.clone();
        }
    }
    private final ArrayDeque<Action> undo=new ArrayDeque<>(),redo=new ArrayDeque<>();
    public void clear(){undo.clear();redo.clear();}
    public boolean canUndo(){return !undo.isEmpty();}
    public boolean canRedo(){return !redo.isEmpty();}
    public int undoCount(){return undo.size();}
    public int redoCount(){return redo.size();}

    private static boolean valid(float[] coords){
        if(coords==null||coords.length!=3)return false;
        for(float v:coords)if(!Float.isFinite(v)||Math.abs(v)>LIMIT)return false;
        return true;
    }
    private static boolean matches(float[] model,int vertex,float[] expected){
        if(model==null||model.length%3!=0||vertex<0||vertex>=model.length/3)return false;
        int k=vertex*3;
        return Float.floatToIntBits(model[k])==Float.floatToIntBits(expected[0])&&
            Float.floatToIntBits(model[k+1])==Float.floatToIntBits(expected[1])&&
            Float.floatToIntBits(model[k+2])==Float.floatToIntBits(expected[2]);
    }
    private static boolean same(float[] a,float[] b){
        return Float.floatToIntBits(a[0])==Float.floatToIntBits(b[0])&&
            Float.floatToIntBits(a[1])==Float.floatToIntBits(b[1])&&
            Float.floatToIntBits(a[2])==Float.floatToIntBits(b[2]);
    }

    /** Call only after the editor has applied a full, valid vertex displacement. */
    public boolean record(float[] model,int vertex,float[] before,float[] after){
        if(!valid(before)||!valid(after)||!matches(model,vertex,after))return false;
        if(same(before,after))return false;
        undo.addLast(new Action(vertex,before,after));
        while(undo.size()>MAX_ACTIONS)undo.removeFirst();
        redo.clear();
        return true;
    }

    public boolean undo(float[] model){return apply(model,undo,redo,true);}
    public boolean redo(float[] model){return apply(model,redo,undo,false);}

    private boolean apply(float[] model,ArrayDeque<Action> source,ArrayDeque<Action> target,
                          boolean reversing){
        Action edit=source.peekLast();
        if(edit==null)return false;
        float[] expected=reversing?edit.after:edit.before;
        float[] destination=reversing?edit.before:edit.after;
        if(!valid(destination)||!matches(model,edit.vertex,expected)){
            // A drawing was replaced or edited outside this transaction stack:
            // refuse to write stale coordinates, and invalidate both histories.
            clear();
            return false;
        }
        int k=edit.vertex*3;
        for(int c=0;c<3;c++)model[k+c]=destination[c];
        source.removeLast();
        target.addLast(edit);
        return true;
    }
}
