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
            String inputPath, int maxFloatWords,long[] audit) throws IOException;

    /** Genuine source evidence, NOT display-ready color or visibility.
     * Counts R2018 indexed OBJECT records, including non-entity records.
     * The diagnostic deliberately does not infer model-space owners or XREF. */
    public static final class Readiness {
        public final long indexed,planarRecognized,sourceStyleLinked;
        public final long directAci,byLayer,byBlock,complex;
        public final long sourceStyleUnresolved,otherRecords,narrowLayerAciEvidence;
        public final long directRgb24,complexUnresolved;
        public final boolean colorAndVisibilityComplete;
        private Readiness(long[] raw)throws IOException {
            if(raw==null||raw.length!=12)
                throw new IOException("DWG stil kanit sayaci eksik");
            for(long n:raw)if(n<0L||n>1000000L)
                throw new IOException("DWG kaynak nesne sayaci gecersiz");
            indexed=raw[0];planarRecognized=raw[1];sourceStyleLinked=raw[2];
            directAci=raw[3];byLayer=raw[4];byBlock=raw[5];
            complex=raw[6];sourceStyleUnresolved=raw[7];
            otherRecords=raw[8];narrowLayerAciEvidence=raw[9];
            colorAndVisibilityComplete=raw[10]!=0L;
            directRgb24=raw[11];
            complexUnresolved=complex-directRgb24;
            if(raw[10]!=0L||indexed<=0L||
               planarRecognized+otherRecords!=indexed||
               sourceStyleLinked+sourceStyleUnresolved!=planarRecognized||
               directAci+byLayer+byBlock+complex!=sourceStyleLinked||
               narrowLayerAciEvidence>sourceStyleLinked||
               directRgb24>complex)
                throw new IOException("DWG kaynak ve gorunurluk raporu tutarsiz");
        }
    }

    public static final class Result {
        public final NativeScene scene;
        public final Readiness readiness;
        public final boolean incomplete;
        private Result(NativeScene scene,Readiness readiness) {
            this.scene=scene;
            this.readiness=readiness;
            /* Audit cannot establish full model space or display semantics. */
            this.incomplete=true;
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
        long[] report=new long[12];
        float[] raw=readOriginalNative(
                original.getAbsolutePath(),maxFloatWords,report);
        NativeScene parsed=NativeScene.fromRaw(raw);
        Readiness readiness=new Readiness(report);
        return new Result(parsed,readiness);
    }
}
