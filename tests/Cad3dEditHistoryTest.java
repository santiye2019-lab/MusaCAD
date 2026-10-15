import com.musa.cad.Cad3dEditHistory;

public final class Cad3dEditHistoryTest {
    private static void ok(boolean b,String msg){if(!b)throw new AssertionError(msg);}
    private static void xyz(float[] mesh,int vertex,float x,float y,float z){
        int k=vertex*3;
        ok(mesh[k]==x&&mesh[k+1]==y&&mesh[k+2]==z,
            "vertex "+vertex+": "+mesh[k]+","+mesh[k+1]+","+mesh[k+2]+" expected "+x+","+y+","+z);
    }
    private static float[] p(float x,float y,float z){return new float[]{x,y,z};}
    public static void main(String[]args){
        Cad3dEditHistory h=new Cad3dEditHistory();
        float[] m=p(1,2,3);
        ok(!h.undo(m)&&!h.redo(m),"empty undo/redo");
        float[] original=p(1,2,3),dragResult=p(5,6,7);
        System.arraycopy(dragResult,0,m,0,3);
        ok(h.record(m,0,original,dragResult),"completed drag is one edit");
        ok(h.undoCount()==1&&h.redoCount()==0,"drag added one action");
        ok(h.undo(m),"undo drag");xyz(m,0,1,2,3);
        ok(h.redo(m),"redo drag");xyz(m,0,5,6,7);
        ok(h.undo(m),"undo before branching");
        float[] stepResult=p(1,2,11);
        System.arraycopy(stepResult,0,m,0,3);
        ok(h.record(m,0,original,stepResult),"new axis step");
        ok(!h.canRedo(),"new edit must invalidate redo branch");
        ok(h.undo(m),"undo numeric step");xyz(m,0,1,2,3);

        float[] before=p(1,2,3),after=p(8,2,3);
        m[0]=8;
        ok(!h.record(m,0,before,before),"preview cancel / no coordinate change");
        ok(h.record(m,0,before,after),"completed drag");
        before[0]=999;after[0]=999; // must not mutate immutable history
        ok(h.undo(m),"defensive copy before/after");
        xyz(m,0,1,2,3);

        // Cross-vertex edits: undo/redo last action only, leaving the other vertex unchanged.
        h.clear();m=new float[]{1,2,3,4,5,6};
        float[] v0Before=p(1,2,3),v0After=p(7,2,3);
        m[0]=7;ok(h.record(m,0,v0Before,v0After),"vertex0 recorded");
        float[] v1Before=p(4,5,6),v1After=p(4,9,6);
        m[4]=9;ok(h.record(m,1,v1Before,v1After),"vertex1 recorded");
        ok(h.undo(m),"undo vertex1");xyz(m,1,4,5,6);xyz(m,0,7,2,3);
        ok(h.undo(m),"undo vertex0");xyz(m,0,1,2,3);
        ok(h.redo(m),"redo vertex0");xyz(m,0,7,2,3);
        ok(h.redo(m),"redo vertex1");xyz(m,1,4,9,6);

        // Any unexpected external write invalidates stale history before it touches geometry.
        m[3]=123;
        ok(!h.undo(m),"external edit must make undo fail");
        xyz(m,1,123,9,6);
        ok(!h.canUndo()&&!h.canRedo(),"external edit clears stale transaction stacks");

        h.clear();m=p(0,0,0);
        for(int i=0;i<130;i++){
            float[] old=p(i,0,0),next=p(i+1,0,0);
            m[0]=i+1;
            ok(h.record(m,0,old,next),"bounded step "+i);
        }
        ok(h.undoCount()==Cad3dEditHistory.MAX_ACTIONS,"history is bounded to 100 edits");
        for(int i=0;i<Cad3dEditHistory.MAX_ACTIONS;i++)ok(h.undo(m),"undo bounded edit");
        xyz(m,0,30,0,0);
        ok(!h.undo(m),"oldest removed edit not available");

        h.clear();
        ok(!h.record(m,-1,p(0,0,0),p(30,0,0)),"negative vertex rejected");
        ok(!h.record(m,10,p(0,0,0),p(30,0,0)),"out-of-range vertex rejected");
        ok(!h.record(m,0,p(Float.NaN,0,0),p(30,0,0)),"NaN snapshot rejected");
        ok(!h.record(m,0,p(0,0,0),p(1e10f,0,0)),"out-of-bounds snapshot rejected");
        ok(!h.canUndo(),"invalid edits do not enter history");
        System.out.println("3D undo/redo: drag transaction, numeric steps, redo branching, stale guards, 100-action bound passed");
    }
}
