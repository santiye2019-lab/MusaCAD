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

    public boolean hasUnsavedChanges(float[] current) {
        // A missing/invalid model should never silently be considered saved.
        return current == null || !Arrays.equals(saved, current);
    }
}
