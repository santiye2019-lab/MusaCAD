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

    /* All real source-verified INSERT LINEs in one experimental scene, still
     * deliberately PARTIAL; not the editable DWG document open route. */
    private static native float[] readAllInsertLinesNative(
            String inputPath,int maxFloatWords,long[] audit)
            throws IOException;

    /* Numeric-only WCS CIRCLE silhouettes. ACI6 is a deliberately
     * NON-SOURCE wireframe indicator, NEVER original DWG color. */
    private static native float[] readCircleGeometryEvidenceNative(
            String inputPath,int maxFloatWords,long[] audit)
            throws IOException;

    /* Genuine R2018 source LAYER ACI, not effective visible paint.
     * Six longs per verified row: INSERT/BLOCK/CIRCLE/LAYER H, ACI, flags.
     * Unknown owner/space, viewport, frozen/off and XREF stay unresolved. */
    private static native long[] readCircleLayerAciEvidenceNative(
            String inputPath,int maxRows,long[] audit) throws IOException;

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

    /** An entire source file's authenticated INSERT LINE instances in ONE
     * vector scene. This proves neither all original 2B objects were read
     * nor that model/paper-space visibility and final colors are correct. */
    public static final class AllInsertLinesDiagnostic {
        public final NativeScene scene;
        public final long indexedRecords,candidateInserts,verifiedInserts;
        public final long unsupportedInserts,sourceLines,admittedLines,omittedLines;
        public final boolean incomplete;
        private AllInsertLinesDiagnostic(NativeScene verified,long[] audit){
            scene=verified;
            indexedRecords=audit[0];
            candidateInserts=audit[1];
            verifiedInserts=audit[2];
            unsupportedInserts=audit[3];
            sourceLines=audit[4];
            admittedLines=audit[5];
            omittedLines=audit[6];
            incomplete=true;
        }
    }

    /** Experiment: show all independently authenticated R2018 INSERT LINE
     * instances together without requiring a user-supplied H. Unknown
     * BYLAYER/BYBLOCK, layer off/freeze, XREF and viewport remain omitted
     * or uncertain; never use the result as a normal editable DWG model. */
    public static AllInsertLinesDiagnostic readAllInsertLinesDiagnostic(
            File original,int maxFloatWords)throws IOException {
        if(original==null||!original.isFile())
            throw new IOException("Kaynak DWG bulunamadi");
        if(maxFloatWords<7||maxFloatWords>9*1024*1024)
            throw new IOException("Coklu INSERT bellek siniri gecersiz");
        long[] audit=new long[11];
        float[] v6=readAllInsertLinesNative(
                original.getAbsolutePath(),maxFloatWords,audit);
        if(v6==null||v6.length<7||
           v6.length>1048576||v6.length>maxFloatWords||
           audit[0]<1||audit[0]>1000000||
           audit[1]<0||audit[1]>4096||
           audit[2]<0||audit[3]<0||audit[2]+audit[3]!=audit[1]||
           audit[4]<0||audit[4]>4096L*64L||
           audit[5]<0||audit[6]<0||audit[5]+audit[6]!=audit[4]||
           audit[5]>Integer.MAX_VALUE||
           audit[7]!=v6.length||
           audit[8]!=(audit[5]>0?1:0)||
           audit[9]!=1||audit[10]!=0||
           v6[0]!=6f||v6[1]!=(float)audit[5]||v6[6]!=1f||
           audit[5]*5L>v6.length-7L||
           audit[5]*6L<v6.length-7L)
            throw new IOException("Coklu INSERT kaynak raporu tutarsiz");
        NativeScene parsed=null;
        if(audit[5]!=0L){
            parsed=NativeScene.fromRaw(v6);
            if(parsed.primitiveCount!=audit[5]||!parsed.truncated)
                throw new IOException("Coklu blok sahnesi hatali veya tam isaretlenmis");
        }else if(v6.length!=7){
            throw new IOException("Bos blok listesinden sahte cizim uretildi");
        }
        return new AllInsertLinesDiagnostic(parsed,audit);
    }

    /** One read-only experimental Android scene containing BOTH original
     * direct primitive candidates (including approx source TEXT/MTEXT) and
     * independently authenticated transformed INSERT LINE instances.
     *
     * Deliberately does not claim all source records, uniqueness of owner
     * (BLOCK definition geometry may be duplicated), complete layer/space
     * visibility, or accurate BYLAYER/BYBLOCK and original text fonts. */
    public static final class CombinedSourcesDiagnostic {
        public final NativeScene scene;
        public final Readiness original;
        public final long verifiedInsertInstances,unsupportedInsertInstances;
        public final long admittedInsertLines,omittedInsertLines;
        public final int shownPrimitiveCount;
        public final boolean incomplete;
        private CombinedSourcesDiagnostic(
                NativeScene joined,Readiness originalReadiness,
                AllInsertLinesDiagnostic inserts) {
            scene=joined;
            original=originalReadiness;
            verifiedInsertInstances=inserts.verifiedInserts;
            unsupportedInsertInstances=inserts.unsupportedInserts;
            admittedInsertLines=inserts.admittedLines;
            omittedInsertLines=inserts.omittedLines;
            shownPrimitiveCount=joined.primitiveCount;
            incomplete=true;
        }
    }

    public static CombinedSourcesDiagnostic readCombinedSourceDiagnostic(
            File original,int maxFloatWords)throws IOException {
        if(original==null||!original.isFile())
            throw new IOException("Kaynak DWG bulunamadi");
        if(maxFloatWords<256||maxFloatWords>9*1024*1024)
            throw new IOException("Birlesik DWG bellek siniri gecersiz");
        // Real original DWG, read-only; no reconstructed DXF or editor handoff.
        Result direct=read(original,maxFloatWords);
        AllInsertLinesDiagnostic inserts=readAllInsertLinesDiagnostic(
                original,maxFloatWords);
        if(!direct.incomplete||!inserts.incomplete)
            throw new IOException("Kaynak veriden tamamlanmis DWG iddiasi");
        NativeScene unified=NativeScene.joinPartialOriginalSourceScenes(
                direct.scene,inserts.scene,maxFloatWords);
        if(unified.primitiveCount!=
                (long)direct.scene.primitiveCount+inserts.admittedLines||
           !unified.truncated)
            throw new IOException("Kaynak 2B ve blok nesneleri uyusmuyor");
        return new CombinedSourcesDiagnostic(
                unified,direct.readiness,inserts);
    }

    /** A single deliberately INCOMPLETE original 2B diagnostic scene:
     * direct source primitives + authenticated INSERT LINE + WCS CIRCLE
     * numeric wireframes. CIRCLE ACI6 is NOT the source DWG color.
     * No owner, visibility, viewport, layer or font parity is inferred. */
    public static final class CombinedNumericCircleDiagnostic {
        public final NativeScene scene;
        public final Readiness original;
        public final long admittedInsertLines,omittedInsertLines;
        public final long numericCircles,unresolvedCircleOwnerOrLayer;
        public final int shownPrimitiveCount;
        public final boolean incomplete,sourceCircleColorAccepted;
        private CombinedNumericCircleDiagnostic(
                NativeScene value,Readiness readiness,
                AllInsertLinesDiagnostic lines,CircleGeometryEvidence circles){
            scene=value;original=readiness;
            admittedInsertLines=lines.admittedLines;
            omittedInsertLines=lines.omittedLines;
            numericCircles=circles.verifiedNumericCircles;
            unresolvedCircleOwnerOrLayer=circles.unresolvedOwnerOrLayer;
            shownPrimitiveCount=value.primitiveCount;
            incomplete=true;
            sourceCircleColorAccepted=false;
        }
    }
    public static CombinedNumericCircleDiagnostic
    readCombinedWithNumericCirclesDiagnostic(
            File original,int maxFloatWords)throws IOException{
        if(original==null||!original.isFile()||
           maxFloatWords<256||maxFloatWords>9*1024*1024)
            throw new IOException("Uc kaynakli DWG tani girisi gecersiz");
        Result direct=read(original,maxFloatWords);
        AllInsertLinesDiagnostic lines=readAllInsertLinesDiagnostic(
                original,maxFloatWords);
        CircleGeometryEvidence circles=readCircleGeometryEvidence(
                original,maxFloatWords);
        if(!direct.incomplete||!lines.incomplete||
           circles.complete||circles.sourceColorKnown)
            throw new IOException("CIRCLE kaynak renk/visibility iddiasi");
        NativeScene joined=
            NativeScene.joinPartialOriginalSourceScenesWithDiagnosticCircles(
                direct.scene,lines.scene,circles.scene,maxFloatWords);
        long expected=(long)direct.scene.primitiveCount+
            lines.admittedLines+circles.verifiedNumericCircles;
        if(!joined.truncated||joined.primitiveCount!=expected)
            throw new IOException("Uc kaynakli DWG sayisal sahnesi tutarsiz");
        return new CombinedNumericCircleDiagnostic(
                joined,direct.readiness,lines,circles);
    }

    /** Separate diagnostic of original transformed CIRCLE numeric geometry.
     * ACI6 outlines are a non-source warning color: owners, H4/H5, layer
     * visibility, BYLAYER/BYBLOCK and actual paint remain UNKNOWN. */
    public static final class CircleGeometryEvidence {
        public final NativeScene scene;
        public final long insertCandidates,verifiedInserts,unsupportedInserts;
        public final long circleCandidates,verifiedNumericCircles;
        public final long unresolvedOwnerOrLayer,omittedCircles;
        public final boolean sourceColorKnown,complete;
        private CircleGeometryEvidence(NativeScene value,long[] audit){
            scene=value;
            insertCandidates=audit[1];
            verifiedInserts=audit[2];
            unsupportedInserts=audit[3];
            circleCandidates=audit[4];
            verifiedNumericCircles=audit[5];
            omittedCircles=audit[6];
            unresolvedOwnerOrLayer=audit[7];
            sourceColorKnown=false;complete=false;
        }
    }

    public static CircleGeometryEvidence readCircleGeometryEvidence(
            File original,int maxFloatWords)throws IOException{
        if(original==null||!original.isFile())
            throw new IOException("Orijinal DWG bulunamadi");
        if(maxFloatWords<7||maxFloatWords>9*1024*1024)
            throw new IOException("Tel-kafes DWG bellek siniri gecersiz");
        long[] audit=new long[11];
        float[] packet=readCircleGeometryEvidenceNative(
                original.getAbsolutePath(),maxFloatWords,audit);
        if(packet==null||packet.length<7||
           packet.length>7+1024*(2+2*65)||
           packet.length>maxFloatWords||
           audit[0]<=0||audit[0]>1000000||
           audit[1]<0||audit[1]>4096||
           audit[2]<0||audit[3]<0||
           audit[2]+audit[3]!=audit[1]||
           audit[4]<0||audit[5]<0||audit[6]<0||
           audit[5]+audit[6]!=audit[4]||
           audit[7]<0||audit[7]>audit[5]||
           audit[8]!=audit[5]||audit[5]>1024||
           audit[9]!=packet.length||audit[10]!=1||
           packet[0]!=6f||packet[1]!=(float)audit[8]||
           packet[6]!=1f||packet.length!=7+audit[8]*132L)
            throw new IOException("Sayisal DWG CIRCLE kaniti tutarsiz");
        NativeScene parsed=null;
        if(audit[8]>0){
            for(int i=7;i<packet.length;i+=132){
                if(packet[i]!=518f||packet[i+1]!=65f)
                    throw new IOException("Tel-kafes CIRCLE renk/tur etiketi degistirildi");
            }
            parsed=NativeScene.fromRaw(packet);
            if(!parsed.truncated||parsed.primitiveCount!=audit[8])
                throw new IOException("Sayisal CIRCLE gorunumu tam DWG gibi isaretlendi");
        }
        return new CircleGeometryEvidence(parsed,audit);
    }

    /** Bounded source-only audit; no pixel/scene is produced here.
     * The numeric CIRCLE diagnostic still uses non-source magenta ACI6. */
    public static final class CircleSourceAciEvidence {
        public final long indexedRecords,insertCandidates,verifiedInserts;
        public final long unsupportedInserts,undecodableRecords;
        public final long circleCandidates,unresolvedCircles,sourceColorRows;
        public final boolean sourceOnly,finalVisiblePaintAccepted;
        private final long[] originalRows;
        private CircleSourceAciEvidence(long[] rows,long[] audit)
                throws IOException {
            if(rows==null||audit==null||audit.length!=10)
                throw new IOException("CIRCLE kaynak ACI kaniti eksik");
            for(int i=0;i<8;i++)
                if(audit[i]<0||audit[i]>1000000L)
                    throw new IOException("CIRCLE kaynak ACI sayaci gecersiz");
            if(audit[0]<1||audit[1]>4096||
               audit[2]+audit[3]!=audit[1]||
               audit[7]>256||audit[7]+audit[6]!=audit[5]||
               rows.length!=audit[7]*6L||
               audit[8]!=1L||audit[9]!=0L)
                throw new IOException("CIRCLE kaynak ACI envanteri tutarsiz");
            for(int i=0;i<rows.length;i+=6)
                if(rows[i]<=0||rows[i+1]<=0||rows[i+2]<=0||
                   rows[i+3]<=0||rows[i+4]<1||rows[i+4]>255||
                   rows[i+5]<0||rows[i+5]>65535)
                    throw new IOException("CIRCLE kaynak renk satiri gecersiz");
            indexedRecords=audit[0];
            insertCandidates=audit[1];verifiedInserts=audit[2];
            unsupportedInserts=audit[3];undecodableRecords=audit[4];
            circleCandidates=audit[5];unresolvedCircles=audit[6];
            sourceColorRows=audit[7];sourceOnly=true;
            finalVisiblePaintAccepted=false;
            originalRows=rows.clone();
        }
        public int firstLayerAci(){
            return originalRows.length==0?-1:(int)originalRows[4];
        }
        public long firstLayerHandle(){
            return originalRows.length==0?0L:originalRows[3];
        }
        public long firstCircleHandle(){
            return originalRows.length==0?0L:originalRows[2];
        }
    }

    /** Read-only original DWG color provenance. Never changes the renderer
     * or authorizes source ACI as effective BYLAYER/BYBLOCK paint. */
    public static CircleSourceAciEvidence readCircleSourceAciEvidence(
            File original)throws IOException {
        if(original==null||!original.isFile())
            throw new IOException("Orijinal DWG bulunamadi");
        long[] audit=new long[10];
        long[] rows=readCircleLayerAciEvidenceNative(
                original.getAbsolutePath(),256,audit);
        return new CircleSourceAciEvidence(rows,audit);
    }

}
