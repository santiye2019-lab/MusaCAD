/* SPDX-License-Identifier: GPL-3.0-or-later */
package com.musa.cad;
import java.io.IOException;
/** Compile-only host stub: verify actual Android FirstPartyDwgPreview Java
 * source against its JNI declarations without packaging any APK. */
final class NativeScene {
    final int primitiveCount=0;
    final boolean truncated=true;
    static NativeScene fromRaw(float[] raw)throws IOException{
        return new NativeScene();
    }
    static NativeScene joinPartialOriginalSourceScenes(
            NativeScene a,NativeScene b,int budget)throws IOException{
        return new NativeScene();
    }
    static NativeScene joinPartialOriginalSourceScenesWithDiagnosticCircles(
            NativeScene a,NativeScene b,NativeScene c,int budget)
            throws IOException{
        return new NativeScene();
    }
}
