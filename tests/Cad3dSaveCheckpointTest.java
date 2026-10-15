import com.musa.cad.Cad3dSaveCheckpoint;

/** Tests last-successful-export checkpoint separately from the original source DXF. */
public final class Cad3dSaveCheckpointTest {
    private static void ok(boolean value,String detail){
        if(!value)throw new AssertionError(detail);
    }
    public static void main(String[] args){
        float[] initial={1,2,3,4,5,6};
        Cad3dSaveCheckpoint checkpoint=new Cad3dSaveCheckpoint(initial);
        ok(!checkpoint.hasUnsavedChanges(initial),"just-opened project is clean");
        initial[0]=8;
        ok(checkpoint.hasUnsavedChanges(initial),"edit since opening is dirty");
        initial[0]=1;
        ok(!checkpoint.hasUnsavedChanges(initial),"undo back to checkpoint is clean");
        initial[0]=8;
        ok(checkpoint.hasUnsavedChanges(initial),"redo restores pending changes");

        checkpoint.markSaved(initial); // only called on successfully published export
        ok(!checkpoint.hasUnsavedChanges(initial),"exported copy becomes exit checkpoint");
        initial[0]=9;
        ok(checkpoint.hasUnsavedChanges(initial),"edit after export is dirty");
        initial[0]=8;
        ok(!checkpoint.hasUnsavedChanges(initial),"undo after export returns to saved copy");

        float[] external={8,2,3,4,5,6};
        checkpoint.markSaved(external);
        external[0]=123;
        ok(!checkpoint.hasUnsavedChanges(initial),"checkpoint owns a defensive copy");
        ok(checkpoint.hasUnsavedChanges(external),"external mutation cannot change saved baseline");
        ok(checkpoint.hasUnsavedChanges(null),"missing live geometry is unsafe");
        ok(checkpoint.hasUnsavedChanges(new float[3]),"different mesh is unsafe");

        boolean rejected=false;
        try{checkpoint.markSaved(new float[]{1,2});}catch(IllegalArgumentException expected){rejected=true;}
        ok(rejected,"partial XYZ export must be rejected");
        ok(!checkpoint.hasUnsavedChanges(initial),"rejected checkpoint preserves prior saved geometry");
        rejected=false;
        try{new Cad3dSaveCheckpoint(null);}catch(IllegalArgumentException expected){rejected=true;}
        ok(rejected,"null initial model must be rejected");
        System.out.println("3D save checkpoint: undo/redo, successful export baseline, copy, validation passed");
    }
}
