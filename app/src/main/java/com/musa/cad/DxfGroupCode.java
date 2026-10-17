package com.musa.cad;

import java.io.IOException;

/** Platform adapter for the ONE shared DXF group-code parser in core/native.
 * Text decoding and I/O belong to each OS; CAD validation lives in portable C.
 */
public final class DxfGroupCode {
    private DxfGroupCode(){}
    private static final boolean coreLoaded;
    static {
        boolean loaded=false;
        try {
            System.loadLibrary("musacad_dwg");
            loaded=true;
        } catch(LinkageError | SecurityException unavailable) {
            // Fail closed: do not silently revert to Java CAD validation.
        }
        coreLoaded=loaded;
    }
    private static native int parseNative(String field);

    public static int parse(String field)throws IOException{
        if(!coreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan DXF grup kodu doğrulanamaz");
        final int value;
        try {
            value=parseNative(field);
        } catch(LinkageError unavailable) {
            throw new IOException("Ortak CAD çekirdeğinde DXF grup kodu okuyucusu bulunamadı",unavailable);
        }
        if(value<0||value>1071)
            throw new IOException("Geçersiz DXF grup kodu");
        return value;
    }
}
