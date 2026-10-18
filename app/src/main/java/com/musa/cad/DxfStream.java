package com.musa.cad;

import java.io.*;
import java.nio.charset.Charset;
import java.util.*;

/**
 * Bounded streaming DXF record reader.
 * Keeps one code-0 record in memory at a time and tracks both byte and text-line offsets.
 */
public final class DxfStream implements Closeable {
    public static final long MAX_BYTES=512L*1024L*1024L;
    private static final int MAX_LINE_BYTES=1024*1024;
    private static final int MAX_RECORD_TAG_STRINGS=2_000_000;
    private static final int MAX_PAIRS=32_000_000;

    private final RandomAccessFile file;
    private final Charset charset;
    private final byte[] buffer=new byte[64*1024];
    private final int[] position=new int[1];
    private int available,lineIndex,pairsRead;
    private long offset;
    private long nativeLines;

    private String pendingType;
    private long pendingStartOffset;
    private int pendingStartLine;

    /** Byte/line position of the next code-0 record after the record just returned. */
    public long nextRecordOffset;
    public int nextRecordLine;

    public DxfStream(File source,Charset charset)throws IOException{
        if(source==null||!source.isFile())throw new IOException("DXF dosyası bulunamadı");
        if(source.length()>MAX_BYTES)throw new IOException("DXF dosyası 512 MB sınırını aşıyor");
        this.file=new RandomAccessFile(source,"r");
        this.charset=charset;
        try{
            nativeLines=DxfPortableLines.openRecordFramer(MAX_LINE_BYTES);
        }catch(IOException failed){
            file.close();
            throw failed;
        }
    }

    public void seek(long byteOffset,int sourceLine)throws IOException{
        if(byteOffset<0||byteOffset>file.length())throw new IOException("Geçersiz DXF konumu");
        // Reset the same portable scanner on every random-access seek.
        // Acquire the replacement before destroying the original cursor.
        final long replacement=DxfPortableLines.openRecordFramer(MAX_LINE_BYTES);
        try{file.seek(byteOffset);}
        catch(IOException failed){
            DxfPortableLines.closeRecordFramer(replacement);
            throw failed;
        }
        final long previous=nativeLines;
        nativeLines=replacement;
        DxfPortableLines.closeRecordFramer(previous);
        offset=byteOffset;lineIndex=Math.max(0,sourceLine);
        position[0]=0;available=0;pairsRead=0;
        pendingType=null;pendingStartOffset=0;pendingStartLine=0;
        nextRecordOffset=byteOffset;nextRecordLine=lineIndex;
    }

    private String line()throws IOException{
        // C owns ALL LF/CR framing, split-buffer state and bounded line size.
        // Java keeps only platform file I/O, cancellation and text decoding.
        if(available<0)return null;
        while(true){
            if(position[0]<available){
                int before=position[0];
                byte[] bytes=DxfPortableLines.feedRecordFramer(
                    nativeLines,buffer,position,available);
                if(position[0]<before||position[0]>available)
                    throw new IOException("Ortak DXF okuyucu konumu geçersiz");
                offset+=position[0]-before;
                if(bytes!=null){
                    lineIndex++;
                    return new String(bytes,charset);
                }
                if(position[0]!=available)
                    throw new IOException("Ortak DXF okuyucu ilerleyemedi");
            }
            FileTransfer.checkCancelled();
            available=file.read(buffer);
            position[0]=0;
            if(available<0){
                // Finish ONCE; without this guard EOF could replay old bytes.
                byte[] finalLine=DxfPortableLines.finishRecordFramer(nativeLines);
                if(finalLine==null)return null;
                lineIndex++;
                return new String(finalLine,charset);
            }
        }
    }

    public DxfBlocks.Record next()throws IOException{
        FileTransfer.checkCancelled();
        String type=pendingType;long recordStartOffset=pendingStartOffset;int recordStartLine=pendingStartLine;
        pendingType=null;
        ArrayList<String> tags=new ArrayList<>(32);

        while(true){
            long pairOffset=offset;int pairLine=lineIndex;
            String codeLine=line();
            if(codeLine==null){
                if(type==null)return null;
                nextRecordOffset=offset;nextRecordLine=lineIndex;
                return new DxfBlocks.Record(type,tags,0,tags.size(),recordStartLine+2,recordStartLine,lineIndex);
            }
            String value=line();
            int code=DxfGroupCode.parsePair(codeLine,value,pairsRead,MAX_PAIRS);
            pairsRead++;

            if(code==0){
                String nextType=value.trim();
                if(type==null){
                    type=nextType;recordStartOffset=pairOffset;recordStartLine=pairLine;
                    continue;
                }
                pendingType=nextType;pendingStartOffset=pairOffset;pendingStartLine=pairLine;
                nextRecordOffset=pairOffset;nextRecordLine=pairLine;
                return new DxfBlocks.Record(type,tags,0,tags.size(),recordStartLine+2,recordStartLine,pairLine);
            }

            // Ignore preamble garbage before the first code-0 record, but never retain it.
            if(type==null)continue;
            if(tags.size()+2>MAX_RECORD_TAG_STRINGS)throw new IOException("DXF nesnesi çok büyük");
            tags.add(Integer.toString(code));tags.add(value);
        }
    }

    public void close()throws IOException{
        final long old=nativeLines;
        nativeLines=0L;
        try{file.close();}
        finally{DxfPortableLines.closeRecordFramer(old);}
    }
}
