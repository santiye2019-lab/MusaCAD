package com.musa.cad;

import java.io.File;
import java.io.IOException;

/**
 * Opaque native DWG session used by the MusaCAD native-engine path.
 *
 * Phase 1 deliberately does not replace the proven DXF renderer. It gives the
 * app a persistent parsed-DWG handle plus cheap document statistics and export.
 * Later viewport batches can be added to this class while all existing CAD
 * commands continue to operate through the current DxfParser.Result model.
 */
public final class NativeCadEngine implements AutoCloseable {
    static { System.loadLibrary("musacad_dwg"); }

    private static native long openNative(String input) throws IOException;
    private static native int exportDxfNative(long handle,String output);
    private static native long[] statsNative(long handle);
    private static native void closeNative(long handle);

    public static final class Stats {
        public final int readWarnings;
        public final long objectCount,entityCount,coreEntityCount,unsupportedEntityCount;
        public final long lineCount,circleCount,arcCount,lwPolylineCount,polyline2dCount,pointCount;
        public final long dwgVersion;

        private Stats(long[] v){
            if(v==null||v.length<12)throw new IllegalStateException("Native CAD istatistikleri alınamadı");
            readWarnings=(int)v[0];objectCount=v[1];entityCount=v[2];coreEntityCount=v[3];unsupportedEntityCount=v[4];
            lineCount=v[5];circleCount=v[6];arcCount=v[7];lwPolylineCount=v[8];polyline2dCount=v[9];pointCount=v[10];dwgVersion=v[11];
        }

        /** True when every discovered entity belongs to the first direct-render primitive set. */
        public boolean coreOnly(){return entityCount>0&&unsupportedEntityCount==0;}

        public double coreCoverage(){
            return entityCount<=0?0d:Math.min(1d,Math.max(0d,coreEntityCount/(double)entityCount));
        }
    }

    private long handle;
    private Stats cachedStats;

    public static NativeCadEngine open(File dwg)throws IOException{
        if(dwg==null||!dwg.isFile())throw new IOException("DWG dosyası bulunamadı");
        long handle=openNative(dwg.getAbsolutePath());
        if(handle==0)throw new IOException("DWG native motor tarafından açılamadı");
        return new NativeCadEngine(handle);
    }

    private NativeCadEngine(long handle){this.handle=handle;}

    public synchronized boolean isOpen(){return handle!=0;}

    public synchronized Stats stats(){
        requireOpen();
        if(cachedStats==null)cachedStats=new Stats(statsNative(handle));
        return cachedStats;
    }

    public synchronized int exportDxf(File output)throws IOException{
        requireOpen();
        if(output==null)throw new IOException("DXF hedefi belirtilmedi");
        int result=exportDxfNative(handle,output.getAbsolutePath());
        if(result<0)throw new IOException("DWG dönüşümü başarısız ("+result+")");
        return result;
    }

    private void requireOpen(){if(handle==0)throw new IllegalStateException("Native CAD oturumu kapalı");}

    @Override public synchronized void close(){
        if(handle==0)return;
        long value=handle;handle=0;cachedStats=null;
        closeNative(value);
    }
}
