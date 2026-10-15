package com.musa.cad;

import java.util.Arrays;

/**
 * Remembers the last successfully exported 3D geometry, not merely the source
 * DXF opened by the editor. A saved copy is a valid exit checkpoint even when
 * the original, intentionally untouched, source has different coordinates.
 */
public final class Cad3dSaveCheckpoint {
    private float[] saved;

    public Cad3dSaveCheckpoint(float[] opened) {
        markSaved(opened);
    }

    /** Call only after a successful, fully published 3D DXF export. */
    public void markSaved(float[] exported) {
        if (exported == null || exported.length % 3 != 0)
            throw new IllegalArgumentException("Invalid XYZ checkpoint");
        saved = exported.clone();
    }

    /** Immutable checkpoint coordinates for a single edited source vertex. */
    public float[] savedVertex(int vertex) {
        if(vertex<0||vertex>=saved.length/3)
            throw new IllegalArgumentException("Checkpoint vertex out of range");
        int k=vertex*3;
        return new float[]{saved[k],saved[k+1],saved[k+2]};
    }

    public boolean hasUnsavedChanges(float[] current) {
        // A missing/invalid model should never silently be considered saved.
        return current == null || !Arrays.equals(saved, current);
    }
}
