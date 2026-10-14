package com.musa.cad;

import java.io.File;
import java.io.IOException;

/** Persistent native DWG session. The full editor still upgrades to the proven DXF model after fast first paint. */
public final class NativeCadEngine implements AutoCloseable {
    static { System.loadLibrary("musacad_dwg"); }

    private static native long openNative(String input) throws IOException;
    private static native int exportDxfNative(long handle,String output);
    private static native long[] statsNative(long handle);
    private static native float[] sceneNative(long handle);
    private static native float[] scenePageNative(long handle,long firstIndex,int maxWords,long[] cursor) throws IOException;
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
        public boolean coreOnly(){return entityCount>0&&unsupportedEntityCount==0;}
        public double coreCoverage(){return entityCount<=0?0d:Math.min(1d,Math.max(0d,coreEntityCount/(double)entityCount));}
    }

    /** One bounded, self-contained native preview page. Cursor is in top-level DWG entities. */
    public static final class ScenePage {
        public final NativeScene scene;
        public final long nextTopLevelIndex;
        public final boolean hasMore;
        private ScenePage(NativeScene scene,long nextIndex,boolean more){
            this.scene=scene;this.nextTopLevelIndex=nextIndex;this.hasMore=more;
        }
    }

    /**
     * Explicit preview paging for very large DWGs, not used by production
     * first paint until an incremental renderer can consume pages safely.
     * Use a fixed bounded word budget (e.g., 1,048,576 floats = 4 MiB raw).
     * An individual expanded INSERT exceeding that budget is rejected, never
     * partially returned. Such blocks require nested-entity continuation.
     */
    public synchronized ScenePage previewPage(long firstIndex,int maxFloatWords)throws IOException{
        requireOpen();
        if(firstIndex<0||maxFloatWords<256||maxFloatWords>9*1024*1024)
            throw new IOException("Geçersiz DWG sayfa sınırı");
        long[] cursor=new long[2];
        float[] values=scenePageNative(handle,firstIndex,maxFloatWords,cursor);
        if(values==null||cursor[0]<=firstIndex)
            throw new IOException("DWG sahne sayfası alınamadı");
        NativeScene scene=NativeScene.fromRaw(values);
        if(scene.truncated)
            throw new IOException("Eksik DWG sahnesi görüntülenemez");
        return new ScenePage(scene,cursor[0],cursor[1]!=0);
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
    public synchronized Stats stats(){requireOpen();if(cachedStats==null)cachedStats=new Stats(statsNative(handle));return cachedStats;}
    public synchronized NativeScene fastScene()throws IOException{requireOpen();return NativeScene.fromRaw(sceneNative(handle));}
    public synchronized int exportDxf(File output)throws IOException{
        requireOpen();if(output==null)throw new IOException("DXF hedefi belirtilmedi");int result=exportDxfNative(handle,output.getAbsolutePath());if(result<0)throw new IOException("DWG dönüşümü başarısız ("+result+")");return result;
    }
    private void requireOpen(){if(handle==0)throw new IllegalStateException("Native CAD oturumu kapalı");}
    @Override public synchronized void close(){if(handle==0)return;long value=handle;handle=0;cachedStats=null;closeNative(value);}
}
