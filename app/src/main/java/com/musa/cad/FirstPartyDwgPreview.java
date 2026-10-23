package com.musa.cad;

import java.io.File;
import java.io.IOException;

/**
 * Opt-in diagnostic adapter for the FIRST-PARTY binary R2018 DWG reader.
 * Never substitutes a partial indexed-object inventory for a full drawing.
 * The normal editor/file-open path remains unchanged until full model-space,
 * layers, visibility, block graph, text and style fidelity are verified.
 */
public final class FirstPartyDwgPreview {
    static { System.loadLibrary("musacad_dwg"); }
    private FirstPartyDwgPreview() {}

    private static native float[] readOriginalNative(
            String inputPath, int maxFloatWords) throws IOException;

    public static final class Result {
        public final NativeScene scene;
        public final boolean incomplete;
        private Result(NativeScene scene) {
            this.scene = scene;
            this.incomplete = scene.truncated;
        }
    }

    /**
     * Debug/acceptance-only read of true AC1032 source geometry.
     * Returns a visibly incomplete scene when unknown objects or styles exist.
     * It never edits/exports a DWG, nor silently substitutes empty DXF data.
     */
    public static Result read(File original, int maxFloatWords)
            throws IOException {
        if (original == null || !original.isFile())
            throw new IOException("Kaynak DWG bulunamadi");
        if (maxFloatWords < 256 || maxFloatWords > 9 * 1024 * 1024)
            throw new IOException("DWG onizleme bellek siniri gecersiz");
        float[] raw = readOriginalNative(
                original.getAbsolutePath(), maxFloatWords);
        NativeScene parsed = NativeScene.fromRaw(raw);
        return new Result(parsed);
    }
}
