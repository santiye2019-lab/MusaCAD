package com.musa.cad;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * Staged DXF byte-stream transport into the ONE portable C11 line framer.
 * Java owns file I/O, cancellation and ISO-8859-1 presentation only.
 * Native code owns newline, CR stripping, split chunks, EOF and line cap.
 */
public final class DxfPortableLines implements Closeable {
    private static final int CHUNK_BYTES=64*1024;
    private static final int MAX_LINE_BYTES=1024*1024;
    private static final boolean coreLoaded;
    static {
        boolean available=false;
        try {
            System.loadLibrary("musacad_dwg");
            available=true;
        }catch(LinkageError | SecurityException missing) {
            // Never fall back to a second Java DXF byte parser.
        }
        coreLoaded=available;
    }
    private static native long openNative(int maxLineBytes,int crMode);
    private static native byte[] nextNative(long context,byte[] chunk,
                                           int[] offset,int limit)throws IOException;
    private static native byte[] finishNative(long context)throws IOException;
    private static native void closeNative(long context);

    /* Same JNI transport for staged-save and seekable DxfStream adapters.
     * Mode 0: staged-save legacy drop-all-CR. Mode 1: trailing-only CR.
     * Fail closed if portable C is unavailable; never parse bytes in Java.
     */
    static long openRecordFramer(int maxLineBytes)throws IOException{
        if(!coreLoaded)throw new IOException("Ortak CAD çekirdeği olmadan DXF satırı okunamaz");
        final long handle;
        try{handle=openNative(maxLineBytes,1);}
        catch(LinkageError missing){
            throw new IOException("Ortak CAD çekirdeği DXF satır JNI bağlantısı yok",missing);
        }
        if(handle==0L)throw new IOException("Ortak DXF satır okuyucusu oluşturulamadı");
        return handle;
    }
    static byte[] feedRecordFramer(long handle,byte[] bytes,int[] offset,int limit)
            throws IOException{
        try{return nextNative(handle,bytes,offset,limit);}
        catch(LinkageError missing){
            throw new IOException("Ortak DXF satır JNI bağlantısı yok",missing);
        }
    }
    static byte[] finishRecordFramer(long handle)throws IOException{
        try{return finishNative(handle);}
        catch(LinkageError missing){
            throw new IOException("Ortak DXF satır JNI bağlantısı yok",missing);
        }
    }
    static void closeRecordFramer(long handle){if(handle!=0L)closeNative(handle);}

    private final FileInputStream input;
    private final byte[] chunk=new byte[CHUNK_BYTES];
    private final int[] offset=new int[1];
    private int length;
    private long context;
    private boolean ended;

    public DxfPortableLines(File source)throws IOException {
        if(source==null||!source.isFile())throw new IOException("DXF dosyası bulunamadı");
        if(!coreLoaded)throw new IOException("Ortak CAD çekirdeği olmadan DXF satırı okunamaz");
        final long nativeHandle;
        try {
            nativeHandle=openNative(MAX_LINE_BYTES,0);
        }catch(LinkageError unavailable){
            throw new IOException("Ortak CAD çekirdeği DXF satır işlemi yok",unavailable);
        }
        if(nativeHandle==0)throw new IOException("DXF satır okuyucu oluşturulamadı");
        context=nativeHandle;
        try{
            input=new FileInputStream(source);
        }catch(IOException failure){
            closeNative(context);
            context=0;
            throw failure;
        }
    }

    /** Returns null at actual EOF, but preserves empty DXF value lines. */
    public String line()throws IOException {
        if(context==0)throw new IOException("DXF okuyucu kapatıldı");
        if(ended)return null;
        try {
            while(true) {
                FileTransfer.checkCancelled();
                if(offset[0]<length) {
                    byte[] line=nextNative(context,chunk,offset,length);
                    if(line!=null)return new String(line,StandardCharsets.ISO_8859_1);
                }
                if(offset[0]!=length)
                    throw new IOException("Ortak DXF okuyucu ilerleyemedi");
                length=input.read(chunk);
                offset[0]=0;
                if(length<0){
                    ended=true;
                    byte[] last=finishNative(context);
                    return last==null?null:new String(last,StandardCharsets.ISO_8859_1);
                }
            }
        }catch(LinkageError missing){
            throw new IOException("Ortak DXF satır JNI modülü bulunamadı",missing);
        }
    }

    @Override public void close()throws IOException {
        long old=context;
        context=0L;
        try {input.close();}
        finally {if(old!=0L)closeNative(old);}
    }
}
