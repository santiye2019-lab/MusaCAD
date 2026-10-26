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

    /* Explicit opt-in ONE-INSERT source packet; not a normal CAD open.
     * Source is read-only and remains on the caller's disk. */
    private static native float[] readInsertLineNative(
            String inputPath,long insertHandle,int maxFloatWords,long[] audit)
            throws IOException;

    /** Genuine source evidence, NOT display-ready color or visibility.
     * Counts R2018 indexed OBJECT records, including non-entity records.
     * The diagnostic deliberately does not infer model-space owners or XREF. */
    public static final class Readiness {
        public final long indexed,planarRecognized,sourceStyleLinked;
        public final long directAci,byLayer,byBlock,complex;
        public final long sourceStyleUnresolved,otherRecords,narrowLayerAciEvidence;
        public final long directRgb24,complexUnresolved,sourceEntityHidden,otherComplex;
        // v6 packet admission ONLY: one DWG source object may render many
        // primitives, and unrecognized indexed types may include non-entities.
        public final long previewCandidateRecords,previewAdmittedRecords;
        public final long previewRejectedCandidates,previewOtherIndexedRecords;
        public final long previewPrimitives,previewOmittedRecords;
        public final long undecodedTextMtext,undecodedAttribAttdef;
        public final long undecodedBlockMarkers,undecodedInsertMinsert;
        public final long otherOriginalTypeRecords;
        public final boolean colorAndVisibilityComplete;
        private Readiness(long[] raw)throws IOException {
            if(raw==null||raw.length!=25)
                throw new IOException("DWG stil ve onizleme sayaci eksik");
            for(int i=0;i<raw.length;i++){
                long maximum=i==17?9L*1024*1024:1000000L;
                if(raw[i]<0L||raw[i]>maximum)
                    throw new IOException("DWG kaynak/ilkel sayaci gecersiz");
            }
            indexed=raw[0];planarRecognized=raw[1];sourceStyleLinked=raw[2];
            directAci=raw[3];byLayer=raw[4];byBlock=raw[5];
            complex=raw[6];sourceStyleUnresolved=raw[7];
            otherRecords=raw[8];narrowLayerAciEvidence=raw[9];
            colorAndVisibilityComplete=raw[10]!=0L;
            directRgb24=raw[11];
            sourceEntityHidden=raw[12];
            previewCandidateRecords=raw[13];
            previewAdmittedRecords=raw[14];
            previewRejectedCandidates=raw[15];
            previewOtherIndexedRecords=raw[16];
            previewPrimitives=raw[17];
            previewOmittedRecords=raw[18];
            undecodedTextMtext=raw[20];
            undecodedAttribAttdef=raw[21];
            undecodedBlockMarkers=raw[22];
            undecodedInsertMinsert=raw[23];
            otherOriginalTypeRecords=raw[24];
            complexUnresolved=complex-directRgb24;
            otherComplex=complexUnresolved-sourceEntityHidden;
            if(raw[10]!=0L||indexed<=0L||
               planarRecognized+otherRecords!=indexed||
               sourceStyleLinked+sourceStyleUnresolved!=planarRecognized||
               directAci+byLayer+byBlock+complex!=sourceStyleLinked||
               narrowLayerAciEvidence>sourceStyleLinked||
               directRgb24>complex||
               sourceEntityHidden>complex-directRgb24||
               raw[19]!=0L||
               previewCandidateRecords+previewOtherIndexedRecords!=indexed||
               previewAdmittedRecords+previewRejectedCandidates!=
                   previewCandidateRecords||
               previewRejectedCandidates+previewOtherIndexedRecords!=
                   previewOmittedRecords||
               previewAdmittedRecords>previewPrimitives||
               previewPrimitives<=0L||
               undecodedTextMtext+undecodedAttribAttdef+
                   undecodedBlockMarkers+undecodedInsertMinsert+
                   otherOriginalTypeRecords!=indexed||
               undecodedTextMtext+undecodedAttribAttdef+
                   undecodedBlockMarkers+undecodedInsertMinsert>
                       previewOtherIndexedRecords) 
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
        long[] report=new long[25];
        float[] raw=readOriginalNative(
                original.getAbsolutePath(),maxFloatWords,report);
        NativeScene parsed=NativeScene.fromRaw(raw);
        Readiness readiness=new Readiness(report);
        if(readiness.previewPrimitives!=parsed.primitiveCount)
            throw new IOException("DWG onizleme nesne/ilkel sayaclari tutarsiz");
        return new Result(parsed,readiness);
    }
    /** One strict R2018 INSERT's original source-owned planar LINE evidence.
     * This is NOT a model-space, layer-visible or AutoCAD-equivalent scene.
     * scene is null when every original source LINE is deliberately excluded
     * by a color/visibility/precision diagnostic gate. */
    public static final class InsertLineDiagnostic {
        public final NativeScene scene;
        public final long insertHandle;
        public final long originalSourceLines,admittedSourceLines,omittedSourceLines;
        public final boolean incomplete;
        private InsertLineDiagnostic(NativeScene preview,long[] audit) {
            scene=preview;
            insertHandle=audit[0];
            originalSourceLines=audit[1];
            admittedSourceLines=audit[2];
            omittedSourceLines=audit[3];
            incomplete=true;
        }
    }

    /** Diagnostic adapter for a source-verified DWG INSERT H handle.
     * Never defaults to ACI7 or claims BYLAYER/BYBLOCK effective colors.
     * The production editor, JNI scene and original DWG stay untouched. */
    public static InsertLineDiagnostic readInsertLineDiagnostic(
            File original,long insertHandle,int maxFloatWords)throws IOException {
        if(original==null||!original.isFile())
            throw new IOException("Kaynak DWG bulunamadi");
        if(insertHandle<=0L||maxFloatWords<7||maxFloatWords>9*1024*1024)
            throw new IOException("Gecersiz INSERT H veya kaynak bellek siniri");
        long[] evidence=new long[8];
        float[] v6=readInsertLineNative(
                original.getAbsolutePath(),insertHandle,maxFloatWords,evidence);
        if(v6==null||v6.length<7||v6.length>7+6*64||
           evidence[0]!=insertHandle||
           evidence[1]<0||evidence[1]>64||
           evidence[2]<0||evidence[2]>evidence[1]||
           evidence[3]!=evidence[1]-evidence[2]||
           evidence[4]!=v6.length||
           evidence[5]!=1||evidence[6]!=1||evidence[7]!=0||
           v6[0]!=6f||v6[1]!=(float)evidence[2]||v6[6]!=1f)
            throw new IOException("Orijinal INSERT geometri raporu tutarsiz");
        NativeScene parsed=null;
        if(evidence[2]!=0L){
            parsed=NativeScene.fromRaw(v6);
            if(parsed.primitiveCount!=evidence[2]||!parsed.truncated)
                throw new IOException("Eksik DWG blok onizlemesi tam gibi isaretlendi");
        }else if(v6.length!=7) {
            throw new IOException("Bos INSERT icin sahte geometri paketi");
        }
        return new InsertLineDiagnostic(parsed,evidence);
    }

}
