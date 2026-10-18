package com.musa.cad;

import java.io.IOException;

/**
 * Short-lived file-local DXF identity verification context.
 * The full set and ALL handle canonicalization/uniqueness rules live only in
 * portable C; the Java adapter holds no additional CAD identity algorithm.
 */
public final class DxfSavedHandleSet implements AutoCloseable {
    private static final boolean sharedCoreLoaded;
    static {
        boolean available=false;
        try {
            System.loadLibrary("musacad_dwg");
            available=true;
        } catch(LinkageError | SecurityException missing) {
            // Never use a Java HashSet as an alternative CAD engine.
        }
        sharedCoreLoaded=available;
    }
    private static native long openNative(int maximum);
    private static native int insertNative(long handle,String raw);
    private static native void closeNative(long handle);
    private long context;

    public DxfSavedHandleSet(int maximum)throws IOException {
        if(!sharedCoreLoaded)
            throw new IOException("Ortak CAD çekirdeği olmadan DXF kimlikleri doğrulanamaz");
        if(maximum<1||maximum>1_500_000)
            throw new IOException("DXF kimlik sınırı geçersiz");
        try {
            context=openNative(maximum);
        }catch(LinkageError missing) {
            throw new IOException("Ortak CAD çekirdeğinde DXF kimlik kümesi yok",missing);
        }
        if(context==0L)
            throw new IOException("DXF kimlik denetimi için bellek ayrılamadı");
    }

    public synchronized void reserve(String raw)throws IOException {
        if(context==0L)throw new IOException("DXF kimlik denetimi kapatıldı");
        final int result;
        try {
            result=insertNative(context,raw);
        }catch(LinkageError missing) {
            throw new IOException("Ortak CAD çekirdeğinde DXF kimlik denetimi yok",missing);
        }
        switch(result) {
            case 1:return;
            case 0:throw new IOException("DXF nesne tutamacı yinelendi");
            case -1:throw new IOException("Invalid DXF handle hexadecimal value");
            case -2:throw new IOException("DXF kimlik sınırı aşıldı");
            case -3:throw new IOException("DXF kimlik denetim belleği yetersiz");
            default:throw new IOException("DXF kimlik doğrulaması başarısız");
        }
    }
    @Override public synchronized void close() {
        long old=context;
        context=0L;
        if(old!=0L)closeNative(old);
    }
}
