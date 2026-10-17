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
    private static native int parsePairNative(String code,boolean valuePresent,
                                              int previousPairs,int maximumPairs);

    /** One canonical shared-core decision on a complete DXF group pair.
     * A blank value is valid; a missing second line must fail before saving.
     */
    public static int parsePair(String code,String value,int previousPairs,
                                int maximumPairs)throws IOException{
        if(!coreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan DXF grup çifti doğrulanamaz");
        final int result;
        try {
            result=parsePairNative(code,value!=null,previousPairs,maximumPairs);
        } catch(LinkageError unavailable) {
            throw new IOException("Ortak CAD çekirdeğinde DXF çift kontrolü bulunamadı",unavailable);
        }
        switch(result){
            case -1:throw new IOException("Eksik DXF grup çifti");
            case -2:throw new IOException("Geçersiz DXF grup kodu");
            case -3:throw new IOException("DXF grup sayısı fazla");
            case -4:throw new IOException("DXF çift kontrolü parametreleri geçersiz");
            default:if(result<0||result>1071)
                throw new IOException("Ortak CAD çekirdeği geçersiz DXF kodu döndürdü");
        }
        return result;
    }


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
