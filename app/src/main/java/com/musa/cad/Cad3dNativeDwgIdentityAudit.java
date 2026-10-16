package com.musa.cad;

import java.io.File;
import java.io.IOException;

/**
 * Opt-in, read-only audit of top-level entities in the ORIGINAL binary DWG.
 * Runs off the UI thread; it never changes Android touch/rendering or exports.
 * Both file digests are verified around all native reads.
 *
 * The returned cross-check only establishes handle+type consistency with the
 * native DWG model-space index, NOT nested INSERT identity or equivalence.
 */
public final class Cad3dNativeDwgIdentityAudit {
    private Cad3dNativeDwgIdentityAudit(){}

    public static Cad3dDwgHandleCrosscheck read(File originalDwg)
            throws IOException {
        CadSaveSourceIntegrity source=CadSaveSourceIntegrity.capture(originalDwg);
        Cad3dDwgHandleCrosscheck.Builder builder=new Cad3dDwgHandleCrosscheck.Builder();
        try(NativeCadEngine session=NativeCadEngine.open(originalDwg)){
            source.requireUnchanged(originalDwg);
            long top=0;
            boolean more=true;
            while(more){
                NativeCadEngine.ModelIdentityPage page=
                    session.modelIdentityPage(top,1024);
                builder.addPage(top,page.packedHandleAndType,
                                page.nextTopLevelIndex,page.hasMore);
                top=page.nextTopLevelIndex;
                more=page.hasMore;
            }
            source.requireUnchanged(originalDwg);
            return builder.finish();
        }
    }
}
