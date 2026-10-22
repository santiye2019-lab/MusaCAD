package com.musa.cad;

import java.io.IOException;

/**
 * Experimental, source-safe bridge to the FIRST-PARTY shared C11
 * AC1018/R2004 indexed-object reader. No LibreDWG, no renderer and
 * no user-editable DWG scene are involved.
 *
 * The caller owns the input byte array, and this routine never modifies it.
 * No claim of AC1032 support, BLOCK/XREF fidelity or whole-DWG Save As.
 * This read-only diagnostic does NOT replace NativeCadEngine.openNative().
 */
public final class NativeAc1018Inspection {
    private static final int MAX_FILE_BYTES = 32 * 1024 * 1024;
    static { System.loadLibrary("musacad_dwg"); }

    private static native long[] inspectNative(byte[] protectedDwg) throws IOException;

    public static final class Result {
        public final long indexedRecords;
        public final long lines, arcs, circles, lwpolylines;
        public final long undecodableKnown, otherDeclaredTypes;
        public final long coveredRecordBytes, unindexedSectionBytes;
        public final boolean allCrcVerified, rangesNonoverlapping;
        private Result(long[] source) throws IOException {
            if (source == null || source.length != 11)
                throw new IOException("Native AC1018 result is incomplete");
            for (int i = 0; i < 9; i++)
                if (source[i] < 0) throw new IOException("Invalid native count");
            if (source[9] != 1 || source[10] != 1)
                throw new IOException("Unverified DWG object records");
            if (source[1] > source[0] || source[2] > source[0] ||
                source[3] > source[0] || source[4] > source[0] ||
                source[5] > source[0] || source[6] > source[0] ||
                source[1] + source[2] + source[3] + source[4] +
                source[5] + source[6] != source[0])
                throw new IOException("Inconsistent DWG object inventory");
            indexedRecords = source[0];
            lines = source[1]; arcs = source[2];
            circles = source[3]; lwpolylines = source[4];
            undecodableKnown = source[5];
            otherDeclaredTypes = source[6];
            coveredRecordBytes = source[7];
            unindexedSectionBytes = source[8];
            allCrcVerified = true;
            rangesNonoverlapping = true;
        }
        public boolean allGeometryDecodable() {
            return indexedRecords > 0 && undecodableKnown == 0 &&
                   otherDeclaredTypes == 0 && unindexedSectionBytes == 0;
        }
    }

    /**
     * Inspect a caller-provided, bounded original DWG without changing it.
     * A real CAD viewer must additionally resolve layers/blocks/styles,
     * full AC1032 semantics, and native viewport integration.
     */
    public static Result inspect(byte[] protectedDwg) throws IOException {
        if (protectedDwg == null || protectedDwg.length < 256 ||
            protectedDwg.length > MAX_FILE_BYTES)
            throw new IOException("AC1018 input size is unsupported");
        return new Result(inspectNative(protectedDwg));
    }

    private NativeAc1018Inspection() {}
}
